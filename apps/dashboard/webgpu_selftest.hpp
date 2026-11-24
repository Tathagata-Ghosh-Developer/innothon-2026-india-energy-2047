#pragma once
// In-browser parity check of the WebGPU Monte Carlo (web build with E47_WEBGPU), run by
// opening the dashboard as /app/?selftest=1 (or ?selftest=2 to add timings at 2^22 draws).
#include "pipeline/pipeline.hpp"

namespace e47 {

// Runs the WebGPU kernel and the CPU float path on the same inputs and seed and compares
// them: raw Philox outputs bit for bit, every kept draw's model and replicate, per-year
// P5 / P50 / P95 of every series, and the histograms. Prints the result to the console
// and sets data-webgpu-selftest="pass" | "fail" | "skipped" | "error" on <html>, with the
// numbers in window.e47SelfTest. Call from a worker thread (it waits for the GPU).
void run_webgpu_selftest(const Pipeline& pl, const ScenarioControls& sc, bool timings);

}  // namespace e47
