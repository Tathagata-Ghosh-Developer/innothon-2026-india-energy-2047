// energy2047: runs the full pipeline and writes results to <repo_root>/outputs/.
//
// Usage: energy2047 [repo_root] [draws]
#include <omp.h>

#include <cstdio>
#include <filesystem>
#include <string>

#include "core/csv.hpp"
#include "core/stats.hpp"
#include "pipeline/pipeline.hpp"
#include "pipeline/presets.hpp"

namespace fs = std::filesystem;
using namespace e47;

namespace {

void write_forecast(const fs::path& out, const Pipeline& pl, const ForecastResult& f) {
  CsvWriter w((out / "forecast_national_regional.csv").string());
  w.field("series").field("year").field("p05").field("p10").field("p25").field("p50").field("p75").field("p90");
  w.field("p95").field("mean").field("central_fe_quad").field("central_fe_gompertz").field("central_damped");
  w.endrow();
  for (const auto& h : pl.fitted().history) {
    w.field("National (observed)").field(h.year);
    for (int k = 0; k < 8; ++k) w.field(h.gen_twh);
    w.field("").field("").field("");
    w.endrow();
  }
  for (int s = 0; s < kSeries; ++s)
    for (size_t t = 0; t < f.years.size(); ++t) {
      w.field(s == 0 ? std::string("National") : std::string(region_name(s - 1))).field(f.years[t]);
      for (int k = 0; k < 8; ++k) w.field(f.quant.q[s][t][k], 7);
      if (s == 0)
        w.field(f.central[0][t], 7).field(f.central[1][t], 7).field(f.central[2][t], 7);
      else
        w.field("").field("").field("");
      w.endrow();
    }
}

void write_strategy(const fs::path& out, const AllocationInputs& in, const StrategyResult& st) {
  CsvWriter w((out / "strategy_2047.csv").string());
  w.field("method").field("region").field("source").field("generation_twh").field("share_of_region").field(
      "base_2024_twh");
  w.endrow();
  for (int m = 0; m < 2; ++m) {
    const AllocationResult& a = m == 0 ? st.least_cost : st.score;
    for (int r = 0; r < kRegions; ++r) {
      double prod = 0;
      for (int s = 0; s < kTechs; ++s) prod += a.gen[r][s];
      for (int s = 0; s < kTechs; ++s) {
        w.field(m == 0 ? "least_cost" : "resource_score").field(region_name(r)).field(tech_key(s));
        w.field(a.gen[r][s], 6).field(prod > 0 ? a.gen[r][s] / prod : 0.0, 4).field(in.base_twh[r][s], 6);
        w.endrow();
      }
    }
  }
  {
    CsvWriter c((out / "corridors_2047.csv").string());
    c.field("corridor").field("from").field("to").field("flow_twh").field("capability_twh").field("loss");
    c.endrow();
    for (size_t k = 0; k < st.least_cost.corridor_flow.size(); ++k) {
      const Corridor& cr = in.corridors[k];
      const double f = st.least_cost.corridor_flow[k];
      const double cap = cr.mw_2032 * st.policy.transfer_scale * st.policy.transfer_util * 8.76e-3;
      c.field(cr.name).field(region_name(f >= 0 ? cr.a : cr.b)).field(region_name(f >= 0 ? cr.b : cr.a));
      c.field(std::abs(f), 6).field(cap, 6).field(cr.loss, 4);
      c.endrow();
    }
  }
  CsvWriter b((out / "strategy_2047_regions.csv").string());
  b.field("method").field("region").field("requirement_twh").field("production_twh").field("net_export_twh");
  b.field("unserved_twh").field("marginal_cost_usd_mwh");
  b.endrow();
  for (int m = 0; m < 2; ++m) {
    const AllocationResult& a = m == 0 ? st.least_cost : st.score;
    for (int r = 0; r < kRegions; ++r) {
      double prod = 0;
      for (int s = 0; s < kTechs; ++s) prod += a.gen[r][s];
      b.field(m == 0 ? "least_cost" : "resource_score").field(region_name(r)).field(st.requirement[r], 6);
      b.field(prod, 6).field(a.net_export[r], 6).field(a.unserved[r], 6);
      b.field(m == 0 ? a.marginal_cost[r] : 0.0, 5);
      b.endrow();
    }
  }
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const fs::path root = argc > 1 ? fs::path(argv[1]) : fs::current_path();
    const fs::path out = root / "outputs";
    fs::create_directories(out);

    Timer t_init;
    const Pipeline pl(root.string());
    const Fitted& fit = pl.fitted();
    const DriverData& drv = pl.drivers();
    std::printf("setup %.0f ms (bootstrap %.0f ms on %d threads)\n", t_init.ms(), fit.bootstrap_ms,
                omp_get_max_threads());
    std::printf("panel %zu countries, %zu obs | FE-Quad R2w %.3f | FE-Gompertz R2w %.3f\n",
                fit.panel.countries.size(), fit.panel.n_obs(), fit.quad_stats.r2_within, fit.gomp_stats.r2_within);
    std::printf("weights (inverse 2010-origin MAPE): quad %.2f gomp %.2f damped %.2f\n", fit.backtest_weights[0],
                fit.backtest_weights[1], fit.backtest_weights[2]);
    std::printf("damped trend: alpha %.3f beta %.3f phi %.3f sigma %.4f level %.3f trend %.4f\n", fit.damped.alpha,
                fit.damped.beta, fit.damped.phi, fit.damped.sigma, fit.damped.level, fit.damped.trend);
    std::printf("GDP pc growth %d-%d: mean %.4f sd %.4f rho %.2f | convergence lambda %.4f (se %.4f)\n",
                drv.g_hist_from, drv.g_hist_to, drv.g_hist_mean, drv.g_hist_sd, drv.g_hist_rho,
                fit.convergence.lambda, fit.convergence.lambda_se);
    std::printf("states %zu; WPP medium %d %.0f M -> 2047 %.0f M; urban (WUP) %.3f -> %.3f\n", drv.states.size(),
                drv.base_year, drv.wpp_med.front() / 1e6, drv.wpp_med.back() / 1e6, drv.wup_base, drv.wup.back());

    ScenarioControls c;
    if (argc > 2) c.draws = std::stoull(argv[2]);
    // Timing table: CPU 1 thread, CPU all threads, GPU (same draws, same seed).
    {
      const McInputs in = [&] {
        McInputs x = pl.build_inputs(c);
        calibrate_hist_ranges(x, fit.boot, c.seed);
        return x;
      }();
      const uint64_t bench = 1u << 20;
      CsvWriter tw((out / "timings.csv").string());
      tw.field("device").field("draws").field("ms").field("draws_per_second");
      tw.endrow();
      const int nt = omp_get_max_threads();
      omp_set_num_threads(1);
      const McResult c1 = run_mc_cpu(in, fit.boot, c.seed, bench / 16, 0, true);
      omp_set_num_threads(nt);
      const McResult cn = run_mc_cpu(in, fit.boot, c.seed, bench, 0, true);
      const double c1_ms = c1.ms * 16;  // scaled to the same draw count
      tw.field("CPU 1 thread (float), scaled from 1/16 run").field(static_cast<long long>(bench)).field(c1_ms).field(
          bench / c1_ms * 1e3);
      tw.endrow();
      tw.field(cn.device).field(static_cast<long long>(bench)).field(cn.ms).field(bench / cn.ms * 1e3);
      tw.endrow();
      std::printf("Monte Carlo %llu draws: CPU 1 thread %.0f ms | %s %.0f ms", static_cast<unsigned long long>(bench),
                  c1_ms, cn.device.c_str(), cn.ms);
      if (cuda_available()) {
        run_mc_gpu(in, fit.boot, c.seed, 1024, 0);  // warm-up (context creation)
        const McResult g = run_mc_gpu(in, fit.boot, c.seed, bench, 0);
        tw.field(g.device).field(static_cast<long long>(bench)).field(g.ms).field(bench / g.ms * 1e3);
        tw.endrow();
        std::printf(" | %s %.1f ms (%.0fx vs all CPU threads)", g.device.c_str(), g.ms, cn.ms / g.ms);
      }
      std::printf("\n");
    }

    Timer t_mc;
    const ForecastResult f = pl.forecast(c);
    std::printf("forecast: %llu draws on %s, kernel %.1f ms, total %.0f ms\n",
                static_cast<unsigned long long>(f.mc.draws), f.mc.device.c_str(), f.mc.ms, t_mc.ms());
    const size_t last = f.years.size() - 1;
    for (int s = 0; s < kSeries; ++s) {
      const auto& q = f.quant.q[s][last];
      std::printf("  %-14s %d: P5 %6.0f  P25 %6.0f  P50 %6.0f  P75 %6.0f  P95 %6.0f TWh\n",
                  s == 0 ? "National" : region_name(s - 1), f.years[last], q[0], q[2], q[3], q[4], q[6]);
    }
    std::printf("  central paths 2047: FE-Quad %.0f, FE-Gompertz %.0f, damped %.0f TWh\n", f.central[0][last],
                f.central[1][last], f.central[2][last]);
    write_forecast(out, pl, f);

    const PolicySettings p;
    const StrategyResult st = pl.strategy(f, c, p);
    const AllocationResult& lc = st.least_cost;
    std::printf("strategy (least cost): %s | non-fossil %.1f%% | CO2 %.0f Mt | cost %.0f bn USD/yr\n",
                lc.status.c_str(), 100 * lc.nonfossil_share(), lc.co2_mt, lc.cost_busd);
    std::printf("  national mix:");
    for (int s = 0; s < kTechs; ++s) std::printf(" %s %.1f%%", tech_key(s), 100 * lc.share(s));
    std::printf("\n");
    for (int r = 0; r < kRegions; ++r)
      std::printf("  %-14s requirement %6.0f  net export LC %+6.0f  score %+6.0f TWh\n", region_name(r),
                  st.requirement[r], lc.net_export[r], st.score.net_export[r]);
    write_strategy(out, pl.allocation_inputs(), st);
    {
      CsvWriter w((out / "tornado.csv").string());
      w.field("setting").field("low").field("high").field("co2_low_mt").field("co2_high_mt").field("coal_share_low");
      w.field("coal_share_high").field("nonfossil_low").field("nonfossil_high").field("cost_low_busd").field(
          "cost_high_busd");
      w.field("unserved_low_twh").field("unserved_high_twh");
      w.endrow();
      for (const auto& t : st.tornado) {
        w.field(t.name).field(t.low_label).field(t.high_label);
        auto put = [&](const AllocationResult& a, double v) { a.ok ? w.field(v, 6) : w.field("infeasible"); };
        put(t.low, t.low.co2_mt);
        put(t.high, t.high.co2_mt);
        put(t.low, t.low.share(kCoal));
        put(t.high, t.high.share(kCoal));
        put(t.low, t.low.nonfossil_share());
        put(t.high, t.high.nonfossil_share());
        put(t.low, t.low.cost_busd);
        put(t.high, t.high.cost_busd);
        auto unserved = [](const AllocationResult& a) {
          double u = 0;
          for (double x : a.unserved) u += x;
          return u;
        };
        put(t.low, unserved(t.low));
        put(t.high, unserved(t.high));
        w.endrow();
      }
    }
    {
      CsvWriter w((out / "backtest.csv").string());
      w.field("origin").field("year").field("actual_twh").field("fe_quad_twh").field("fe_gompertz_twh").field(
          "damped_twh");
      w.endrow();
      for (const auto& b : fit.backtests)
        for (const auto& p : b.points) {
          w.field(b.origin).field(p.year).field(p.actual_twh).field(p.fe_quad_twh).field(p.fe_gompertz_twh).field(
              p.damped_twh);
          w.endrow();
        }
    }
    {
      // Preset matrix: every scenario preset against every policy preset.
      CsvWriter w((out / "presets.csv").string());
      for (const char* h : {"scenario", "policy", "status", "national_p05_2047", "national_p50_2047", "national_p95_2047",
                            "nonfossil_share", "coal_share", "solar_share", "wind_share", "hydro_share", "nuclear_share",
                            "co2_mt", "cost_busd", "unserved_twh", "eastern_net_export_twh", "gas_share",
                            "offshore_wind_share", "bioenergy_share", "firmed_re_share", "unserved_at_p90_twh",
                            "cost_at_p90_busd"})
        w.field(h);
      w.endrow();
      std::printf("preset matrix (2047 median TWh | non-fossil | coal | CO2 Mt | unserved TWh):\n");
      for (const auto& sp : scenario_presets()) {
        ScenarioControls sc = c;
        sc = ScenarioControls{};
        sc.draws = c.draws;
        sp.apply(sc);
        const ForecastResult pf = pl.forecast(sc);
        const auto& q = pf.quant.q[0][pf.years.size() - 1];
        for (const auto& pp : policy_presets()) {
          PolicySettings ps;
          pp.apply(ps);
          const StrategyResult r = pl.strategy(pf, sc, ps);
          const AllocationResult& a = r.least_cost;
          double unserved = 0;
          for (double u : a.unserved) unserved += u;
          w.field(sp.name).field(pp.name).field(a.ok ? "optimal" : a.status);
          w.field(q[0], 6).field(q[3], 6).field(q[6], 6);
          w.field(a.nonfossil_share(), 4).field(a.share(kCoal), 4).field(a.share(kSolar), 4).field(a.share(kWind), 4);
          w.field(a.share(kHydro), 4).field(a.share(kNuclear), 4).field(a.co2_mt, 6).field(a.cost_busd, 5);
          w.field(unserved, 4).field(a.net_export[3], 5);
          w.field(a.share(kGas), 4).field(a.share(kOffshore), 4).field(a.share(kBio), 4).field(a.share(kFirmRE), 4);
          // Stress test: the same policy planned for the P90 requirement.
          ScenarioControls s90 = sc;
          s90.requirement_quantile = 0.9;
          const StrategyResult r90 = pl.strategy(pf, s90, ps);
          double u90 = 0;
          for (double u : r90.least_cost.unserved) u90 += u;
          if (r90.least_cost.ok)
            w.field(u90, 4).field(r90.least_cost.cost_busd, 5);
          else
            w.field("infeasible").field("infeasible");
          w.endrow();
          std::printf("  %-12s x %-16s %6.0f | %5.1f%% | %5.1f%% | %5.0f | %6.1f | P90 unmet %6.1f %s\n",
                      sp.name.c_str(), pp.name.c_str(), q[3], 100 * a.nonfossil_share(), 100 * a.share(kCoal),
                      a.co2_mt, unserved, r90.least_cost.ok ? u90 : -1.0, a.ok ? "" : a.status.c_str());
        }
      }
    }
    std::printf("outputs written to %s\n", out.string().c_str());
    return 0;
  } catch (const std::exception& e) {
    std::fprintf(stderr, "error: %s\n", e.what());
    return 1;
  }
}
