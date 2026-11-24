// Parity test: the CPU (double), CPU (float) and GPU (float) Monte Carlo paths
// must agree. Uses a synthetic, self-contained input set (test fixture only).
// Also checks the demand-feature draws of the joint feature scenario: the three paths
// agree with them, each draw's add-on equals the host-side sum of its feature draws, and
// constant features give exactly the ensemble of the same constant add-on.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

#include "forecast/mc.hpp"

using namespace e47;

namespace {

McInputs fixture(std::vector<float>& boot) {
  McInputs in{};
  in.base_year = 2025;
  in.n_years = 22;
  in.ln_e0 = std::log(1400.0f);
  in.y0 = 10.0f;
  in.u0 = 0.36f;
  in.gen0_twh = 2000.0f;
  for (int t = 0; t < in.n_years; ++t) {
    in.pop_med[t] = 1460.0f + 9.0f * t;
    in.pop_sd[t] = 3.0f + 1.5f * t;
    in.urb_med[t] = 0.36f + 0.006f * t;
    in.urb_sd[t] = 0.002f * t;
    in.g_mean[t] = 0.05f - 0.0006f * t;
  }
  in.g_level_sd = 0.01f;
  in.g_shock_sd = 0.02f;
  in.g_rho = 0.3f;
  in.w_model[0] = 0.4f;
  in.w_model[1] = 0.4f;
  in.w_model[2] = 0.2f;
  in.dt_level = std::log(2000.0f);
  in.dt_trend = 0.05f;
  in.dt_phi = 0.95f;
  in.dt_alpha = 0.8f;
  in.dt_beta = 0.2f;
  in.dt_sigma = 0.02f;
  in.addon_lo = 0.0f;
  in.addon_hi = 100.0f;
  in.n_states = 10;
  float total = 0;
  for (int s = 0; s < in.n_states; ++s) total += 1.0f + s;
  for (int s = 0; s < in.n_states; ++s) {
    in.state_region[s] = s % kRegions;
    for (int t = 0; t < in.n_years; ++t) in.state_pop_share[s][t] = (1.0f + s) / total;
    in.state_ln_rel_income[s] = -0.5f + 0.1f * s;
  }
  in.conv_mean = 0.01f;
  in.conv_sd = 0.005f;
  const float reps[4][kBootStride] = {{0.44f, -0.14f, 2.6f, 0.02f, 2.4f, 0.22f, 2.5f, 0.03f},
                                      {0.40f, -0.12f, 2.4f, 0.03f, 2.2f, 0.20f, 2.3f, 0.02f},
                                      {0.48f, -0.15f, 2.8f, 0.01f, 2.6f, 0.25f, 2.7f, 0.04f},
                                      {0.42f, -0.13f, 2.5f, 0.02f, 2.3f, 0.21f, 2.4f, 0.03f}};
  in.n_boot = 4;
  boot.assign(&reps[0][0], &reps[0][0] + 4 * kBootStride);
  return in;
}

int compare(const char* label, const McInputs& in, const McResult& a, const McResult& b, double tol) {
  int fails = 0;
  for (int s = 0; s < kSeries; ++s)
    for (int t : {0, 10, in.n_years - 1})
      for (double q : {0.05, 0.5, 0.95}) {
        const double x = a.quantile(in, s, t, q), y = b.quantile(in, s, t, q);
        const double rel = std::abs(x - y) / std::max(1e-9, std::abs(x));
        if (rel > tol) {
          std::printf("  %s mismatch series %d year %d q%.2f: %.4f vs %.4f (rel %.2e)\n", label, s, t, q, x, y, rel);
          ++fails;
        }
      }
  std::printf("%s: %s\n", label, fails ? "FAIL" : "ok");
  return fails;
}

// Each kept draw's add-on (keep[6]) against the host-side sum of its feature draws.
int compare_feature_keep(const char* label, const McInputs& in, const McResult& r, uint64_t seed, int n_keep) {
  double worst = 0;
  for (int i = 0; i < n_keep; ++i) {
    const double host = feature_draw_total<double>(in, DrawRng(seed, static_cast<uint64_t>(i)));
    const double kernel = r.keep[static_cast<size_t>(i) * kKeepFeatures + 6];
    worst = std::max(worst, std::abs(kernel - host) / (1.0 + std::abs(host)));
  }
  const bool ok = n_keep > 0 && worst < 1e-4;
  std::printf("%s: %d draws, worst relative gap %.2e: %s\n", label, n_keep, worst, ok ? "ok" : "FAIL");
  return ok ? 0 : 1;
}

}  // namespace

