// WebGPU driver for the Monte Carlo ensemble (web build only; see mc_webgpu.hpp).
// The per-draw physics is the WGSL port of simulate_draw<float>() in mc_webgpu.wgsl;
// this file sets up the device, maps draws to invocations and reads the histograms
// back, as mc_gpu.cu does for CUDA.
//
// Threads: every WebGPU call runs on the main browser thread, which owns the device.
// A worker that wants an ensemble proxies a task to the main thread with
// emscripten_proxy_sync_with_ctx() and blocks; the main thread records and submits the
// commands and returns to the browser at once; the task ends in the buffer-mapping
// callback, which copies the results and releases the worker
// (emscripten_proxy_finish). All callbacks use WGPUCallbackMode_AllowSpontaneous, so
// they run from the browser's event loop and need neither JSPI nor Asyncify.
#include "mc_webgpu.hpp"

#include <emscripten/emscripten.h>
#include <emscripten/eventloop.h>
#include <emscripten/proxying.h>
#include <emscripten/threading.h>
#include <webgpu/webgpu.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <utility>

#include "mc_webgpu_wgsl.hpp"  // kMcWebGpuWgsl: mc_webgpu.wgsl, embedded at build time (cmake/EmbedText.cmake)

namespace e47 {

// mc_webgpu.wgsl declares McInputs member for member. Every member is a 4-byte int or
// float, so the two layouts agree while the size and these offsets hold; when McInputs
// changes, change the WGSL struct with it.
static_assert(sizeof(McInputs) == 2014 * 4, "McInputs changed: update struct McInputs in mc_webgpu.wgsl");
static_assert(offsetof(McInputs, g_level_sd) == 166 * 4 && offsetof(McInputs, n_features) == 185 * 4 &&
                  offsetof(McInputs, n_states) == 266 * 4 && offsetof(McInputs, state_pop_share) == 307 * 4 &&
                  offsetof(McInputs, conv_mean) == 1627 * 4 && offsetof(McInputs, hist_lo) == 1630 * 4,
              "McInputs layout changed: update struct McInputs in mc_webgpu.wgsl");

namespace {

constexpr uint32_t kWorkgroup = 64;       // @workgroup_size of mc_main and philox_main
constexpr uint32_t kChunk = 1u << 20;     // draws per dispatch: 16,384 workgroups
constexpr uint32_t kParamStride = 256;    // minUniformBufferOffsetAlignment (WebGPU default limit)
constexpr size_t kHistWords = static_cast<size_t>(kSeries) * kMaxYears * kBins;
constexpr uint64_t kHistBytes = kHistWords * sizeof(uint32_t);
constexpr double kTimeoutMs = 60000;      // a task still running after this counts as failed

// struct Params in mc_webgpu.wgsl
struct Params {
  uint32_t seed_lo, seed_hi, first, count, n_keep, pad[3];
};
static_assert(sizeof(Params) == 32);

WGPUStringView sv(const char* s) { return WGPUStringView{s, WGPU_STRLEN}; }

std::string str(WGPUStringView v) {
  if (!v.data) return {};
  return v.length == WGPU_STRLEN ? std::string(v.data) : std::string(v.data, v.length);
}

// --- State ----------------------------------------------------------------------

// WebGPU objects: main browser thread only.
struct Gpu {
  WGPUInstance instance = nullptr;
  WGPUAdapter adapter = nullptr;
  WGPUDevice device = nullptr;
  WGPUQueue queue = nullptr;
  WGPUShaderModule module = nullptr;
  WGPUBindGroupLayout mc_layout = nullptr;
  WGPUPipelineLayout mc_pipeline_layout = nullptr;
  WGPUComputePipeline mc = nullptr;      // mc_main
  WGPUComputePipeline philox = nullptr;  // philox_main (self-test)
  double t_init = 0, t_adapter = 0, t_device = 0;  // page time (ms) of the set-up steps
};
Gpu g_gpu;

std::atomic<int> g_state{static_cast<int>(WebGpuState::kOff)};
std::mutex g_text_mutex;
std::string g_name;    // adapter, under g_text_mutex
std::string g_status;  // why WebGPU is not used, under g_text_mutex

void set_ready() {
  int expected = static_cast<int>(WebGpuState::kPending);
  if (!g_state.compare_exchange_strong(expected, static_cast<int>(WebGpuState::kReady))) return;
  const double now = emscripten_performance_now();
  std::lock_guard<std::mutex> lock(g_text_mutex);
  std::printf("WebGPU ready on %s at %.0f ms (set-up %.0f ms: adapter %.0f, device %.0f, pipelines %.0f): "
              "the Monte Carlo runs on the GPU.\n",
              g_name.c_str(), now, now - g_gpu.t_init, g_gpu.t_adapter - g_gpu.t_init,
              g_gpu.t_device - g_gpu.t_adapter, now - g_gpu.t_device);
}

// From any thread: WebGPU is not used from now on; the first reason is kept.
void fail(const std::string& why) {
  if (g_state.exchange(static_cast<int>(WebGpuState::kFailed)) == static_cast<int>(WebGpuState::kFailed)) return;
  {
    std::lock_guard<std::mutex> lock(g_text_mutex);
    g_status = why;
  }
  std::printf("WebGPU not used (%s): the Monte Carlo runs on the CPU.\n", why.c_str());
}

// Adapter name. Browsers often hide the device name, so vendor and architecture
// ("NVIDIA Ampere") stand in for it.
std::string adapter_name(WGPUAdapter a) {
  WGPUAdapterInfo info = WGPU_ADAPTER_INFO_INIT;
  if (wgpuAdapterGetInfo(a, &info) != WGPUStatus_Success) return "adapter";
  std::string name = str(info.description);
  if (name.empty()) name = str(info.device);
  if (name.empty()) {
    std::string vendor = str(info.vendor), arch = str(info.architecture);
    static const std::pair<const char*, const char*> kVendors[] = {
        {"nvidia", "NVIDIA"}, {"amd", "AMD"}, {"intel", "Intel"}, {"apple", "Apple"},
        {"arm", "Arm"},       {"qualcomm", "Qualcomm"}, {"microsoft", "Microsoft"}};
    for (const auto& [id, pretty] : kVendors)
      if (vendor == id) vendor = pretty;
    if (!arch.empty()) arch[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(arch[0])));
    name = vendor + (vendor.empty() || arch.empty() ? "" : " ") + arch;
  }
  wgpuAdapterInfoFreeMembers(info);
  return name.empty() ? std::string("adapter") : name;
}

// --- Set-up (main thread) ----------------------------------------------------------

void on_pipeline(WGPUCreatePipelineAsyncStatus status, WGPUComputePipeline pipeline, WGPUStringView message,
                 void* slot, void*) {
  if (status != WGPUCreatePipelineAsyncStatus_Success || !pipeline) {
    fail("compute pipeline: " + str(message));
    return;
  }
  *static_cast<WGPUComputePipeline*>(slot) = pipeline;
  if (g_gpu.mc && g_gpu.philox) set_ready();
}

void on_device_lost(WGPUDevice const*, WGPUDeviceLostReason, WGPUStringView message, void*, void*) {
  fail("device lost: " + str(message));
}

void on_uncaptured_error(WGPUDevice const*, WGPUErrorType, WGPUStringView message, void*, void*) {
  fail("uncaptured error: " + str(message));
}

void on_device(WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, void*, void*) {
  if (status != WGPURequestDeviceStatus_Success || !device) {
    fail("device request: " + str(message));
    return;
  }
  g_gpu.t_device = emscripten_performance_now();
  g_gpu.device = device;
  g_gpu.queue = wgpuDeviceGetQueue(device);

  WGPUShaderSourceWGSL wgsl = WGPU_SHADER_SOURCE_WGSL_INIT;
  wgsl.code = sv(kMcWebGpuWgsl);
  WGPUShaderModuleDescriptor md = WGPU_SHADER_MODULE_DESCRIPTOR_INIT;
  md.nextInChain = &wgsl.chain;
  md.label = sv("e47 Monte Carlo");
  g_gpu.module = wgpuDeviceCreateShaderModule(device, &md);

  // mc_main: inputs, bootstrap replicates, histograms, kept features, and the
  // per-dispatch parameters (dynamic offset: one Params slot per dispatch).
  const WGPUBufferBindingType types[5] = {WGPUBufferBindingType_ReadOnlyStorage, WGPUBufferBindingType_ReadOnlyStorage,
                                          WGPUBufferBindingType_Storage, WGPUBufferBindingType_Storage,
                                          WGPUBufferBindingType_Uniform};
  WGPUBindGroupLayoutEntry e[5];
  for (uint32_t i = 0; i < 5; ++i) {
    e[i] = WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT;
    e[i].binding = i;
    e[i].visibility = WGPUShaderStage_Compute;
    e[i].buffer.type = types[i];
  }
  e[4].buffer.hasDynamicOffset = 1;
  e[4].buffer.minBindingSize = sizeof(Params);
  WGPUBindGroupLayoutDescriptor ld = WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;
  ld.entryCount = 5;
  ld.entries = e;
  g_gpu.mc_layout = wgpuDeviceCreateBindGroupLayout(device, &ld);
  WGPUPipelineLayoutDescriptor pld = WGPU_PIPELINE_LAYOUT_DESCRIPTOR_INIT;
  pld.bindGroupLayoutCount = 1;
  pld.bindGroupLayouts = &g_gpu.mc_layout;
  g_gpu.mc_pipeline_layout = wgpuDeviceCreatePipelineLayout(device, &pld);

  const auto create = [&](const char* entry, WGPUPipelineLayout layout, WGPUComputePipeline* slot) {
    WGPUComputePipelineDescriptor pd = WGPU_COMPUTE_PIPELINE_DESCRIPTOR_INIT;
    pd.label = sv(entry);
    pd.layout = layout;  // null = layout derived from the shader (philox_main)
    pd.compute.module = g_gpu.module;
    pd.compute.entryPoint = sv(entry);
    WGPUCreateComputePipelineAsyncCallbackInfo cb{};
    cb.mode = WGPUCallbackMode_AllowSpontaneous;
    cb.callback = on_pipeline;
    cb.userdata1 = slot;
    wgpuDeviceCreateComputePipelineAsync(device, &pd, cb);
  };
  create("mc_main", g_gpu.mc_pipeline_layout, &g_gpu.mc);
  create("philox_main", nullptr, &g_gpu.philox);
}

void on_adapter(WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, void*, void*) {
  if (status != WGPURequestAdapterStatus_Success || !adapter) {
    fail(message.data ? str(message) : "no adapter");
    return;
  }
  g_gpu.t_adapter = emscripten_performance_now();
  g_gpu.adapter = adapter;
  {
    std::lock_guard<std::mutex> lock(g_text_mutex);
    g_name = adapter_name(adapter);
  }
  WGPUDeviceDescriptor dd = WGPU_DEVICE_DESCRIPTOR_INIT;
  dd.label = sv("e47 Monte Carlo");
  dd.deviceLostCallbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
  dd.deviceLostCallbackInfo.callback = on_device_lost;
  dd.uncapturedErrorCallbackInfo.callback = on_uncaptured_error;
  WGPURequestDeviceCallbackInfo cb{};
  cb.mode = WGPUCallbackMode_AllowSpontaneous;
  cb.callback = on_device;
  wgpuAdapterRequestDevice(adapter, &dd, cb);
}

// --- Tasks (main thread) -------------------------------------------------------------

// What a worker asks for; it lives on the blocked worker's stack.
struct Request {
  // Ensemble
  const McInputs* in = nullptr;
  const std::vector<float>* boot = nullptr;
  uint64_t seed = 0;
  uint32_t draws = 0;
  uint32_t n_keep = 0;
  McResult* result = nullptr;
  // Philox self-test
  const std::vector<std::array<uint32_t, 6>>* items = nullptr;
  std::vector<U32x4>* bits = nullptr;

