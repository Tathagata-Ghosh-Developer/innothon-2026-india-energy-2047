// CUDA driver for the Monte Carlo ensemble. The per-draw physics is the same
// simulate_draw() used on the CPU; this file only maps draws to threads and
// accumulates histograms.
#include <cuda_runtime.h>

#include <stdexcept>
#include <string>

#include "mc.hpp"

namespace e47 {

namespace {

__constant__ McInputs c_in;

void check(cudaError_t e, const char* what) {
  if (e != cudaSuccess) throw std::runtime_error(std::string(what) + ": " + cudaGetErrorString(e));
}

__global__ void mc_kernel(const float* __restrict__ boot, uint64_t seed, uint64_t draws, unsigned int* hist,
                          float* keep, int n_keep) {
  const uint64_t stride = static_cast<uint64_t>(gridDim.x) * blockDim.x;
  for (uint64_t i = static_cast<uint64_t>(blockIdx.x) * blockDim.x + threadIdx.x; i < draws; i += stride) {
    auto emit = [&](int s, int t, float v) {
      atomicAdd(&hist[(static_cast<size_t>(s) * kMaxYears + t) * kBins + hist_bin<float>(c_in, s, t, v)], 1u);
    };
    float* kp = i < static_cast<uint64_t>(n_keep) ? keep + i * kKeepFeatures : nullptr;
    simulate_draw<float>(c_in, boot, seed, i, emit, kp);
  }
}

}  // namespace

bool cuda_available() {
  int n = 0;
  return cudaGetDeviceCount(&n) == cudaSuccess && n > 0;
}

McResult run_mc_gpu(const McInputs& in, const std::vector<float>& boot, uint64_t seed, uint64_t draws, int n_keep) {
  constexpr size_t H = static_cast<size_t>(kSeries) * kMaxYears * kBins;
  if (draws > 0xFFFFFFFFull) throw std::runtime_error("run_mc_gpu: at most 2^32 - 1 draws per call");
  n_keep = static_cast<int>(draws < static_cast<uint64_t>(n_keep) ? draws : static_cast<uint64_t>(n_keep));

  cudaDeviceProp prop{};
  check(cudaGetDeviceProperties(&prop, 0), "cudaGetDeviceProperties");

  McResult r;
  r.draws = draws;
  r.n_years = in.n_years;
  r.device = std::string("GPU ") + prop.name + " (float)";

  float* d_boot = nullptr;
  unsigned int* d_hist = nullptr;
  float* d_keep = nullptr;
  check(cudaMemcpyToSymbol(c_in, &in, sizeof(McInputs)), "copy inputs");
  check(cudaMalloc(&d_boot, boot.size() * sizeof(float)), "malloc boot");
  check(cudaMemcpy(d_boot, boot.data(), boot.size() * sizeof(float), cudaMemcpyHostToDevice), "copy boot");
  check(cudaMalloc(&d_hist, H * sizeof(unsigned int)), "malloc hist");
  check(cudaMemset(d_hist, 0, H * sizeof(unsigned int)), "memset hist");
  if (n_keep > 0) check(cudaMalloc(&d_keep, static_cast<size_t>(n_keep) * kKeepFeatures * sizeof(float)), "malloc keep");

  constexpr int kBlock = 256;
  int per_sm = 0;
  check(cudaOccupancyMaxActiveBlocksPerMultiprocessor(&per_sm, mc_kernel, kBlock, 0), "occupancy");
  const int grid = prop.multiProcessorCount * (per_sm > 0 ? per_sm : 4);

  cudaEvent_t e0, e1;
  cudaEventCreate(&e0);
  cudaEventCreate(&e1);
  cudaEventRecord(e0);
  mc_kernel<<<grid, kBlock>>>(d_boot, seed, draws, d_hist, d_keep, n_keep);
  check(cudaGetLastError(), "launch");
  cudaEventRecord(e1);
  check(cudaEventSynchronize(e1), "kernel");
  float ms = 0;
  cudaEventElapsedTime(&ms, e0, e1);
  r.ms = ms;

  std::vector<unsigned int> h(H);
  check(cudaMemcpy(h.data(), d_hist, H * sizeof(unsigned int), cudaMemcpyDeviceToHost), "copy hist");
  r.hist.assign(h.begin(), h.end());
  if (n_keep > 0) {
    r.keep.resize(static_cast<size_t>(n_keep) * kKeepFeatures);
    check(cudaMemcpy(r.keep.data(), d_keep, r.keep.size() * sizeof(float), cudaMemcpyDeviceToHost), "copy keep");
  }
  cudaFree(d_boot);
  cudaFree(d_hist);
  cudaFree(d_keep);
  cudaEventDestroy(e0);
  cudaEventDestroy(e1);
  return r;
}

}  // namespace e47