int run();

// The browser's WebGPU kernel (src/forecast/mc_webgpu.wgsl) is a second copy of the
// per-draw physics in mc.hpp and of the Philox generator in philox.hpp, checked only by
// the in-browser self-test (/app/?selftest=1), which CI cannot run. This tripwire records,
// in a line of the WGSL file, a hash of the two C++ files as they were when the port was
// last checked; any later change to either file fails the test until the WGSL port is
// updated, the self-test passes again and the line is updated with the hashes printed here.
namespace {

uint64_t fnv1a64_file(const std::string& path) {
  std::FILE* f = std::fopen(path.c_str(), "rb");
  if (!f) throw std::runtime_error("cannot open " + path);
  uint64_t h = 1469598103934665603ull;
  for (int c; (c = std::fgetc(f)) != EOF;) {
    if (c == '\r') continue;  // the same hash for LF and CRLF checkouts
    h ^= static_cast<unsigned char>(c);
    h *= 1099511628211ull;
  }
  std::fclose(f);
  return h;
}

int wgsl_sync_check(const std::string& root) {
  const uint64_t mc = fnv1a64_file(root + "/src/forecast/mc.hpp");
  const uint64_t philox = fnv1a64_file(root + "/src/core/philox.hpp");
  char want[160];
  std::snprintf(want, sizeof want, "// Synced with mc.hpp fnv1a64 %016llx and philox.hpp fnv1a64 %016llx",
                static_cast<unsigned long long>(mc), static_cast<unsigned long long>(philox));
  std::FILE* f = std::fopen((root + "/src/forecast/mc_webgpu.wgsl").c_str(), "rb");
  if (!f) throw std::runtime_error("cannot open src/forecast/mc_webgpu.wgsl");
  std::string text;
  char buf[4096];
  for (size_t n; (n = std::fread(buf, 1, sizeof buf, f)) > 0;) text.append(buf, n);
  std::fclose(f);
  if (text.find(want) != std::string::npos) {
    std::printf("wgsl port in sync with mc.hpp and philox.hpp: ok\n");
    return 0;
  }
  std::printf("FAIL wgsl port: src/forecast/mc.hpp or src/core/philox.hpp changed since\n"
              "  src/forecast/mc_webgpu.wgsl was last checked. Port the change to the WGSL file,\n"
              "  open /app/?selftest=1 in a WebGPU browser until it reports pass, then set the\n"
              "  file's sync line to:\n  %s\n",
              want);
  return 1;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    int fails = run();
    if (argc > 1) fails += wgsl_sync_check(argv[1]);
    return fails == 0 ? 0 : 1;
  } catch (const std::exception& e) {
    std::printf("error: %s\n", e.what());
    return 2;
  }
}