  std::string error;
};

// One submission. It outlives its Request when a timeout releases the worker first,
// and is deleted once every callback has returned.
struct Task {
  Request* req = nullptr;  // null once the worker has been released
  em_proxying_ctx* ctx = nullptr;
  std::vector<WGPUBuffer> buffers;
  WGPUBindGroup group = nullptr;
  WGPUBuffer readback = nullptr;
  uint64_t readback_bytes = 0;
  int pending = 0;  // callbacks still to come
  int timeout = 0;
  bool mapped = false;
  std::string error;
  double t0 = 0;
};

WGPUBuffer make_buffer(Task* t, WGPUBufferUsage usage, uint64_t bytes, const char* label) {
  WGPUBufferDescriptor bd = WGPU_BUFFER_DESCRIPTOR_INIT;
  bd.label = sv(label);
  bd.usage = usage;
  bd.size = std::max<uint64_t>(bytes, 16);
  WGPUBuffer b = wgpuDeviceCreateBuffer(g_gpu.device, &bd);
  if (b) t->buffers.push_back(b);
  return b;
}

WGPUBindGroupEntry buffer_entry(uint32_t binding, WGPUBuffer buffer, uint64_t size) {
  WGPUBindGroupEntry be = WGPU_BIND_GROUP_ENTRY_INIT;
  be.binding = binding;
  be.buffer = buffer;
  be.offset = 0;
  be.size = size;
  return be;
}

void release_worker(Task* t, const std::string& error) {
  if (!t->req) return;
  t->req->error = error;
  em_proxying_ctx* ctx = t->ctx;
  t->req = nullptr;
  emscripten_proxy_finish(ctx);
}

void end_callback(Task* t) {
  if (--t->pending > 0) return;
  emscripten_clear_timeout(t->timeout);
  std::string error = t->error;
  if (error.empty() && !t->mapped) error = "results not read back";
  release_worker(t, error);
  for (WGPUBuffer b : t->buffers) {
    wgpuBufferDestroy(b);
    wgpuBufferRelease(b);
  }
  if (t->group) wgpuBindGroupRelease(t->group);
  delete t;
}

void on_timeout(void* arg) {
  Task* t = static_cast<Task*>(arg);
  release_worker(t, "the GPU did not finish within " + std::to_string(static_cast<int>(kTimeoutMs / 1000)) + " s");
}

void on_scope(WGPUPopErrorScopeStatus status, WGPUErrorType type, WGPUStringView message, void* arg, void*) {
  Task* t = static_cast<Task*>(arg);
  if (t->error.empty()) {
    if (status != WGPUPopErrorScopeStatus_Success)
      t->error = "error scope: " + str(message);
    else if (type != WGPUErrorType_NoError)
      t->error = str(message);
  }
  end_callback(t);
}

// Copies the mapped results into the worker's request.
void copy_out(Task* t) {
  Request& r = *t->req;
  if (r.result) {
    std::vector<uint32_t> h(kHistWords);
    WGPUStatus s = wgpuBufferReadMappedRange(t->readback, 0, h.data(), kHistBytes);
    r.result->hist.assign(h.begin(), h.end());
    if (r.n_keep > 0) {
      r.result->keep.resize(static_cast<size_t>(r.n_keep) * kKeepFeatures);
      if (s == WGPUStatus_Success)
        s = wgpuBufferReadMappedRange(t->readback, kHistBytes, r.result->keep.data(),
                                      r.result->keep.size() * sizeof(float));
    }
    r.result->ms = emscripten_get_now() - t->t0;
    if (s != WGPUStatus_Success && t->error.empty()) t->error = "could not read the mapped results";
  } else {
    r.bits->resize(r.items->size());
    if (wgpuBufferReadMappedRange(t->readback, 0, r.bits->data(), r.bits->size() * sizeof(U32x4)) !=
            WGPUStatus_Success &&
        t->error.empty())
      t->error = "could not read the mapped results";
  }
}

void on_mapped(WGPUMapAsyncStatus status, WGPUStringView message, void* arg, void*) {
  Task* t = static_cast<Task*>(arg);
  if (status == WGPUMapAsyncStatus_Success) {
    t->mapped = true;
    if (t->req) copy_out(t);
    wgpuBufferUnmap(t->readback);
  } else if (t->error.empty()) {
    t->error = "mapAsync: " + str(message);
  }
  end_callback(t);
}

// The ensemble: one dispatch per kChunk draws, then the histograms and kept features
// are copied into one readback buffer.
bool encode_mc(Task* t, WGPUCommandEncoder enc) {
  const Request& r = *t->req;
  const uint64_t boot_bytes = r.boot->size() * sizeof(float);
  const uint64_t keep_bytes = static_cast<uint64_t>(r.n_keep) * kKeepFeatures * sizeof(float);
  const uint32_t chunks = (r.draws + kChunk - 1) / kChunk;

  WGPUBuffer in = make_buffer(t, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, sizeof(McInputs), "inputs");
  WGPUBuffer boot = make_buffer(t, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, boot_bytes, "boot");
  WGPUBuffer hist = make_buffer(t, WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc, kHistBytes, "hist");
  WGPUBuffer keep = make_buffer(t, WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc, keep_bytes, "keep");
  WGPUBuffer prm = make_buffer(t, WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst,
                               static_cast<uint64_t>(chunks) * kParamStride, "params");
  t->readback_bytes = kHistBytes + keep_bytes;
  t->readback = make_buffer(t, WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst, t->readback_bytes, "readback");
  if (t->buffers.size() != 6) return false;

  wgpuQueueWriteBuffer(g_gpu.queue, in, 0, r.in, sizeof(McInputs));
  if (boot_bytes) wgpuQueueWriteBuffer(g_gpu.queue, boot, 0, r.boot->data(), boot_bytes);
  std::vector<uint8_t> params(static_cast<size_t>(chunks) * kParamStride, 0);
  for (uint32_t c = 0; c < chunks; ++c) {
    const uint32_t first = c * kChunk;
    const Params p{static_cast<uint32_t>(r.seed), static_cast<uint32_t>(r.seed >> 32), first,
                   std::min(kChunk, r.draws - first), r.n_keep, {0, 0, 0}};
    std::memcpy(params.data() + static_cast<size_t>(c) * kParamStride, &p, sizeof p);
  }
  wgpuQueueWriteBuffer(g_gpu.queue, prm, 0, params.data(), params.size());

  const WGPUBindGroupEntry be[5] = {buffer_entry(0, in, sizeof(McInputs)),
                                    buffer_entry(1, boot, std::max<uint64_t>(boot_bytes, 16)),
                                    buffer_entry(2, hist, kHistBytes),
                                    buffer_entry(3, keep, std::max<uint64_t>(keep_bytes, 16)),
                                    buffer_entry(4, prm, sizeof(Params))};
  WGPUBindGroupDescriptor bgd = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
  bgd.layout = g_gpu.mc_layout;
  bgd.entryCount = 5;
  bgd.entries = be;
  t->group = wgpuDeviceCreateBindGroup(g_gpu.device, &bgd);

  // One submission per dispatch, so that no single submission keeps a slow GPU busy for
  // long (operating systems reset a GPU that stays busy for about two seconds).
  for (uint32_t c = 0; c < chunks; ++c) {
    const uint32_t offset = c * kParamStride;
    const uint32_t count = std::min(kChunk, r.draws - c * kChunk);
    WGPUCommandEncoder ce = wgpuDeviceCreateCommandEncoder(g_gpu.device, nullptr);
    WGPUComputePassEncoder pass = wgpuCommandEncoderBeginComputePass(ce, nullptr);
    wgpuComputePassEncoderSetPipeline(pass, g_gpu.mc);
    wgpuComputePassEncoderSetBindGroup(pass, 0, t->group, 1, &offset);
    wgpuComputePassEncoderDispatchWorkgroups(pass, (count + kWorkgroup - 1) / kWorkgroup, 1, 1);
    wgpuComputePassEncoderEnd(pass);
    wgpuComputePassEncoderRelease(pass);
    WGPUCommandBuffer cb = wgpuCommandEncoderFinish(ce, nullptr);
    wgpuQueueSubmit(g_gpu.queue, 1, &cb);
    wgpuCommandBufferRelease(cb);
    wgpuCommandEncoderRelease(ce);
  }
  // `enc` (submitted by the caller after these) copies the results for reading back.
  wgpuCommandEncoderCopyBufferToBuffer(enc, hist, 0, t->readback, 0, kHistBytes);
  if (keep_bytes) wgpuCommandEncoderCopyBufferToBuffer(enc, keep, 0, t->readback, kHistBytes, keep_bytes);
  return true;
}

// The Philox self-test: item i -> philox4x32_10(counter, key) in one dispatch.
bool encode_philox(Task* t, WGPUCommandEncoder enc) {
  const Request& r = *t->req;
  const uint64_t n = r.items->size();
  std::vector<uint32_t> data(static_cast<size_t>(n) * 8, 0);  // per item: counter (4 words), key (2) and padding
  for (size_t i = 0; i < n; ++i) std::copy((*r.items)[i].begin(), (*r.items)[i].end(), data.begin() + 8 * i);
  WGPUBuffer in = make_buffer(t, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, n * 32, "philox in");
  WGPUBuffer out = make_buffer(t, WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc, n * 16, "philox out");
  t->readback_bytes = n * 16;
  t->readback = make_buffer(t, WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst, t->readback_bytes, "readback");
  if (t->buffers.size() != 3) return false;
  wgpuQueueWriteBuffer(g_gpu.queue, in, 0, data.data(), n * 32);

  const WGPUBindGroupEntry be[2] = {buffer_entry(5, in, n * 32), buffer_entry(6, out, n * 16)};
  WGPUBindGroupLayout layout = wgpuComputePipelineGetBindGroupLayout(g_gpu.philox, 0);
  WGPUBindGroupDescriptor bgd = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
  bgd.layout = layout;
  bgd.entryCount = 2;
  bgd.entries = be;
  t->group = wgpuDeviceCreateBindGroup(g_gpu.device, &bgd);
  wgpuBindGroupLayoutRelease(layout);

  WGPUComputePassEncoder pass = wgpuCommandEncoderBeginComputePass(enc, nullptr);
  wgpuComputePassEncoderSetPipeline(pass, g_gpu.philox);
  wgpuComputePassEncoderSetBindGroup(pass, 0, t->group, 0, nullptr);
  wgpuComputePassEncoderDispatchWorkgroups(pass, static_cast<uint32_t>((n + kWorkgroup - 1) / kWorkgroup), 1, 1);
  wgpuComputePassEncoderEnd(pass);
  wgpuComputePassEncoderRelease(pass);
  wgpuCommandEncoderCopyBufferToBuffer(enc, out, 0, t->readback, 0, n * 16);
  return true;
}

// Proxied from a worker: records and submits the task, then returns to the browser.
void start_task(em_proxying_ctx* ctx, void* arg) {
  Request* req = static_cast<Request*>(arg);
  if (static_cast<WebGpuState>(g_state.load()) != WebGpuState::kReady) {
    req->error = "not ready";
    emscripten_proxy_finish(ctx);
    return;
  }
  Task* t = new Task;
  t->req = req;
  t->ctx = ctx;
  t->t0 = emscripten_get_now();

  wgpuDevicePushErrorScope(g_gpu.device, WGPUErrorFilter_OutOfMemory);
  wgpuDevicePushErrorScope(g_gpu.device, WGPUErrorFilter_Validation);
  WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(g_gpu.device, nullptr);
  const bool ok = req->result ? encode_mc(t, enc) : encode_philox(t, enc);
  if (ok) {
    WGPUCommandBuffer cb = wgpuCommandEncoderFinish(enc, nullptr);
    wgpuQueueSubmit(g_gpu.queue, 1, &cb);
    wgpuCommandBufferRelease(cb);
  } else {
    t->error = "could not create the GPU buffers";
  }
  wgpuCommandEncoderRelease(enc);

  // The callbacks that end the task: two error scopes and, after a submission, the
  // readback mapping. Nothing touches t after the last call below, since the last
  // callback deletes t.
  t->pending = ok ? 3 : 2;
  t->timeout = emscripten_set_timeout(on_timeout, kTimeoutMs, t);
  WGPUPopErrorScopeCallbackInfo scb{};
  scb.mode = WGPUCallbackMode_AllowSpontaneous;
  scb.callback = on_scope;
  scb.userdata1 = t;
  if (ok) {
    WGPUBufferMapCallbackInfo mcb{};
    mcb.mode = WGPUCallbackMode_AllowSpontaneous;
    mcb.callback = on_mapped;
    mcb.userdata1 = t;
    wgpuBufferMapAsync(t->readback, WGPUMapMode_Read, 0, static_cast<size_t>(t->readback_bytes), mcb);
  }
  wgpuDevicePopErrorScope(g_gpu.device, scb);
  wgpuDevicePopErrorScope(g_gpu.device, scb);
}

// Worker side: hands the request to the main thread and waits for it.
void run_on_main_thread(Request& req) {
  if (emscripten_is_main_runtime_thread())
    throw std::logic_error("WebGPU tasks must be started from a worker thread, not the main browser thread");
  if (webgpu_state() != WebGpuState::kReady) throw std::runtime_error("WebGPU is not ready");
  if (!emscripten_proxy_sync_with_ctx(emscripten_proxy_get_system_queue(), emscripten_main_runtime_thread_id(),
                                      start_task, &req))
    req.error = "the main thread did not take the task";
  if (!req.error.empty()) {
    fail(req.error);
    throw std::runtime_error("WebGPU: " + req.error);
  }
}

}  // namespace

