// energy2047: runs the full pipeline and writes results to <repo_root>/outputs/.
//
// Usage: energy2047 [repo_root] [draws]
#include <omp.h>

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <map>
#include <string>

#include "core/csv.hpp"
#include "core/stats.hpp"
#include "pipeline/pipeline.hpp"
#include "pipeline/presets.hpp"

namespace fs = std::filesystem;
using namespace e47;

namespace {

void write_forecast(const fs::path& out, const Pipeline& pl, const ForecastResult& f,
                    const std::string& file = "forecast_national_regional.csv") {
  CsvWriter w((out / file).string());
  w.field("series").field("year").field("p05").field("p10").field("p25").field("p50").field("p75").field("p90");
  w.field("p95").field("mean").field("central_model0").field("central_model1").field("central_model2");
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

// Storage and land back-calculated from the least-cost plan: Base and Robust policy
// at the median and P90 requirement, plus the Robust plan with the P90 levers
// (45% usable solar land, build rates 3.5x the record year), which meets the P90.
void write_storage_land(const fs::path& out, const Pipeline& pl, const ForecastResult& f, const ScenarioControls& c) {
  const StorageInputs& si = pl.storage_inputs();
  if (!si.ok) {
    std::printf("storage and land: skipped (%s)\n", si.status.c_str());
    return;
  }
  struct Case {
    const char* plan;
    const char* policy;
    double quantile;
    double solar_land, build_accel;  // < 0 = preset value
  };
  const Case cases[] = {{"robust_p50", "Robust", 0.5, -1, -1},
                        {"base_p50", "Base", 0.5, -1, -1},
                        {"robust_p90", "Robust", 0.9, -1, -1},
                        {"base_p90", "Base", 0.9, -1, -1},
                        {"robust_p90_levers", "Robust", 0.9, 0.45, 3.5}};
  CsvWriter w((out / "storage_land_2047.csv").string());
  w.field("plan").field("quantity").field("value").field("unit").field("note");
  w.endrow();
  auto row = [&](const std::string& plan, const char* q, double v, const char* unit, const std::string& note) {
    w.field(plan).field(q).field(v, 7).field(unit).field(note);
    w.endrow();
  };
  // Inputs shared by every plan.
  {
    const std::string all = "all";
    row(all, "complete_days", static_cast<double>(si.days.size()), "days",
        "NPP Merit hourly generation, " + si.days.front().date + " to " + si.days.back().date +
            "; months weighted to a 365-day year");
    static const char* names[kProfileSeries] = {"load", "solar", "wind", "hydro", "coal", "gas"};
    for (int s = 0; s < kProfileSeries; ++s)
      row(all, (std::string("trend_") + names[s]).c_str(), si.trend[s], "ln per year",
          si.detrended[s] ? "same month a year apart; removed from the shape (fleet growth)"
                          : "same month a year apart; kept (weather, not a trend)");
    row(all, "coal_peak_to_mean", si.coal_peak_ratio, "ratio", "highest hourly coal (Merit thermal) / mean hourly");
    row(all, "coal_minimum_level", si.coal_min_level, "fraction", "of the coal fleet's highest hourly output (CEA 2023)");
    row(all, "round_trip_efficiency", si.round_trip, "fraction", "NREL ATB 2025");
    row(all, "cea_2029_30_storage_power", si.bench_power_gw, "GW", "CEA optimal mix 2029-30 v2.0 (PSP + BESS)");
    row(all, "cea_2029_30_storage_energy", si.bench_energy_gwh, "GWh", "CEA optimal mix 2029-30 v2.0 (PSP + BESS)");
    row(all, "cea_2029_30_storage_gwh_per_twh_vre", si.bench_energy_gwh / (si.bench_generation_twh * si.bench_vre_share),
        "GWh per TWh", "storage energy / solar and wind generation, CEA 2029-30");
  }
  std::printf("storage and land (least-cost plan; total = firmed RE counted with its storage):\n");
  std::printf("  %-18s %7s %7s %7s %6s %7s %8s %7s %7s %8s %6s\n", "plan", "req TWh", "GW", "GWh", "h", "multi",
              "short h", "curt %", "loss", "km2", "% area");
  for (const Case& k : cases) {
    PolicySettings ps;
    for (const auto& pp : policy_presets())
      if (pp.name == k.policy) pp.apply(ps);
    if (k.solar_land > 0) ps.solar_land = k.solar_land;
    if (k.build_accel > 0) ps.build_accel = k.build_accel;
    ScenarioControls sc = c;
    sc.requirement_quantile = k.quantile;
    const StrategyResult r = pl.strategy(f, sc, ps);
    const StorageLandResult& s = r.storage;
    const std::string plan = k.plan;
    if (plan == "robust_p50" && s.ok) {
      // What-if settings on the recommended plan: a stiffer coal fleet and a lower round trip.
      struct WhatIf {
        const char* name;
        const char* note;
        void (*apply)(StorageInputs&);
      };
      const WhatIf whatifs[] = {
          {"robust_p50_coal_min_55", "what-if: coal fleet minimum 55% instead of 40% of its peak hour",
           [](StorageInputs& x) { x.coal_min_level = 0.55; }},
          {"robust_p50_round_trip_75", "what-if: round trip 75% instead of 85%",
           [](StorageInputs& x) { x.round_trip = 0.75; }},
      };
      for (const WhatIf& wi : whatifs) {
        StorageInputs x = si;
        wi.apply(x);
        const StorageLandResult t = storage_and_land(x, pl.allocation_inputs(), ps, r.least_cost);
        const StorageSizing& z = t.total;
        row(wi.name, "storage_power", z.power_gw, "GW", wi.note);
        row(wi.name, "storage_energy_daily", z.energy_day_gwh, "GWh", wi.note);
        row(wi.name, "storage_duration", z.duration_h, "h", wi.note);
        row(wi.name, "curtailed_vre_daily_storage_pct", z.curtailed_day_pct(), "% of VRE", wi.note);
        row(wi.name, "unmet_after_daily_storage", z.unmet_day_twh, "TWh", wi.note);
        std::printf("  %-26s %.0f GW, %.0f GWh, %.1f h, curtailed %.1f%%, unmet after storage %.0f TWh\n", wi.name,
                    z.power_gw, z.energy_day_gwh, z.duration_h, z.curtailed_day_pct(), z.unmet_day_twh);
      }
    }
    if (!s.ok) {
      row(plan, "status", 0, "", s.status);
      std::printf("  %-18s %s\n", k.plan, s.status.c_str());
      continue;
    }
    double req = 0, unserved = 0;
    for (int i = 0; i < kRegions; ++i) {
      req += r.requirement[i];
      unserved += r.least_cost.unserved[i];
    }
    const AllocationResult& a = r.least_cost;
    char desc[160];
    std::snprintf(desc, sizeof desc, "%s policy, P%.0f requirement%s", k.policy, 100 * k.quantile,
                  k.solar_land > 0 ? ", 45% usable solar land, build rates 3.5x" : "");
    row(plan, "requirement", req, "TWh", desc);
    row(plan, "unserved_in_plan", unserved, "TWh", "annual shortfall of the plan itself (LP); storage cannot supply it");
    row(plan, "solar_generation", a.national[kSolar], "TWh", "");
    row(plan, "onshore_wind_generation", a.national[kWind], "TWh", "");
    row(plan, "offshore_wind_generation", a.national[kOffshore], "TWh", "shaped like onshore wind");
    row(plan, "firmed_re_generation", a.national[kFirmRE], "TWh",
        "split back into solar and onshore wind in proportion to the usable potential the plan leaves in each region");
    row(plan, "firmed_re_solar_part", s.firm_re_solar_twh, "TWh", "");
    row(plan, "firmed_re_wind_part", s.firm_re_wind_twh, "TWh", "");
    for (int pass = 0; pass < 2; ++pass) {
      const StorageSizing& z = pass == 0 ? s.total : s.system;
      const std::string sfx = pass == 0 ? "" : "_firmed_re_as_rtc";
      const std::string scope = pass == 0 ? "all storage, firmed RE as solar and wind"
                                          : "system storage only, firmed RE as round-the-clock supply";
      auto put = [&](const char* q, double v, const char* unit, const std::string& note) {
        row(plan, (q + sfx).c_str(), v, unit, note.empty() ? scope : note + "; " + scope);
      };
      put("storage_power", z.power_gw, "GW",
          "charge and discharge rating: largest shortfall, raised while each extra GW serves 365 GWh a year");
      put("storage_discharge_power", z.discharge_gw, "GW", "largest hourly shortfall after firm dispatch");
      put("storage_energy_daily", z.energy_day_gwh, "GWh", "daily cycle, design day " + z.design_day);
      put("storage_energy_median_day", z.energy_day_median_gwh, "GWh", "daily need on the median day");
      put("storage_duration", z.duration_h, "h", "daily energy / storage power");
      put("storage_energy_multiday", z.energy_multi_gwh, "GWh",
          "carry-over between days; grown while each extra GWh serves at least 52 GWh of shortfall a year");
      put("shortfall_hours_before_storage", z.deficit_hours, "h/yr", "hours with load above supply after firm dispatch");
      put("shortfall_before_storage", z.deficit_twh, "TWh", "");
      put("surplus_before_storage", z.surplus_twh, "TWh", "");
      put("vre_generation", z.vre_twh, "TWh", "solar + wind + offshore in this run");
      put("curtailed_vre_daily_storage", z.curtailed_day_twh, "TWh", "surplus the daily store cannot take");
      put("curtailed_vre_daily_storage_pct", z.curtailed_day_pct(), "% of VRE", "");
      put("unmet_after_daily_storage", z.unmet_day_twh, "TWh", "extra generation still needed hour by hour");
      put("unmet_after_daily_storage_free_charging", z.unmet_day_free_charge_twh, "TWh",
          "same store if charging were not limited by its power (value of a larger charging rating)");
      put("storage_throughput", z.throughput_twh, "TWh/yr", "energy discharged");
      put("storage_losses", z.losses_twh, "TWh/yr", "charged - discharged");
      put("storage_full_cycles", z.cycles, "cycles/yr", "");
      put("unmet_after_multiday_storage", z.unmet_multi_twh, "TWh", "");
      put("curtailed_vre_multiday_storage", z.curtailed_multi_twh, "TWh", "");
      for (size_t i = 0; i < z.curve_unmet_twh.size(); ++i) {
        char q[64], note[96];
        std::snprintf(q, sizeof q, "unmet_with_carryover_store_x%.1f", StorageSizing::kCurveMult[i]);
        std::snprintf(note, sizeof note, "store of %.1f x the daily energy (%.0f GWh), carry-over between days",
                      StorageSizing::kCurveMult[i], StorageSizing::kCurveMult[i] * z.energy_day_gwh);
        put(q, z.curve_unmet_twh[i], "TWh", note);
      }
    }
    const LandUse& L = s.land;
    row(plan, "land_solar", L.solar_total(), "km2", "4 acres/MW (MNRE) at the regional CUF; solar incl. firmed RE");
    row(plan, "land_solar_high", L.solar_km2_high, "km2", "3.4 acres per GWh/yr (NREL large PV, total area)");
    row(plan, "land_wind_footprint", L.total_km2() - L.solar_total(), "km2", "0.3 ha/MW permanent footprint (NREL)");
    row(plan, "land_wind_project_area", L.wind_total_area(), "km2", "34.5 ha/MW project area (NREL); mostly still farmed");
    row(plan, "land_solar_wind", L.total_km2(), "km2", "solar + wind footprint; offshore wind uses no land");
    row(plan, "land_share_india", s.pct_india, "% of area", "India 3,287,469 km2 (ISFR 2023)");
    row(plan, "land_share_wasteland", s.pct_wasteland, "% of wasteland", "Wastelands Atlas 2019, 2015-16");
    for (int i = 0; i < kRegions; ++i)
      row(plan, (std::string("land_share_") + region_name(i)).c_str(), s.pct_region[i], "% of area",
          "solar + wind footprint in the region / its area");
    row(plan, "solar_share_of_potential", s.solar_share_of_potential, "fraction",
        "solar incl. firmed / NISE 2025 ground-mounted generation potential");
    row(plan, "solar_share_of_usable_potential", s.solar_share_of_usable, "fraction",
        "same / usable fraction of the policy (" + std::to_string(static_cast<int>(std::lround(100 * ps.solar_land))) +
            "%)");
    const StorageSizing& z = s.total;
    std::printf("  %-18s %7.0f %7.0f %7.0f %6.1f %7.0f %8.0f %7.1f %7.1f %8.0f %6.2f\n", k.plan, req, z.power_gw,
                z.energy_day_gwh, z.duration_h, z.energy_multi_gwh, z.deficit_hours, z.curtailed_day_pct(),
                z.losses_twh, L.total_km2(), s.pct_india);
    std::printf("  %-18s unserved in plan %.0f TWh; shortfall left after storage %.0f TWh | system only (firmed RE round "
                "the clock): %.0f GW, %.0f GWh, %.1f h, shortfall left %.0f TWh\n",
                "", unserved, z.unmet_day_twh, s.system.power_gw, s.system.energy_day_gwh, s.system.duration_h,
                s.system.unmet_day_twh);
  }
  std::printf("  CEA 2029-30 optimal mix: %.1f GW, %.1f GWh for %.0f TWh of solar and wind (%.2f GWh per TWh)\n",
              si.bench_power_gw, si.bench_energy_gwh, si.bench_generation_twh * si.bench_vre_share,
              si.bench_energy_gwh / (si.bench_generation_twh * si.bench_vre_share));
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const fs::path root = argc > 1 ? fs::path(argv[1]) : fs::current_path();
    const fs::path out = root / "outputs";
    fs::create_directories(out);

    Timer t_init;
    const Pipeline pl(root.string(), CoreModel::kIndia);
    const Fitted& fit = pl.fitted();
    const DriverData& drv = pl.drivers();
    std::printf("primary core: %s\n", core_name(pl.core()));
    std::printf("setup %.0f ms (bootstrap %.0f ms on %d threads)\n", t_init.ms(), fit.bootstrap_ms,
                omp_get_max_threads());
    const IndiaFit& fi = fit.india_fit;
    const IndiaFit& ff = fit.india_fit_full;
    std::printf("India elasticity FY%d-%d: beta %.3f (se %.3f, block-bootstrap 90%% %.3f-%.3f) | FY2020 %.3f | resid sd "
                "%.4f | DW %.2f | kappa %.3f\n",
                fi.first, fi.last, fi.beta, fi.beta_se, quantile(fit.india_beta_boot, 0.05),
                quantile(fit.india_beta_boot, 0.95), fi.covid, fi.resid_sd, fi.dw, fi.kappa);
    std::printf("  first differences: beta %.3f | base-year residual %+.4f\n", fi.fd_beta, fi.resid.back());
    std::printf("  full window FY%d-%d (2011 splice imputed %.4f): beta %.3f (se %.3f, first diff %.3f)\n", ff.first,
                ff.last, fit.india.splice_growth, ff.beta, ff.beta_se, ff.fd_beta);
    std::printf("  urban excess gamma prior: mean %.2f sd %.2f (%s) | captive/utility FY%d %.3f\n", fit.gamma_mean,
                fit.gamma_sd, fit.gamma_source.c_str(), fit.history.back().year, fit.captive_share);
    {
      // Rolling-origin backtest: mean absolute percentage error by horizon (1..5 years).
      CsvWriter w((out / "backtest_horizon.csv").string());
      w.field("horizon").field("n").field("mape_model0").field("mape_model1").field("mape_model2");
      w.endrow();
      std::printf("  rolling backtest MAPE by horizon (%s | %s | %s):\n", fit.model_names[0].c_str(),
                  fit.model_names[1].c_str(), fit.model_names[2].c_str());
      for (int h = 1; h <= 5; ++h) {
        double e[3] = {0, 0, 0};
        int n = 0;
        for (const auto& b : fit.backtests)
          for (const auto& p : b.points)
            if (p.year - b.origin == h) {
              e[0] += std::abs(p.fe_quad_twh / p.actual_twh - 1);
              e[1] += std::abs(p.fe_gompertz_twh / p.actual_twh - 1);
              e[2] += std::abs(p.damped_twh / p.actual_twh - 1);
              ++n;
            }
        if (n == 0) continue;
        w.field(h).field(n).field(e[0] / n, 5).field(e[1] / n, 5).field(e[2] / n, 5);
        w.endrow();
        std::printf("    h=%d (n=%d): %5.1f%% | %5.1f%% | %5.1f%%\n", h, n, 100 * e[0] / n, 100 * e[1] / n,
                    100 * e[2] / n);
      }
    }
    {
      CsvWriter w((out / "india_fit.csv").string());
      w.field("parameter").field("value").field("note");
      w.endrow();
      auto row = [&](const char* k, double v, const std::string& note) {
        w.field(k).field(v, 6).field(note);
        w.endrow();
      };
      row("window_first", fi.first, "fiscal-year start (2011 = FY2011-12)");
      row("window_last", fi.last, "");
      row("beta", fi.beta, "income elasticity of NPP generation per person");
      row("beta_se", fi.beta_se, "OLS standard error");
      row("beta_boot_p05", quantile(fit.india_beta_boot, 0.05), "moving-block bootstrap, block 3");
      row("beta_boot_p50", quantile(fit.india_beta_boot, 0.50), "");
      row("beta_boot_p95", quantile(fit.india_beta_boot, 0.95), "");
      row("covid_fy2020", fi.covid, "FY2020-21 dummy, log points");
      row("resid_sd", fi.resid_sd, "");
      row("durbin_watson", fi.dw, "");
      row("kappa", fi.kappa, "slope of urban share on ln income over the window");
      row("beta_first_difference", fi.fd_beta, "first differences, FY2020-21 and FY2021-22 changes left out");
      row("beta_full_window", ff.beta, "FY2006-07..FY2023-24 with the 2011 price-base splice");
      row("beta_full_window_se", ff.beta_se, "");
      row("beta_full_window_first_difference", ff.fd_beta, "");
      row("kappa_full_window", ff.kappa, "");
      row("resid_base_year", fi.resid.back(), "residual of the base year (FY2023-24), log points");
      row("anchor_shift", fit.anchor_shift,
          "base-year anchor moved to fitted + mean residual of FY2021-22..FY2023-24 when |resid| > resid_sd");
      row("splice_growth", fit.india.splice_growth, "imputed ln growth of GSDP per person for 2011");
      row("gamma_mean", fit.gamma_mean, fit.gamma_source);
      row("gamma_sd", fit.gamma_sd, "");
      row("captive_share", fit.captive_share, "captive / NPP utility generation in the base year (CEA)");
      row("g_hist_mean", drv.g_hist_mean, "mean ln growth of GSDP per person (organiser data)");
      row("g_hist_sd", drv.g_hist_sd, "");
      row("g_hist_rho", drv.g_hist_rho, "");
    }
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
    std::printf("  central paths %d: %s %.0f, %s %.0f, %s %.0f TWh\n", f.years[last], fit.model_names[0].c_str(),
                f.central[0][last], fit.model_names[1].c_str(), f.central[1][last], fit.model_names[2].c_str(),
                f.central[2][last]);
    std::printf("  captive layer (literature, same share as FY%d): +%.0f TWh at P50\n", fit.history.back().year,
                fit.captive_share * f.quant.q[0][last][3]);
    write_forecast(out, pl, f);

    // Holdout: FY2024-25 and FY2025-26 were not used in fitting. NPP publishes
    // conventional generation (thermal, nuclear, hydro) for them but no RES, so the
    // check compares conventional + RES extended at its FY2018-23 NPP growth rate.
    {
      std::map<int, double> conv;
      const CsvTable t = CsvTable::read((root / "data/raw/npp/npp_region_generation_fy.csv").string());
      for (size_t i = 0; i < t.rows(); ++i) {
        if (t.str(i, t.col("region")) != "All India" || t.str(i, t.col("partial")) == "True") continue;
        const std::string src = t.str(i, t.col("source"));
        if (src == "thermal" || src == "nuclear" || src == "hydro")
          conv[static_cast<int>(t.num(i, t.col("fy_start_year")))] += t.num(i, t.col("generation_gwh")) / 1000.0;
      }
      const auto& obs = fit.india.obs;
      const IndiaObs& o23 = obs.back();
      const IndiaObs& o18 = obs[obs.size() - 6];
      const double res_growth = std::pow(o23.res_twh / o18.res_twh, 1.0 / (o23.year - o18.year));
      CsvWriter w((out / "holdout.csv").string());
      w.field("fiscal_year_start").field("npp_conventional_twh").field("res_extended_twh").field("total_estimate_twh");
      w.field("model_p05").field("model_p50").field("model_p95").field("model_central").field("error_p50");
      w.endrow();
      std::printf("holdout (NPP conventional + RES at %.1f%%/yr vs model):\n", 100 * (res_growth - 1));
      for (int k = 0; k < 2 && k < static_cast<int>(f.years.size()); ++k) {
        const int y = f.years[k];
        if (!conv.count(y)) continue;
        const double res = o23.res_twh * std::pow(res_growth, y - o23.year);
        const double tot = conv[y] + res;
        const auto& q = f.quant.q[0][k];
        w.field(y).field(conv[y], 6).field(res, 6).field(tot, 6).field(q[0], 6).field(q[3], 6).field(q[6], 6);
        w.field(f.central[0][k], 6).field(q[3] / tot - 1, 4);
        w.endrow();
        std::printf("  FY%d: conventional %.0f + RES %.0f = %.0f | model P50 %.0f (P5 %.0f, P95 %.0f) | error %+.1f%%\n", y,
                    conv[y], res, tot, q[3], q[0], q[6], 100 * (q[3] / tot - 1));
      }
    }

    // Sensitivity checks on the 2047 national requirement (same seed, same draws).
    {
      CsvWriter w((out / "sensitivity_2047.csv").string());
      w.field("case").field("description").field("p05").field("p50").field("p95");
      w.endrow();
      struct Case {
        const char* name;
        const char* desc;
        void (*apply)(ScenarioControls&);
      };
      const Case cases[] = {
          {"base", "India elasticity with the excess-urbanisation prior", [](ScenarioControls&) {}},
          {"no_urban_term", "gamma = 0: two organiser datasets only, urbanisation inside beta",
           [](ScenarioControls& s) { s.w_model = {0, 1, 0}; }},
          {"damped_trend", "benchmark without drivers", [](ScenarioControls& s) { s.w_model = {0, 0, 1}; }},
          {"elasticity_decline_0.2", "elasticity falls 0.2 per unit of ln income",
           [](ScenarioControls& s) { s.elasticity_decline = 0.2; }},
          {"growth_plus_1.5", "income growth 1.5 points above history", [](ScenarioControls& s) { s.g_start_shift = 0.015; }},
          {"growth_minus_1.5", "income growth 1.5 points below history",
           [](ScenarioControls& s) { s.g_start_shift = -0.015; }},
          {"urban_plus_5", "urban share 5 points above the UN path by 2047",
           [](ScenarioControls& s) { s.urban_shift_2047 = 0.05; }},
          {"pop_band_x2", "population uncertainty doubled", [](ScenarioControls& s) { s.pop_band_scale = 2.0; }},
      };
      std::printf("sensitivity (2047 national, TWh P5 / P50 / P95):\n");
      for (const Case& k : cases) {
        ScenarioControls s = c;
        k.apply(s);
        const ForecastResult r = pl.forecast(s);
        const auto& q = r.quant.q[0][r.years.size() - 1];
        w.field(k.name).field(k.desc).field(q[0], 6).field(q[3], 6).field(q[6], 6);
        w.endrow();
        std::printf("  %-24s %6.0f %6.0f %6.0f\n", k.name, q[0], q[3], q[6]);
      }
    }

    // Secondary check: the cross-country panel core, same scenario settings.
    {
      Timer t2;
      const Pipeline sec(root.string(), CoreModel::kCrossCountry);
      ScenarioControls s = c;
      const ForecastResult r = sec.forecast(s);
      write_forecast(out, sec, r, "forecast_secondary_cross_country.csv");
      const size_t l2 = r.years.size() - 1;
      const auto& q2 = r.quant.q[0][l2];
      const auto& q1 = f.quant.q[0][last];
      CsvWriter w((out / "core_comparison.csv").string());
      w.field("core").field("scope").field("base_year").field("base_twh").field("target_year").field("p05").field(
          "p50").field("p95");
      w.endrow();
      w.field(core_name(pl.core())).field("NPP utility generation, fiscal year (start-year label)");
      w.field(fit.history.back().year).field(fit.history.back().gen_twh, 6).field(f.years[last]);
      w.field(q1[0], 6).field(q1[3], 6).field(q1[6], 6);
      w.endrow();
      w.field("India-only + captive layer").field("utility + captive (CEA share held at base year)");
      w.field(fit.history.back().year).field(fit.history.back().gen_twh * (1 + fit.captive_share), 6).field(f.years[last]);
      w.field(q1[0] * (1 + fit.captive_share), 6).field(q1[3] * (1 + fit.captive_share), 6);
      w.field(q1[6] * (1 + fit.captive_share), 6);
      w.endrow();
      w.field(core_name(sec.core())).field("Ember total generation incl. captive, calendar year");
      w.field(sec.fitted().history.back().year).field(sec.fitted().history.back().gen_twh, 6).field(r.years[l2]);
      w.field(q2[0], 6).field(q2[3], 6).field(q2[6], 6);
      w.endrow();
      std::printf("secondary (%s, %.0f ms): %d P5 %.0f P50 %.0f P95 %.0f TWh\n", core_name(sec.core()), t2.ms(),
                  r.years[l2], q2[0], q2[3], q2[6]);
    }

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
      w.field("origin").field("year").field("actual_twh").field("elasticity_twh").field("elasticity_no_urban_twh").field(
          "damped_twh");
      w.endrow();
      for (const auto& b : fit.backtests)
        for (const auto& pt : b.points) {
          w.field(b.origin).field(pt.year).field(pt.actual_twh).field(pt.fe_quad_twh).field(pt.fe_gompertz_twh).field(
              pt.damped_twh);
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
    {
      // What it takes to meet the P90 requirement: Robust policy with faster builds
      // and more usable solar land (rooftop, agri-PV, canal tops).
      CsvWriter w((out / "p90_levers.csv").string());
      w.field("build_accel").field("solar_land").field("unserved_p90_twh").field("cost_busd").field("co2_mt").field(
          "nonfossil_share");
      w.endrow();
      ScenarioControls s90 = c;
      s90.requirement_quantile = 0.9;
      std::printf("P90 levers (Robust policy; unmet TWh at P90):\n");
      for (double accel : {2.5, 3.0, 3.5, 4.0}) {
        std::printf("  build %.1fx:", accel);
        for (double land : {0.35, 0.45, 0.55}) {
          PolicySettings ps;
          for (const auto& pp : policy_presets())
            if (pp.name == "Robust") pp.apply(ps);
          ps.build_accel = accel;
          ps.solar_land = land;
          const StrategyResult r = pl.strategy(f, s90, ps);
          double u = 0;
          for (double x : r.least_cost.unserved) u += x;
          w.field(accel, 3).field(land, 3).field(u, 5).field(r.least_cost.cost_busd, 5).field(r.least_cost.co2_mt, 5);
          w.field(r.least_cost.nonfossil_share(), 4);
          w.endrow();
          std::printf("  land %.0f%% -> %6.0f", 100 * land, u);
        }
        std::printf("\n");
      }
    }
    write_storage_land(out, pl, f, c);
    std::printf("outputs written to %s\n", out.string().c_str());
    return 0;
  } catch (const std::exception& e) {
    std::fprintf(stderr, "error: %s\n", e.what());
    return 1;
  }
}
