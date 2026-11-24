// In-browser parity check of the WebGPU Monte Carlo against the CPU float path
// (see webgpu_selftest.hpp). Pass: identical Philox outputs, identical model and
// bootstrap replicate in every kept draw, and per-year quantiles within 0.1% (P50) and
// 0.5% (P5, P95) at the same draw count.
#include "webgpu_selftest.hpp"

#include <emscripten/emscripten.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <exception>
#include <string>
#include <vector>

#include "core/philox.hpp"
#include "forecast/mc_webgpu.hpp"

namespace e47 {

namespace {

constexpr double kTolP50 = 1e-3;
constexpr double kTolTails = 5e-3;
constexpr int kKeep = 65536;

struct Case {
  std::string name;
  McInputs in{};
  std::vector<float> boot;
  uint64_t draws = 0;
};

struct CaseResult {
  std::string name;
  uint64_t draws = 0;
  double q_rel[3] = {0, 0, 0};  // largest relative difference at P5, P50, P95 (every series and year)
  double ks = 0;                // largest |CDF difference| of a series-year histogram
  double moved = 0;             // largest share of draws in a different bin
  uint64_t kept = 0;            // kept draws compared
  uint64_t discrete_diff = 0;   // kept draws whose model or bootstrap replicate differ
  double keep_abs = 0;          // largest |difference| of z0, z1, z2, lambda and the add-on
  double gen_rel = 0;           // largest relative difference of the final national value
  double cpu_ms = 0, gpu_ms = 0;
  bool pass = false;
};

// Synthetic inputs with the cross-country layout (Gompertz slot, quad_only = 0), which the
// India-only dashboard core does not reach; the fixture of tests/test_mc_parity.cpp.
Case gompertz_case(uint64_t seed) {
  Case c;
  c.name = "synthetic cross-country layout (Gompertz slot)";
  McInputs& in = c.in;
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
  c.boot.assign(&reps[0][0], &reps[0][0] + 4 * kBootStride);
  calibrate_hist_ranges(in, c.boot, seed);
  return c;
}

Case pipeline_case(const Pipeline& pl, const ScenarioControls& sc, std::string name, uint64_t draws) {
  Case c;
  c.name = std::move(name);
  c.in = pl.build_inputs(sc);
  c.boot = pl.boot_for(sc);
  c.draws = draws;
  calibrate_hist_ranges(c.in, c.boot, sc.seed);
  return c;
}

void compare(const McInputs& in, const McResult& cpu, const McResult& gpu, CaseResult& r) {
  const int series = in.n_states > 0 ? kSeries : 1;
  const double qs[3] = {0.05, 0.50, 0.95};
  for (int s = 0; s < series; ++s)
    for (int t = 0; t < in.n_years; ++t) {
      for (int k = 0; k < 3; ++k) {
        const double x = cpu.quantile(in, s, t, qs[k]), y = gpu.quantile(in, s, t, qs[k]);
        r.q_rel[k] = std::max(r.q_rel[k], std::abs(x - y) / std::max(1e-9, std::abs(x)));
      }
      const uint64_t* a = cpu.bins(s, t);
      const uint64_t* b = gpu.bins(s, t);
      double ca = 0, cb = 0, diff = 0, ks = 0;
      for (int i = 0; i < kBins; ++i) {
        ca += static_cast<double>(a[i]);
        cb += static_cast<double>(b[i]);
        ks = std::max(ks, std::abs(ca - cb));
        diff += std::abs(static_cast<double>(a[i]) - static_cast<double>(b[i]));
      }
      const double n = static_cast<double>(cpu.draws);
      r.ks = std::max(r.ks, ks / n);
      r.moved = std::max(r.moved, diff / (2 * n));
    }
  r.kept = std::min(cpu.keep.size(), gpu.keep.size()) / kKeepFeatures;
  for (uint64_t i = 0; i < r.kept; ++i) {
    const float* a = cpu.keep.data() + i * kKeepFeatures;
    const float* b = gpu.keep.data() + i * kKeepFeatures;
    if (a[4] != b[4] || a[5] != b[5]) ++r.discrete_diff;
    for (int j : {0, 1, 2, 3, 6}) r.keep_abs = std::max(r.keep_abs, static_cast<double>(std::abs(a[j] - b[j])));
    r.gen_rel = std::max(r.gen_rel, std::abs(static_cast<double>(a[7]) - b[7]) / std::max(1e-9, std::abs(double(a[7]))));
  }
  r.pass = r.q_rel[1] <= kTolP50 && r.q_rel[0] <= kTolTails && r.q_rel[2] <= kTolTails && r.discrete_diff == 0 &&
           r.kept > 0;
}

CaseResult run_case(const Case& c, uint64_t seed) {
  CaseResult r;
  r.name = c.name;
  r.draws = c.draws;
  const McResult cpu = run_mc_cpu(c.in, c.boot, seed, c.draws, kKeep, true);
  const McResult gpu = run_mc_webgpu(c.in, c.boot, seed, c.draws, kKeep);
  r.cpu_ms = cpu.ms;
  r.gpu_ms = gpu.ms;
  compare(c.in, cpu, gpu, r);
  std::printf(
      "selftest %-48s %9llu draws: max rel diff P5 %.2e, P50 %.2e, P95 %.2e; histogram KS %.2e, moved %.2e; "
      "kept %llu draws, model/replicate differ in %llu, max |diff| %.2e, final value rel %.2e; "
      "CPU float %.0f ms, WebGPU %.0f ms: %s\n",
      c.name.c_str(), static_cast<unsigned long long>(c.draws), r.q_rel[0], r.q_rel[1], r.q_rel[2], r.ks, r.moved,
      static_cast<unsigned long long>(r.kept), static_cast<unsigned long long>(r.discrete_diff), r.keep_abs, r.gen_rel,
      r.cpu_ms, r.gpu_ms, r.pass ? "ok" : "FAIL");
  return r;
}

// Raw Philox outputs: the kernel's counters and keys (draws, steps 0..25, every stream,
// both seeds), plus scattered and extreme counters and keys.
uint64_t philox_mismatches(uint64_t seed, uint64_t& checked) {
  std::vector<std::array<uint32_t, 6>> items;
  const uint64_t pick_seed = seed ^ 0xA5A5A5A5u;
  uint32_t lcg = 12345u;
  const auto next = [&lcg] { return lcg = lcg * 1664525u + 1013904223u; };
  for (uint32_t i = 0; i < 1u << 18; ++i) {
    const uint64_t key = (i & 1) ? pick_seed : seed;
    std::array<uint32_t, 6> it{i >> 1, 0u, i % 26u, i % 7u, static_cast<uint32_t>(key), static_cast<uint32_t>(key >> 32)};
    if (i % 4 == 3) it = {next(), next(), next(), next(), next(), next()};
    items.push_back(it);
  }
  items.push_back({0u, 0u, 0u, 0u, 0u, 0u});
  items.push_back({0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu});
  const std::vector<U32x4> gpu = webgpu_philox(items);
  uint64_t bad = 0;
  for (size_t i = 0; i < items.size(); ++i) {
    const auto& it = items[i];
    const U32x4 c = philox4x32_10(U32x4{it[0], it[1], it[2], it[3]}, it[4], it[5]);
    const U32x4& g = gpu[i];
    if (c.x != g.x || c.y != g.y || c.z != g.z || c.w != g.w) ++bad;
  }
  checked = items.size();
  return bad;
}

void publish(const char* verdict, const std::string& json) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdollar-in-identifier-extension"  // $0, $1: EM_ASM arguments
  MAIN_THREAD_EM_ASM(
      {
        document.documentElement.dataset.webgpuSelftest = UTF8ToString($0);
        window.e47SelfTest = JSON.parse(UTF8ToString($1));
      },
      verdict, json.c_str());
#pragma clang diagnostic pop
}

std::string quoted(const std::string& s) {
  std::string out = "\"";
  for (char ch : s) {
    if (ch == '"' || ch == '\\') out.push_back('\\');
    out.push_back(ch == '\n' ? ' ' : ch);
  }
  return out + "\"";
}

}  // namespace