void webgpu_init() {
  if (!emscripten_is_main_runtime_thread()) return;
  int expected = static_cast<int>(WebGpuState::kOff);
  if (!g_state.compare_exchange_strong(expected, static_cast<int>(WebGpuState::kPending))) return;
  g_gpu.t_init = emscripten_performance_now();
  g_gpu.instance = wgpuCreateInstance(nullptr);
  if (!g_gpu.instance) {
    fail("no WebGPU instance");
    return;
  }
  WGPURequestAdapterOptions opt = WGPU_REQUEST_ADAPTER_OPTIONS_INIT;
  opt.powerPreference = WGPUPowerPreference_HighPerformance;
  WGPURequestAdapterCallbackInfo cb{};
  cb.mode = WGPUCallbackMode_AllowSpontaneous;
  cb.callback = on_adapter;
  wgpuInstanceRequestAdapter(g_gpu.instance, &opt, cb);  // no navigator.gpu: on_adapter fails at once
}

WebGpuState webgpu_state() { return static_cast<WebGpuState>(g_state.load()); }

std::string webgpu_device_name() {
  std::lock_guard<std::mutex> lock(g_text_mutex);
  return "GPU " + g_name + " (WebGPU)";
}

std::string webgpu_status() {
  std::lock_guard<std::mutex> lock(g_text_mutex);
  return g_status;
}