int run() {
  std::vector<float> boot;
  McInputs in = fixture(boot);
  const uint64_t seed = 20470815;
  std::printf("calibrating histogram ranges\n");
  std::fflush(stdout);
  calibrate_hist_ranges(in, boot, seed, 4096);
  const uint64_t draws = 1u << 20;

  std::printf("cpu double\n");
  std::fflush(stdout);
  const McResult cd = run_mc_cpu(in, boot, seed, draws, 0, false);
  std::printf("cpu float\n");
  std::fflush(stdout);
  const McResult cf = run_mc_cpu(in, boot, seed, draws, 0, true);
  int fails = compare("cpu double vs cpu float", in, cd, cf, 2e-3);
  if (cuda_available()) {
    const McResult gf = run_mc_gpu(in, boot, seed, draws, 0);
    fails += compare("cpu float vs gpu float", in, cf, gf, 2e-3);
    std::printf("timings for %llu draws: cpu double %.1f ms, cpu float %.1f ms, gpu %.1f ms\n",
                static_cast<unsigned long long>(draws), cd.ms, cf.ms, gf.ms);
  } else {
    std::printf("no CUDA device: GPU parity skipped\n");
  }

  // India-only core layout: both parameter slots in quadratic form (slot 1 reads p[4..7]).
  {
    std::vector<float> qboot = boot;
    for (int k = 0; k < in.n_boot; ++k) {
      float* p = qboot.data() + static_cast<size_t>(k) * kBootStride;
      const float beta = 0.80f + 0.03f * k, gamma = 0.5f + 0.25f * k;
      p[0] = beta - gamma * 0.09f;
      p[1] = 0.0f;
      p[2] = gamma;
      p[3] = 0.0f;
      p[4] = beta;
      p[5] = 0.0f;
      p[6] = 0.0f;
      p[7] = 0.0f;
    }
    McInputs iq = in;
    iq.quad_only = 1;
    iq.w_model[0] = 0.5f;
    iq.w_model[1] = 0.5f;
    iq.w_model[2] = 0.0f;
    // Model-error path (AR(1) residual from the base year).
    iq.resid0 = 0.07f;
    iq.resid_rho = 0.5f;
    iq.resid_innov_sd = 0.032f;
    calibrate_hist_ranges(iq, qboot, seed, 4096);
    const McResult qd = run_mc_cpu(iq, qboot, seed, draws, 0, false);
    const McResult qf = run_mc_cpu(iq, qboot, seed, draws, 0, true);
    fails += compare("india layout: cpu double vs cpu float", iq, qd, qf, 2e-3);
    if (cuda_available()) {
      const McResult qg = run_mc_gpu(iq, qboot, seed, draws, 0);
      fails += compare("india layout: cpu float vs gpu float", iq, qf, qg, 2e-3);
    }
  }

  // Demand-feature draws: triangular and uniform, both signs, more than one step of four.
  {
    McInputs fi = in;
    fi.addon_lo = 0.0f;
    fi.addon_hi = 0.0f;
    pack_features(fi, {{kFeatureTriangular, 1, 100, 150, 400},
                       {kFeatureUniform, -1, 0, 0, 250},
                       {kFeatureTriangular, 1, 50, 50, 80},
                       {kFeatureTriangular, -1, 10, 30, 30},
                       {kFeatureTriangular, 1, 20, 40, 60}});
    calibrate_hist_ranges(fi, boot, seed, 4096);
    const int n_keep = 4096;
    const McResult fd = run_mc_cpu(fi, boot, seed, draws, n_keep, false);
    const McResult ff = run_mc_cpu(fi, boot, seed, draws, n_keep, true);
    fails += compare("features: cpu double vs cpu float", fi, fd, ff, 2e-3);
    fails += compare_feature_keep("features: cpu double add-on = host feature sum", fi, fd, seed, n_keep);
    fails += compare_feature_keep("features: cpu float add-on = host feature sum", fi, ff, seed, n_keep);
    if (cuda_available()) {
      const McResult fg = run_mc_gpu(fi, boot, seed, draws, n_keep);
      fails += compare("features: cpu float vs gpu float", fi, ff, fg, 2e-3);
      fails += compare_feature_keep("features: gpu float add-on = host feature sum", fi, fg, seed, n_keep);
    }

    // Constant features (lo = mode = hi) against the same constant add-on: the same ensemble
    // bin for bin, so the feature stream leaves every other random input of a draw unchanged.
    McInputs a = in, b = in;
    a.addon_lo = 120.0f;
    a.addon_hi = 120.0f;
    calibrate_hist_ranges(a, boot, seed, 4096);
    b.addon_lo = 0.0f;
    b.addon_hi = 0.0f;
    pack_features(b, {{kFeatureTriangular, 1, 200, 200, 200}, {kFeatureUniform, -1, 80, 80, 80}});
    for (int s = 0; s < kSeries; ++s)
      for (int t = 0; t < kMaxYears; ++t) {
        b.hist_lo[s][t] = a.hist_lo[s][t];
        b.hist_hi[s][t] = a.hist_hi[s][t];
      }
    const McResult ra = run_mc_cpu(a, boot, seed, draws, 0, true);
    const McResult rb = run_mc_cpu(b, boot, seed, draws, 0, true);
    const bool same = ra.hist == rb.hist;
    std::printf("features: constant features = the same constant add-on, bin for bin: %s\n", same ? "ok" : "FAIL");
    fails += same ? 0 : 1;
  }
  return fails == 0 ? 0 : 1;
}
