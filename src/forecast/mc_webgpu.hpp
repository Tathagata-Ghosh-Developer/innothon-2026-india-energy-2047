#pragma once
// Monte Carlo ensemble on the browser's GPU through WebGPU (web build only: E47_WEBGPU).
//
// The kernel, src/forecast/mc_webgpu.wgsl, is a port of simulate_draw<float>() and
// the histogram update of the CUDA driver; this file is the host side (C++ on the
// WebGPU C API of Emscripten's emdawnwebgpu port).
//
// WebGPU objects belong to the main browser thread. webgpu_init() runs there and
// returns at once; the adapter, device and pipelines arrive in callbacks. run_mc_webgpu()
// hands its work to the main thread and blocks the calling worker thread until the
// GPU has finished, so the page keeps drawing while the GPU computes. Any failure
// leaves the CPU path in charge (webgpu_state() becomes kFailed).
#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "../core/philox.hpp"
#include "mc.hpp"

namespace e47 {

enum class WebGpuState : int {
  kOff = 0,  // webgpu_init() not called
  kPending,  // adapter, device or pipelines still being created
  kReady,
  kFailed,   // no WebGPU, or an error: the CPU path is used
};

// Starts the WebGPU set-up. Main browser thread only; later calls do nothing.
void webgpu_init();
WebGpuState webgpu_state();
// "GPU <adapter> (WebGPU)" once ready; why WebGPU is not used once failed.
std::string webgpu_device_name();
std::string webgpu_status();

// The ensemble of run_mc_gpu() (mc.hpp) on the WebGPU device: draws 0 .. draws - 1 under
// `seed`, the first n_keep draws' features kept. At most 2^32 - 1 draws. Call from a
// worker thread, never the main browser thread. Throws on any WebGPU error (and marks
// WebGPU failed, so later runs use the CPU).
McResult run_mc_webgpu(const McInputs& in, const std::vector<float>& boot, uint64_t seed, uint64_t draws,
                       int n_keep = 0);

// Self-test: philox4x32_10(counter, key) on the GPU for each item
// {counter.x, counter.y, counter.z, counter.w, key0, key1}. Worker thread only; throws on error.
std::vector<U32x4> webgpu_philox(const std::vector<std::array<uint32_t, 6>>& items);

}  // namespace e47