void run_webgpu_selftest(const Pipeline& pl, const ScenarioControls& sc, bool timings) {
  if (webgpu_state() != WebGpuState::kReady) {
    const std::string why = webgpu_state() == WebGpuState::kOff ? "WebGPU not requested" : webgpu_status();
    std::printf("selftest skipped: %s\n", why.c_str());
    publish("skipped", "{\"verdict\":\"skipped\",\"reason\":" + quoted(why) + "}");
    return;
  }
  try {
    const uint64_t seed = sc.seed;
    std::printf("selftest: WebGPU %s against the CPU float path, seed %llu\n", webgpu_device_name().c_str(),
                static_cast<unsigned long long>(seed));
    uint64_t checked = 0;
    const uint64_t philox_bad = philox_mismatches(seed, checked);
    std::printf("selftest Philox 4x32-10: %llu outputs, %llu differ: %s\n", static_cast<unsigned long long>(checked),
                static_cast<unsigned long long>(philox_bad), philox_bad == 0 ? "ok" : "FAIL");

    std::vector<Case> cases;
    cases.push_back(pipeline_case(pl, sc, "dashboard settings", 1u << 20));
    {
      ScenarioControls c = sc;
      c.w_model = {0.4, 0.4, 0.2};
      cases.push_back(pipeline_case(pl, c, "three model slots (0.4 / 0.4 / 0.2)", 1u << 18));
    }
    {
      ScenarioControls c = sc;
      apply_feature_scenario(c, pl.demand_features(), Scenario::kJoint);
      cases.push_back(pipeline_case(pl, c, "joint demand-feature draws", 1u << 18));
    }
    cases.push_back(gompertz_case(seed));
    cases.back().draws = 1u << 18;

    bool pass = philox_bad == 0;
    std::string json = "{\"device\":" + quoted(webgpu_device_name()) + ",\"philox\":{\"checked\":" +
                       std::to_string(checked) + ",\"differ\":" + std::to_string(philox_bad) + "},\"cases\":[";
    char b[1024];
    for (size_t i = 0; i < cases.size(); ++i) {
      const CaseResult r = run_case(cases[i], seed);
      pass = pass && r.pass;
      std::snprintf(b, sizeof b,
                    "%s{\"name\":%s,\"draws\":%llu,\"p05\":%.3e,\"p50\":%.3e,\"p95\":%.3e,\"ks\":%.3e,\"moved\":%.3e,"
                    "\"kept\":%llu,\"discrete_diff\":%llu,\"keep_abs\":%.3e,\"gen_rel\":%.3e,\"cpu_ms\":%.1f,"
                    "\"gpu_ms\":%.1f,\"pass\":%s}",
                    i ? "," : "", quoted(r.name).c_str(), static_cast<unsigned long long>(r.draws), r.q_rel[0],
                    r.q_rel[1], r.q_rel[2], r.ks, r.moved, static_cast<unsigned long long>(r.kept),
                    static_cast<unsigned long long>(r.discrete_diff), r.keep_abs, r.gen_rel, r.cpu_ms, r.gpu_ms,
                    r.pass ? "true" : "false");
      json += b;
    }
    json += "]";

    if (timings) {
      // Ensemble time alone at the native default (2^22 draws) and above, dashboard settings.
      json += ",\"timings\":[";
      const Case& c = cases[0];
      int k = 0;
      for (uint64_t n : {1ull << 22, 1ull << 24}) {
        const McResult g = run_mc_webgpu(c.in, c.boot, seed, n, kKeep);
        double cpu_ms = -1;
        if (n == 1ull << 22) cpu_ms = run_mc_cpu(c.in, c.boot, seed, n, kKeep, true).ms;
        std::printf("selftest timing %llu draws: WebGPU %.0f ms, CPU float %s ms\n", static_cast<unsigned long long>(n),
                    g.ms, cpu_ms < 0 ? "(not run)" : std::to_string(static_cast<long long>(cpu_ms)).c_str());
        std::snprintf(b, sizeof b, "%s{\"draws\":%llu,\"gpu_ms\":%.1f,\"cpu_ms\":%.1f}", k++ ? "," : "",
                      static_cast<unsigned long long>(n), g.ms, cpu_ms);
        json += b;
      }
      json += "]";
    }
    const char* verdict = pass ? "pass" : "fail";
    json += ",\"verdict\":\"" + std::string(verdict) + "\"}";
    std::printf("selftest result: %s\n", verdict);
    publish(verdict, json);
  } catch (const std::exception& e) {
    std::printf("selftest error: %s\n", e.what());
    publish("error", "{\"verdict\":\"error\",\"reason\":" + quoted(e.what()) + "}");
  }
}

}  // namespace e47