McResult run_mc_webgpu(const McInputs& in, const std::vector<float>& boot, uint64_t seed, uint64_t draws,
                       int n_keep) {
  if (draws == 0 || draws > 0xFFFFFFFFull) throw std::invalid_argument("run_mc_webgpu: 1 to 2^32 - 1 draws per call");
  if (in.n_boot < 1 || static_cast<size_t>(in.n_boot) * kBootStride > boot.size())
    throw std::invalid_argument("run_mc_webgpu: fewer bootstrap replicates than n_boot");
  if (in.n_years > kMaxYears || in.n_states > kMaxStates || in.n_features > kMaxFeatures)
    throw std::invalid_argument("run_mc_webgpu: inputs out of range");
  McResult r;
  r.draws = draws;
  r.n_years = in.n_years;
  r.device = webgpu_device_name();
  Request req;
  req.in = &in;
  req.boot = &boot;
  req.seed = seed;
  req.draws = static_cast<uint32_t>(draws);
  req.n_keep = static_cast<uint32_t>(std::clamp<uint64_t>(static_cast<uint64_t>(std::max(n_keep, 0)), 0, draws));
  req.result = &r;
  run_on_main_thread(req);
  return r;
}

std::vector<U32x4> webgpu_philox(const std::vector<std::array<uint32_t, 6>>& items) {
  if (items.empty() || items.size() > 65535ull * kWorkgroup)
    throw std::invalid_argument("webgpu_philox: 1 to 4,194,240 items");
  std::vector<U32x4> bits;
  Request req;
  req.items = &items;
  req.bits = &bits;
  run_on_main_thread(req);
  return bits;
}

}  // namespace e47
