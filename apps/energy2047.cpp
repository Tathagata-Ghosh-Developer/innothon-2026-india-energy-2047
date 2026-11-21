// energy2047: runs the full pipeline and writes results to <repo_root>/outputs/.
//
// Usage: energy2047 [repo_root] [draws]
#include <omp.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/csv.hpp"
#include "core/stats.hpp"
#include "forecast/state_panel.hpp"
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
      "base_fy2023_24_twh");
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

// Numbers shown on the landing page (web/landing.html). They are collected from the
// same in-memory results that write the CSV files and written to
// outputs/site_summary.json by write_site_summary(); the web build renders the page from it.
struct SiteSummary {
  struct Holdout {
    int year;
    double estimate_twh, p05, p50, p95;
  };
  struct Sensitivity {
    std::string key, description;
    double p50;
  };
  std::vector<Holdout> holdout;
  std::vector<Sensitivity> sensitivity;  // first entry: base case
  int today_year = 0;                        // latest complete fiscal year of NPP regional generation
  std::array<double, kRegions> today_twh{};  // NPP conventional generation (thermal, nuclear, hydro)
  double cross_country_p50 = 0;
  int cross_country_year = 0;
  AllocationResult base_plan;    // least cost, base policy, median requirement
  AllocationResult robust_plan;  // least cost, Robust policy, median requirement (presets matrix)
  bool robust_plan_ok = false;
  PolicySettings robust_policy;
  double robust_p90_unserved = -1;  // Robust policy planned for the P90 requirement; < 0 = infeasible
  bool storage_ok = false;
  StorageLandResult robust_storage;  // Robust policy, median requirement
  StorageLandResult levers_storage;  // Robust policy with the P90 levers, P90 requirement
  double levers_unserved = -1, levers_solar_land = 0;
  double levers_build_accel = 0;  // multiplier on the published additions of levers_build_case
  int levers_build_case = kBuildHigh;
  double cea_power_gw = 0, cea_energy_gwh = 0;
  std::vector<ParetoPoint> pareto;  // cost-CO2 trade-off, Robust policy, median requirement
  // Evidence behind the midpoint panel's questions (short summaries; the full tables are the
  // CSV files named in each key's comment in write_site_summary).
  int state_count = 0;              // states and union territories in the regional split
  GrowthDecomposition growth;       // central path to the target year (growth_decomposition_2047.csv)
  bool panel_ok = false;            // state panel check (state_panel_check.csv)
  PanelFit panel;
  StatePanelData panel_data;
  std::vector<NewLoad> new_loads;               // new_loads_2047.csv
  std::vector<MixSensitivityRow> mix_sensitivity;  // Robust preset copies (sensitivity_mix.csv)
};

// Storage and land back-calculated from the least-cost plan: Base and Robust policy
// at the median and P90 requirement, plus the Robust plan with the P90 levers
// (45% usable solar land, build rates at the published high-case limits), which meets the P90.
void write_storage_land(const fs::path& out, const Pipeline& pl, const ForecastResult& f, const ScenarioControls& c,
                        SiteSummary& site) {
  const StorageInputs& si = pl.storage_inputs();
  if (!si.ok) {
    std::printf("storage and land: skipped (%s)\n", si.status.c_str());
    return;
  }
  struct Case {
    const char* plan;
    const char* policy;
    double quantile;
    double solar_land, build_accel;  // < 0 = preset value; build_accel x the published limits
  };
  const Case cases[] = {{"robust_p50", "Robust", 0.5, -1, -1},
                        {"base_p50", "Base", 0.5, -1, -1},
                        {"robust_p90", "Robust", 0.9, -1, -1},
                        {"base_p90", "Base", 0.9, -1, -1},
                        {"robust_p90_levers", "Robust", 0.9, 0.45, 1.0}};
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
    if (plan == "robust_p50") {
      site.storage_ok = true;
      site.robust_storage = s;
      site.cea_power_gw = si.bench_power_gw;
      site.cea_energy_gwh = si.bench_energy_gwh;
    } else if (plan == "robust_p90_levers") {
      site.levers_storage = s;
      site.levers_unserved = unserved;
      site.levers_solar_land = ps.solar_land;
      site.levers_build_accel = ps.build_accel;
      site.levers_build_case = ps.build_case;
    }
    const AllocationResult& a = r.least_cost;
    char levers[128] = "";
    if (k.solar_land > 0 || k.build_accel > 0)
      std::snprintf(levers, sizeof levers, ", %.0f%% usable solar land, build rates %.2fx the published %s case",
                    100 * ps.solar_land, ps.build_accel, build_case_name(ps.build_case));
    char desc[224];
    std::snprintf(desc, sizeof desc, "%s policy, P%.0f requirement%s", k.policy, 100 * k.quantile, levers);
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
      put("gas_peaking_hours", z.gas_hours, "h/yr",
          "hours with gas above 1 MW; the plan's gas fills the hours of highest residual load");
      put("gas_peaking_generation", z.gas_twh, "TWh", "gas dispatched hour by hour (the plan's gas)");
      put("gas_peaking_max_power", z.gas_peak_gw, "GW", "largest hourly gas output (at most Merit peak/mean x mean)");
      put("gas_evening_generation", z.gas_evening_twh, "TWh", "gas in the hours 18:00-24:00 IST");
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
    if (z.gas_twh > 0)
      std::printf("  %-18s gas as peaking supply: %.0f h/yr, %.1f TWh, up to %.1f GW; %.1f TWh (%.0f%%) in 18:00-24:00 "
                  "IST\n",
                  "", z.gas_hours, z.gas_twh, z.gas_peak_gw, z.gas_evening_twh, 100 * z.gas_evening_twh / z.gas_twh);
  }
  std::printf("  CEA 2029-30 optimal mix: %.1f GW, %.1f GWh for %.0f TWh of solar and wind (%.2f GWh per TWh)\n",
              si.bench_power_gw, si.bench_energy_gwh, si.bench_generation_twh * si.bench_vre_share,
              si.bench_energy_gwh / (si.bench_generation_twh * si.bench_vre_share));
}

// outputs/committed_builds_2047.csv: lower bounds from plants already being built, by region, for
// the default (Base) and the Robust policy at the median requirement, with each plan's generation.
// Nuclear: units under construction or sanctioned, or commissioned after FY2023-24
// (nuclear_pipeline.csv); hydro: large hydro under construction (resource_potential.csv); floor =
// FY2023-24 + committed MW / 1000 x CUF x 8.76, cut to the national cap and the regional bound.
// Gas has no committed builds: its rows show the regional bound (the gas cap x the FY2023-24 share)
// and where the plan keeps the national gas minimum (gas_min_twh_share x the fleet floor).
void write_committed_builds(const fs::path& out, const AllocationInputs& in, const std::array<double, kRegions>& req) {
  CsvWriter w((out / "committed_builds_2047.csv").string());
  for (const char* h : {"policy", "region", "source", "base_fy2023_24_twh", "committed_mw", "cuf", "committed_twh",
                        "floor_twh", "committed_cut_twh", "upper_bound_twh", "plan_twh"})
    w.field(h);
  w.endrow();
  std::printf("committed builds and gas (2047 TWh, median requirement; floor = FY2023-24 + committed MW x CUF x 8.76):\n");
  for (const char* name : {"Base", "Robust"}) {
    PolicySettings ps;
    for (const auto& pp : policy_presets())
      if (pp.name == name) pp.apply(ps);
    const Bounds b = make_bounds(in, ps, req);
    const AllocationResult a = solve_least_cost(in, ps, req);
    for (int s : {kNuclear, kHydro, kGas}) {
      const auto* mw = s == kNuclear ? &in.committed_nuclear_mw : s == kHydro ? &in.committed_hydro_mw : nullptr;
      const double cuf = s == kNuclear ? in.committed_nuclear_cuf : s == kHydro ? in.committed_hydro_cuf : 0.0;
      double base = 0, committed = 0, cut = 0, floor = 0, plan = 0;
      std::string by;
      for (int r = 0; r < kRegions; ++r) {
        w.field(name).field(region_name(r)).field(tech_key(s)).field(in.base_twh[r][s], 6);
        w.field(mw ? (*mw)[r] : 0.0, 6).field(cuf, 4).field(b.committed[r][s], 6).field(b.lo[r][s], 6);
        w.field(b.committed_cut[r][s], 6).field(b.hi[r][s], 6);
        a.ok ? w.field(a.gen[r][s], 6) : w.field("infeasible");
        w.endrow();
        base += in.base_twh[r][s];
        committed += b.committed[r][s];
        cut += b.committed_cut[r][s];
        floor += b.lo[r][s];
        plan += a.ok ? a.gen[r][s] : 0.0;
        char cell[64];
        std::snprintf(cell, sizeof cell, "%s%s %.1f", by.empty() ? "" : ", ", region_name(r), a.ok ? a.gen[r][s] : 0.0);
        by += cell;
      }
      if (s == kGas)
        std::printf("  %-6s gas: national minimum %.1f TWh (%.0f%% of the %.1f TWh fleet floor); plan %.1f TWh (%s)\n",
                    name, gas_min_twh(in, ps), 100 * ps.gas_min_twh_share, in.fleet_floor_twh[kGas], plan, by.c_str());
      else
        std::printf("  %-6s %s: floor %.1f = FY2023-24 %.1f + committed %.1f (cut %.1f); plan %.1f TWh (%s)\n", name,
                    tech_key(s), floor, base, committed, cut, plan, by.c_str());
    }
  }
}

// outputs/population_consistency.csv: growth of the challenge dataset's state and
// national populations against the model's population paths and UN WPP 2024 (the
// organisers' condition that external projections be "reasonably consistent with the
// state-level population growth provided in the GitHub dataset"). Rates are compound
// annual growth in percent a year; an empty cell means the span is not available.
// The Technical Group column is a reference only: the model does not read it.
void write_population_consistency(const fs::path& out, const Pipeline& pl) {
  const PopulationConsistency pc = pl.population_consistency();
  const DriverData& drv = pl.drivers();
  const auto span = [](const char* what, int a, int b) {
    return std::string(what) + "_cagr_pct_" + std::to_string(a) + "_" + std::to_string(b);
  };
  CsvWriter w((out / "population_consistency.csv").string());
  w.field("area").field("grid_region").field("region_from").field("population_" + std::to_string(pc.base));
  w.field("share_pct_" + std::to_string(pc.base)).field("share_pct_" + std::to_string(pc.target));
  w.field(span("dataset", pc.long_from, pc.base)).field(span("dataset", pc.recent_from, pc.base));
  w.field(span("model", pc.base, pc.near)).field(span("model", pc.base, pc.target));
  w.field(span("un_wpp2024_india", pc.long_from, pc.base)).field(span("un_wpp2024_india", pc.recent_from, pc.base));
  w.field(span("un_wpp2024_india", pc.base, pc.near)).field(span("un_wpp2024_india", pc.base, pc.target));
  w.field(span("technical_group_reference", pc.base, pc.near));
  w.endrow();
  for (const PopulationGrowth& g : pc.rows) {
    w.field(g.area).field(g.region).field(g.region_from).field(g.pop_base, 12);
    w.field(100 * g.share_base, 6).field(100 * g.share_target, 6);
    w.field(100 * g.data_long, 5).field(100 * g.data_recent, 5).field(100 * g.model_near, 5).field(100 * g.model_target, 5);
    w.field(100 * pc.un_long, 5).field(100 * pc.un_recent, 5).field(100 * pc.un_near, 5).field(100 * pc.un_target, 5);
    w.field(100 * g.tg_near, 5);
    w.endrow();
  }

  // Short summary: India against the dataset and UN; how far each state's path for the
  // next six years is from its own recent growth; the range of state paths to 2047.
  const PopulationGrowth& india = pc.rows.front();
  std::printf("population (compound growth, %% a year; challenge dataset | model path | UN WPP 2024 medium):\n");
  std::printf("  India: dataset %d-%d %.2f, %d-%d %.2f | model %d-%d %.2f, %d-%d %.2f | UN %d-%d %.2f, %d-%d %.2f, "
              "%d-%d %.2f\n",
              pc.long_from, pc.base, 100 * india.data_long, pc.recent_from, pc.base, 100 * india.data_recent, pc.base,
              pc.near, 100 * india.model_near, pc.base, pc.target, 100 * india.model_target, pc.recent_from, pc.base,
              100 * pc.un_recent, pc.base, pc.near, 100 * pc.un_near, pc.base, pc.target, 100 * pc.un_target);
  double worst = 0, lo = 1, hi = -1;
  std::string worst_s, lo_s, hi_s;
  for (size_t k = 1; k < pc.rows.size(); ++k) {
    const PopulationGrowth& g = pc.rows[k];
    if (std::abs(g.model_near - g.data_recent) > worst) worst = std::abs(g.model_near - g.data_recent), worst_s = g.area;
    if (g.model_target < lo) lo = g.model_target, lo_s = g.area;
    if (g.model_target > hi) hi = g.model_target, hi_s = g.area;
  }
  std::printf("  %zu states: g_state = g_UN + (g_state - g_India over %d-%d); paths %d-%d within %.2f points of each "
              "state's own %d-%d growth (widest: %s); %d-%d from %.2f (%s) to %.2f (%s)\n",
              pc.rows.size() - 1, drv.state_growth_from, drv.state_pop_year, pc.base, pc.near, 100 * worst,
              pc.recent_from, pc.base, worst_s.c_str(), pc.base, pc.target, 100 * lo, lo_s.c_str(), 100 * hi,
              hi_s.c_str());
  std::printf("  grid regions: NPP state file; dataset zone where NPP lists no state (Central -> Western)\n");
  for (const std::string& n : drv.region_notes) std::printf("    %s\n", n.c_str());
}

std::string fy_label(int y);  // "FY2023-24"; defined below

// outputs/state_panel_check.csv: what a state-level regression would give (a diagnostic,
// never used by the forecast). ln(NPP state generation per person) on ln(dataset GSDP
// per person) with state fixed effects, FY2018-19..FY2023-24, and the spread of state
// generation per person against the national figure. Sections: data (sources), fit,
// spread (summary), state (one row per matched state), note (what was left out).
void write_state_panel_check(const fs::path& out, const fs::path& root, const Pipeline& pl, SiteSummary& site) {
  const StatePanelData d = state_panel_data(root.string(), &pl.gsdp_basis());  // GSDP on constant 2011-12 prices
  const PanelFit f = fe_slope(d.obs);
  site.panel = f;
  site.panel_data = d;
  site.panel_ok = true;
  const IndiaFit& fi = pl.fitted().india_fit;
  const std::string window = fy_label(d.first_year) + ".." + fy_label(d.last_year);
  std::string skipped;
  for (const std::string& s : d.skipped) skipped += (skipped.empty() ? "" : ", ") + s;
  const StateSpread* lo = nullptr;
  const StateSpread* hi = nullptr;
  std::string none;  // matched states with no generation in the window
  for (const StateSpread& s : d.spread) {
    if (!(s.ratio_to_national > 0)) {
      none += (none.empty() ? "" : ", ") + s.state;
      continue;
    }
    if (!lo || s.ratio_to_national < lo->ratio_to_national) lo = &s;
    if (!hi || s.ratio_to_national > hi->ratio_to_national) hi = &s;
  }

  CsvWriter w((out / "state_panel_check.csv").string());
  w.field("section").field("name").field("grid_region").field("years").field("gen_kwh_per_person");
  w.field("ratio_to_national").field("value").field("note");
  w.endrow();
  const auto row = [&](const char* section, const std::string& name, double value, const std::string& note) {
    w.field(section).field(name).field("").field("").field("").field("").field(value, 6).field(note);
    w.endrow();
  };
  row("data", "generation", kNaN,
      "NPP state file (data/raw/npp/npp_state_generation_fy.csv): conventional stations only (CEA 18-column reports: "
      "coal and lignite, gas, diesel, nuclear, hydro), no renewables. Generation is recorded in the state where the "
      "plant stands, not where the power is used. Sources summed per state and year; subtotal and partial-year rows "
      "left out");
  row("data", "population_and_gsdp", kNaN,
      "challenge dataset (data/raw/competition/renewable_energy_dataset.csv), years " + window +
          "; a GSDP equal to the year before is a placeholder and that state-year is left out; the 2024 GSDP is not "
          "used; names matched with normalize_state only; GSDP at constant 2011-12 prices (states listed in "
          "data/external/macro/gsdp_price_basis.csv are at current prices and are deflated with the MoSPI GDP "
          "deflator, secondary)");
  row("fit", "slope", f.slope,
      "within (state fixed effects) OLS of ln(generation per person) on ln(GSDP per person), " + window +
          ". Diagnostic only, never used by the forecast");
  row("fit", "se", f.se, "conventional OLS s.e., n_obs - n_states - 1 degrees of freedom, not clustered by state");
  row("fit", "n_obs", f.n_obs, "state-years in the regression");
  row("fit", "n_states", f.n_states, "states with at least one state-year in the regression");
  row("fit", "n_skipped", f.n_skipped,
      skipped.empty() ? std::string("no unmatched names")
                      : skipped + ": in the NPP state file, not in the challenge dataset; skipped, generation not "
                                  "assigned to any other state");
  row("fit", "left_out_zero_generation", d.n_zero_generation, "matched state-years with no generation in the NPP file");
  row("fit", "left_out_stale_gsdp", d.n_stale, "matched state-years with GSDP equal to the year before");
  row("fit", "left_out_no_data", d.n_no_data, "matched state-years with no dataset population or GSDP");
  row("fit", "india_beta", fi.beta,
      "the elasticity the forecast uses: national, NPP all-India generation (all sources) per person on GSDP per "
      "person, " + fy_label(fi.first) + ".." + fy_label(fi.last));
  row("spread", "national_gen_kwh_per_person", d.national_gen_pc_kwh,
      "all NPP state rows (matched or not) over the dataset's total population, " + window +
          " totals; same conventional-only basis as the states");
  if (lo) row("spread", "ratio_min", lo->ratio_to_national, lo->state + " (states with generation)");
  if (hi) row("spread", "ratio_max", hi->ratio_to_national, hi->state);
  row("spread", "sd_ln_ratio", d.sd_ln_ratio, "s.d. across states of ln(state / national), states with generation");
  row("spread", "states_without_generation", kNaN, none.empty() ? std::string("none") : none);
  row("spread", "between_share_ln_gen", d.between_share_ln_gen,
      "share of the variance of ln(generation per person) in the regression sample that lies between states "
      "(absorbed by the state effects)");
  row("spread", "between_share_ln_income", d.between_share_ln_income,
      "the same for ln(GSDP per person)");
  for (const StateSpread& s : d.spread) {
    w.field("state").field(s.state).field(s.region).field(s.n_years).field(s.gen_pc_kwh, 6);
    w.field(s.ratio_to_national, 6).field("");
    w.field(s.ratio_to_national > 0 ? std::string("") : std::string("no generation in the window: not in the regression"));
    w.endrow();
  }
  for (const std::string& n : d.notes) row("note", "", kNaN, n);

  std::printf("state panel check (diagnostic, not used): within slope %.3f (s.e. %.3f), %d state-years, %d states, "
              "%d skipped%s%s%s; national beta %.3f\n",
              f.slope, f.se, f.n_obs, f.n_states, f.n_skipped, skipped.empty() ? "" : " (", skipped.c_str(),
              skipped.empty() ? "" : ")", fi.beta);
  if (lo && hi)
    std::printf("  state generation per person %s: %.2fx (%s) to %.2fx (%s) the national %.0f kWh, s.d. of ln ratio "
                "%.2f; %.0f%% of the variance of ln generation per person is between states\n",
                window.c_str(), lo->ratio_to_national, lo->state.c_str(), hi->ratio_to_national, hi->state.c_str(),
                d.national_gen_pc_kwh, d.sd_ln_ratio, 100 * d.between_share_ln_gen);
}

// outputs/state_income_2047.csv: the regional split state by state (Pipeline::state_table),
// with the Monte Carlo's regional medians for comparison on the console.
void write_state_table(const fs::path& out, const Pipeline& pl, const ForecastResult& f, const ScenarioControls& c) {
  const DriverData& drv = pl.drivers();
  const size_t last = f.years.size() - 1;
  const int target = f.years[last];
  const std::vector<StateRow> rows = pl.state_table(c, target);
  const double lambda = pl.fitted().convergence.lambda;
  const std::string b = std::to_string(drv.state_pop_year), t = std::to_string(target);
  CsvWriter w((out / ("state_income_" + t + ".csv")).string());
  w.field("state").field("grid_region").field("region_from").field("pop_share_pct_" + b).field("ln_rel_income_" + b);
  w.field("rel_income_" + b).field("convergence_lambda_per_year").field("ln_rel_income_" + t).field("rel_income_" + t);
  w.field("per_person_index_" + t).field("pop_share_pct_" + t).field("requirement_share_pct_" + t);
  w.endrow();
  std::map<std::string, double> by_region;
  for (size_t k = 0; k < rows.size(); ++k) {
    const StateRow& r = rows[k];
    w.field(r.state).field(r.region).field(r.region_from).field(100 * r.pop_share_base, 6);
    w.field(r.ln_rel_income_base, 6).field(std::exp(r.ln_rel_income_base), 6).field(lambda, 6);
    w.field(r.ln_rel_income_target, 6).field(std::exp(r.ln_rel_income_target), 6).field(r.per_person_index_target, 6);
    w.field(100 * drv.state_pop_share[k][last], 6).field(100 * r.requirement_share_target, 6);
    w.endrow();
    by_region[r.region] += r.requirement_share_target;
  }
  double med_sum = 0;
  for (int r = 0; r < kRegions; ++r) med_sum += f.quant.q[1 + r][last][3];
  std::printf("state table %s (%zu states, convergence lambda %.4f/yr): regional share, state table | Monte Carlo "
              "medians:",
              t.c_str(), rows.size(), lambda);
  for (int r = 0; r < kRegions; ++r)
    std::printf(" %s %.1f%% | %.1f%%", region_name(r), 100 * by_region[region_name(r)],
                100 * f.quant.q[1 + r][last][3] / med_sum);
  std::printf("\n");
}

// outputs/new_loads_2047.csv: the published new loads (secondary sources) behind the
// new-loads scenario, which stays outside the headline. One row per load, scaled from its
// published year to the target year, then two totals: without the overlapping loads (the
// add-on of the new_loads_published sensitivity row and the dashboard preset) and with
// them (a reference only, never added).
void write_new_loads(const fs::path& out, const std::vector<NewLoad>& loads, int target_year) {
  const std::string t = std::to_string(target_year);
  const char* units = "TWh of end-use electricity, not generation: not grossed up for losses";
  CsvWriter w((out / ("new_loads_" + t + ".csv")).string());
  w.field("load").field("overlaps_history").field("in_addon").field("published_year").field("base_year");
  w.field("scale_to_" + t).field("low_published_twh").field("central_published_twh").field("high_published_twh");
  w.field("low_" + t + "_twh").field("central_" + t + "_twh").field("high_" + t + "_twh").field("units").field("note");
  w.field("basis");
  w.endrow();
  for (const NewLoad& l : loads) {
    const bool added = l.overlaps == "no";
    w.field(l.load).field(l.overlaps).field(added ? "yes" : "no").field(l.year).field(l.base_year).field(l.scale, 6);
    w.field(l.low_published, 6).field(l.central_published, 6).field(l.high_published, 6);
    w.field(l.low, 6).field(l.central, 6).field(l.high, 6).field(units);
    w.field(added ? "added to the new-loads scenario (outside the headline): absent from the historical generation "
                    "series; linear growth from near zero in the base year; if supplied from the grid (off-grid or "
                    "captive supply would be outside NPP utility generation)"
            : l.overlaps == "partial"
                ? "reported, not added: the income elasticity already carries part of this load"
                : "reported, not added: the historical series and the income elasticity already carry this load");
    w.field(l.basis);
    w.endrow();
  }
  // Totals; the published sums are given only when the summed loads share a published year.
  const auto total = [&](const char* name, bool with_overlap, const char* in_addon, const char* note) {
    const NewLoadTotals s = new_loads_total(loads, with_overlap);
    double pub[3] = {0, 0, 0}, scale = 0;
    int year = 0, base = 0;
    for (const NewLoad& l : loads) {
      if (!with_overlap && l.overlaps != "no") continue;
      pub[0] += l.low_published, pub[1] += l.central_published, pub[2] += l.high_published;
      year = (year == 0 || year == l.year) ? l.year : -1;
      base = l.base_year;
      scale = l.scale;
    }
    w.field(name).field(with_overlap ? "any" : "no").field(in_addon);
    year > 0 ? w.field(year) : w.field("");
    base > 0 ? w.field(base) : w.field("");
    year > 0 ? w.field(scale, 6) : w.field("");
    for (double v : pub) year > 0 ? w.field(v, 6) : w.field("");
    w.field(s.low, 6).field(s.central, 6).field(s.high, 6).field(units).field(note).field("");
    w.endrow();
  };
  total("total_without_overlapping_loads", false, "yes",
        "the add-on of the new-loads scenario (outside the headline): addon_lo = low, addon_hi = high, drawn uniformly "
        "and ramped linearly from the base year to the full value in the target year; if supplied from the grid");
  total("total_with_overlapping_loads", true, "no",
        "reference only, never added: double counts the part of the overlapping loads that the income elasticity "
        "already carries");
  const NewLoadTotals a = new_loads_total(loads), all = new_loads_total(loads, true);
  std::printf("new loads %s (published, outside the headline): add-on %.0f-%.0f TWh from %d of %zu loads; all loads "
              "%.0f-%.0f TWh (reference only)\n",
              t.c_str(), a.low, a.high, a.n, loads.size(), all.low, all.high);
}

// Minimal JSON writer. Numbers are written in fixed notation (never exponents) so the
// web build (cmake/Site.cmake) can round and format them; non-finite numbers become null.
class JsonOut {
 public:
  explicit JsonOut(const fs::path& path) : f_(std::fopen(path.string().c_str(), "wb")) {
    if (!f_) throw std::runtime_error("cannot write " + path.string());
  }
  ~JsonOut() { std::fclose(f_); }
  JsonOut(const JsonOut&) = delete;
  JsonOut& operator=(const JsonOut&) = delete;

  JsonOut& obj(const char* key = nullptr) { return open(key, '{'); }
  JsonOut& arr(const char* key = nullptr) { return open(key, '['); }
  JsonOut& end() {
    const char c = stack_.back();
    stack_.pop_back();
    newline();
    std::fputc(c == '{' ? '}' : ']', f_);
    first_ = false;
    if (stack_.empty()) std::fputc('\n', f_);
    return *this;
  }
  JsonOut& num(const char* key, double v, int decimals = 3) {
    item(key);
    if (!std::isfinite(v)) {
      std::fputs("null", f_);
      return *this;
    }
    if (std::abs(v) < 0.5 * std::pow(10.0, -decimals)) v = 0;  // no "-0.000"
    std::fprintf(f_, "%.*f", decimals, v);
    return *this;
  }
  JsonOut& integer(const char* key, long long v) {
    item(key);
    std::fprintf(f_, "%lld", v);
    return *this;
  }
  JsonOut& str(const char* key, const std::string& v) {
    item(key);
    quoted(v);
    return *this;
  }

 private:
  JsonOut& open(const char* key, char c) {
    item(key);
    std::fputc(c, f_);
    stack_.push_back(c);
    first_ = true;
    return *this;
  }
  void newline() {
    std::fputc('\n', f_);
    for (size_t i = 0; i < stack_.size(); ++i) std::fputs("  ", f_);
  }
  void item(const char* key) {
    if (!stack_.empty()) {
      if (!first_) std::fputc(',', f_);
      newline();
    }
    first_ = false;
    if (key) {
      quoted(key);
      std::fputs(": ", f_);
    }
  }
  void quoted(const std::string& s) {
    std::fputc('"', f_);
    for (const unsigned char ch : s) {
      if (ch == '"' || ch == '\\')
        std::fprintf(f_, "\\%c", ch);
      else if (ch < 0x20)
        std::fprintf(f_, "\\u%04x", ch);
      else
        std::fputc(ch, f_);
    }
    std::fputc('"', f_);
  }
  std::FILE* f_;
  std::vector<char> stack_;
  bool first_ = true;
};

std::string fy_label(int y) {
  char b[16];
  std::snprintf(b, sizeof b, "FY%d-%02d", y, (y + 1) % 100);
  return b;
}

// "andaman and nicobar islands" -> "Andaman and Nicobar Islands"; "green_hydrogen" -> "green hydrogen"
// with words=false (underscores to spaces only).
std::string display_name(std::string s, bool words = true) {
  std::replace(s.begin(), s.end(), '_', ' ');
  if (!words) return s;
  for (size_t i = 0; i < s.size(); ++i) {
    const bool start = i == 0 || s[i - 1] == ' ';
    if (start && s.compare(i, 4, "and ") != 0) s[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[i])));
  }
  return s;
}

// Upper end and step of a chart axis: about three steps of 1, 2, 2.5 or 5 x 10^k.
std::pair<double, double> nice_axis(double top) {
  const double raw = top / 3;
  const double mag = std::pow(10.0, std::floor(std::log10(raw)));
  double step = 10 * mag;
  for (const double m : {1.0, 2.0, 2.5, 5.0})
    if (m * mag >= raw) {
      step = m * mag;
      break;
    }
  return {std::ceil(top / step) * step, step};
}

// outputs/site_summary.json: every number on the landing page (see SiteSummary).
void write_site_summary(const fs::path& out, const Pipeline& pl, const ForecastResult& f, const SiteSummary& s) {
  const fs::path path = out / "site_summary.json";
  std::string missing;
  if (!s.robust_plan_ok) missing = "Robust plan";
  if (s.pareto.size() < 2) missing = "cost-CO2 trade-off";
  if (!s.storage_ok) missing += missing.empty() ? "storage and land" : ", storage and land";
  if (s.holdout.empty() || s.sensitivity.empty() || s.today_year == 0) missing += missing.empty() ? "checks" : ", checks";
  if (!missing.empty()) {
    fs::remove(path);  // the web build stops on a missing summary rather than show stale numbers
    std::printf("site summary: not written (missing %s)\n", missing.c_str());
    return;
  }
  const Fitted& fit = pl.fitted();
  const HistoryPoint& h = fit.history.back();
  const size_t last = f.years.size() - 1;
  const int target = f.years[last];
  const auto& q = f.quant.q[0][last];
  JsonOut j(path);
  j.obj();
  j.integer("schema", 1);
  j.str("about",
        "Numbers shown on the landing page (web/landing.html), written by energy2047 from the same results as the "
        "CSV files in this folder. Energy generated in TWh; fiscal years labelled by their start year.");
  j.str("core", core_name(pl.core()));
  j.integer("scenarios", static_cast<long long>(f.mc.draws));
  j.obj("years").integer("base", h.year).str("base_label", fy_label(h.year));
  j.integer("target", target).str("target_label", fy_label(target)).end();

  j.obj("national");
  j.num("base_twh", h.gen_twh);
  static const char* qn[8] = {"p05", "p10", "p25", "p50", "p75", "p90", "p95", "mean"};
  for (int k = 0; k < 8; ++k) j.num(qn[k], q[k]);
  j.num("multiple_of_base", q[3] / h.gen_twh, 4);
  j.num("growth_pct_per_year", 100 * (std::pow(q[3] / h.gen_twh, 1.0 / (target - h.year)) - 1));
  // The headline is the country's total in TWh (organisers' ruling of 16 November 2026).
  j.num("increase_twh", q[3] - h.gen_twh);
  // Cross-country panel: an independent check used for justification, not for estimation.
  j.num("cross_country_p50_twh", s.cross_country_p50).integer("cross_country_year", s.cross_country_year);
  // Frame of the national chart: years shown and a rounded upper end above the P90 path.
  const auto [ymax, ystep] = nice_axis(1.05 * q[5]);
  j.obj("chart").integer("x0", fit.history.front().year).integer("span", target - fit.history.front().year);
  j.num("ymax", ymax, 0).arr("ticks");
  for (int k = 0; k * ystep <= ymax + 1e-9; ++k) j.num(nullptr, k * ystep, 0);
  j.end().end();
  j.arr("history");
  for (const auto& p : fit.history)
    j.obj().integer("year", p.year).str("label", fy_label(p.year)).num("twh", p.gen_twh).end();
  j.end().arr("forecast");
  for (size_t t = 0; t < f.years.size(); ++t) {
    const auto& v = f.quant.q[0][t];
    j.obj().integer("year", f.years[t]).num("p10", v[1]).num("p50", v[3]).num("p90", v[5]).end();
  }
  j.end().arr("milestones");  // every fifth year from five years after the base year, and the target year
  for (size_t t = 0; t < f.years.size(); ++t) {
    const int y = f.years[t];
    if (!((y % 5 == 0 && y >= h.year + 5 && y <= target - 5) || y == target)) continue;
    const auto& v = f.quant.q[0][t];
    j.obj().integer("year", y).str("label", fy_label(y)).num("p10", v[1]).num("p50", v[3]).num("p90", v[5]).end();
  }
  j.end().end();

  j.obj("elasticity").num("beta", fit.india_fit.beta).str("from", fy_label(fit.india_fit.first));
  j.str("to", fy_label(fit.india_fit.last)).end();

  bool all_inside = true;
  j.arr("holdout");
  for (const auto& o : s.holdout) {
    const bool inside = o.estimate_twh >= o.p05 && o.estimate_twh <= o.p95;
    all_inside = all_inside && inside;
    j.obj().integer("year", o.year).str("label", fy_label(o.year)).num("estimate_twh", o.estimate_twh);
    j.num("model_p05", o.p05).num("model_p50", o.p50).num("model_p95", o.p95);
    j.num("error_pct", 100 * (o.p50 / o.estimate_twh - 1)).str("in_range", inside ? "inside" : "outside").end();
  }
  j.end();
  j.str("holdout_range_note", all_inside ? (s.holdout.size() == 2 ? "Both years fall inside the model's 90% range."
                                                                    : "Every year falls inside the model's 90% range.")
                                         : "Not every year falls inside the model's 90% range.");

  // Drivers of uncertainty: change of the 2047 median against the base case.
  {
    struct Plain {
      const char* key;
      const char* label;
    };
    static const Plain plain[] = {
        {"growth_plus_1.5", "Income growth 1.5 points higher"},
        {"growth_minus_1.5", "Income growth 1.5 points lower"},
        {"elasticity_decline_0.2", "Demand rises less as incomes rise"},
        {"urban_plus_5", "Urban share 5 points above UN path"},
        {"no_urban_term", "No urbanisation effect"},
        {"pop_band_x2", "Population range twice as wide"},
    };
    const double base = s.sensitivity.front().p50;
    struct Driver {
      std::string key, label;
      double p50, change;
    };
    std::vector<Driver> drivers;
    for (const auto& c : s.sensitivity)
      for (const Plain& p : plain)
        if (c.key == p.key) drivers.push_back({c.key, p.label, c.p50, 100 * (c.p50 / base - 1)});
    std::stable_sort(drivers.begin(), drivers.end(),
                     [](const Driver& a, const Driver& b) { return std::abs(a.change) > std::abs(b.change); });
    auto key_of = [](std::string k) {
      std::replace(k.begin(), k.end(), '.', '_');  // dots separate the path parts in the page template
      return k;
    };
    j.obj("sensitivity").num("base_p50_twh", base);
    j.num("scale_pct", drivers.empty() ? 1.0 : std::max(1.0, std::abs(drivers.front().change)));
    j.arr("drivers");
    for (const Driver& d : drivers)
      j.obj().str("key", key_of(d.key)).str("label", d.label).num("p50_twh", d.p50).num("change_pct", d.change).end();
    j.end();
    for (const auto& c : s.sensitivity) {
      j.obj(key_of(c.key).c_str()).str("description", c.description).num("p50_twh", c.p50);
      j.num("change_pct", 100 * (c.p50 / base - 1)).end();
    }
    j.end();
  }

  // What shapes the requirement beyond income (growth_decomposition_2047.csv): the central
  // path's log growth to the target year split by driver; the shares add up to 100.
  if (std::isfinite(s.growth.total) && s.growth.total != 0 && std::isfinite(s.growth.income)) {
    const GrowthDecomposition& g = s.growth;
    j.obj("growth_decomposition").str("from_label", fy_label(g.base_year)).str("to_label", fy_label(g.target_year));
    j.num("base_twh", g.gen_base_twh).num("central_twh", g.gen_target_twh).num("total_log_points", g.total, 4);
    j.num("population_pct", 100 * g.population / g.total, 2).num("income_pct", 100 * g.income / g.total, 2);
    j.num("urbanisation_pct", 100 * g.urbanisation / g.total, 2);
    j.num("model_error_pct", 100 * g.model_error / g.total, 2);
    j.str("note", "Shares of the log growth of the central path (medium drivers, mean income growth), not of the "
                  "median; model error is the fading of the high base-year residual.");
    j.end();
  }

  // State panel check (state_panel_check.csv): a diagnostic, never used by the forecast.
  if (s.panel_ok) {
    const StatePanelData& d = s.panel_data;
    const PanelFit& p = s.panel;
    const StateSpread* lo = nullptr;
    const StateSpread* hi = nullptr;
    for (const StateSpread& x : d.spread) {
      if (!(x.ratio_to_national > 0)) continue;
      if (!lo || x.ratio_to_national < lo->ratio_to_national) lo = &x;
      if (!hi || x.ratio_to_national > hi->ratio_to_national) hi = &x;
    }
    std::string skipped;
    for (const std::string& x : d.skipped) skipped += (skipped.empty() ? "" : ", ") + display_name(x);
    j.obj("state_panel").str("from_label", fy_label(d.first_year)).str("to_label", fy_label(d.last_year));
    j.num("slope", p.slope).num("se", p.se).integer("n_obs", p.n_obs).integer("n_states", p.n_states);
    j.integer("n_skipped", p.n_skipped).str("skipped", skipped);
    j.num("national_beta", fit.india_fit.beta).num("national_kwh_per_person", d.national_gen_pc_kwh);
    if (lo && hi) {
      j.num("ratio_min", lo->ratio_to_national).str("ratio_min_state", display_name(lo->state));
      j.num("ratio_max", hi->ratio_to_national).str("ratio_max_state", display_name(hi->state));
    }
    j.num("sd_ln_ratio", d.sd_ln_ratio).num("between_share_pct", 100 * d.between_share_ln_gen, 1);
    j.str("note", "Diagnostic only, never used: ln(NPP state generation per person) on ln(GSDP per person) with state "
                  "fixed effects. NPP records generation where the plant stands (conventional stations only), so a "
                  "state's figure measures where plants are, not its demand.");
    j.end();
  }

  // New-loads scenario (new_loads_2047.csv): outside the headline, secondary sources.
  if (!s.new_loads.empty()) {
    const NewLoadTotals add = new_loads_total(s.new_loads), all = new_loads_total(s.new_loads, true);
    std::string added, not_added;
    int year = 0;
    double scale = 0;
    for (const NewLoad& l : s.new_loads) {
      std::string& to = l.overlaps == "no" ? added : not_added;
      to += (to.empty() ? "" : ", ") + display_name(l.load, false);
      if (l.overlaps == "no") {
        year = (year == 0 || year == l.year) ? l.year : -1;
        scale = l.scale;
      }
    }
    j.obj("new_loads").str("added", added).str("not_added", not_added).integer("n_added", add.n);
    j.integer("n_loads", static_cast<long long>(s.new_loads.size()));
    if (year > 0) j.integer("published_year", year).num("scale", scale, 4);
    j.num("addon_low_twh", add.low).num("addon_central_twh", add.central).num("addon_high_twh", add.high);
    j.num("all_low_twh", all.low).num("all_high_twh", all.high);
    for (const auto& c : s.sensitivity)
      if (c.key == "new_loads_published")
        j.num("p50_twh", c.p50).num("change_pct", 100 * (c.p50 / s.sensitivity.front().p50 - 1));
    j.str("note", "Outside the headline. End-use electricity, not grossed up for losses, if supplied from the grid; "
                  "loads the income elasticity already carries in part are reported, never added (all-loads total is "
                  "a reference only).");
    j.end();
  }

  // Sensitivity of the recommended mix to the published figures behind it (sensitivity_mix.csv):
  // copies of the Robust preset with one change each, at the median and the P90 requirement.
  // A case that leaves the median unmet describes less generation than the requirement.
  if (!s.mix_sensitivity.empty()) {
    j.obj("sensitivity_mix").str("policy", "Robust");
    for (const MixSensitivityRow& r : s.mix_sensitivity) {
      const AllocationResult& a = r.plan;
      j.obj(r.name.c_str()).str("description", r.description).str("status", a.ok ? "optimal" : "infeasible");
      j.num("nonfossil_pct", a.ok ? 100 * a.nonfossil_share() : kNaN).num("coal_pct", a.ok ? 100 * a.share(kCoal) : kNaN);
      j.num("co2_mt", a.ok ? a.co2_mt : kNaN).num("cost_busd", a.ok ? a.cost_busd : kNaN);
      j.num("unmet_median_twh", a.ok ? a.unserved_total() : kNaN);
      j.num("unmet_p90_twh", r.plan_p90.ok ? r.plan_p90.unserved_total() : kNaN).end();
    }
    j.end();
  }

  // Regional requirement (median) against today's NPP conventional generation.
  {
    double need_sum = 0, today_sum = 0;
    for (int r = 0; r < kRegions; ++r) {
      need_sum += f.quant.q[1 + r][last][3];
      today_sum += s.today_twh[r];
    }
    double scale = 0;
    for (int r = 0; r < kRegions; ++r)
      scale = std::max({scale, 100 * f.quant.q[1 + r][last][3] / need_sum, 100 * s.today_twh[r] / today_sum});
    j.obj("regions").integer("today_year", s.today_year).str("today_label", fy_label(s.today_year));
    // States and union territories of the challenge dataset in the split (state_income_2047.csv).
    if (s.state_count > 0) j.integer("state_count", s.state_count);
    j.num("today_total_twh", today_sum).num("scale_pct", scale).arr("list");
    for (int r = 0; r < kRegions; ++r) {
      const auto& v = f.quant.q[1 + r][last];
      const double need = 100 * v[3] / need_sum, today = 100 * s.today_twh[r] / today_sum;
      j.obj().str("name", region_name(r)).num("p10_twh", v[1]).num("p50_twh", v[3]).num("p90_twh", v[5]);
      j.num("share_2047_pct", need).num("today_twh", s.today_twh[r]).num("share_today_pct", today);
      j.num("gap_points", need - today).end();
    }
    j.end().end();
  }

  // Recommended plan: least cost under the Robust policy at the median requirement.
  {
    auto total = [](const std::array<double, kRegions>& x) {
      double t = 0;
      for (double v : x) t += v;
      return t;
    };
    const AllocationResult& a = s.robust_plan;
    struct Group {
      const char* key;
      const char* label;
      const char* note;
      std::vector<int> techs;
    };
    const Group groups[] = {
        {"solar", "Solar", "ground-mounted, rooftop and agri-PV", {kSolar}},
        {"firmed", "Firmed renewables", "solar and wind backed by storage, sold round the clock", {kFirmRE}},
        {"coal", "Coal", "", {kCoal}},
        {"wind", "Wind", "onshore and offshore", {kWind, kOffshore}},
        {"hydro", "Hydro", "", {kHydro}},
        {"nuclear", "Nuclear", "", {kNuclear}},
        {"gasbio", "Gas and bioenergy", "", {kGas, kBio}},
    };
    struct Share {
      const Group* g;
      double twh;
    };
    std::vector<Share> mix;
    for (const Group& g : groups) {
      double twh = 0;
      for (int t : g.techs) twh += a.national[t];
      mix.push_back({&g, twh});
    }
    std::stable_sort(mix.begin(), mix.end(), [](const Share& x, const Share& y) { return x.twh > y.twh; });
    const double req = total(a.requirement);
    j.obj("plan").str("policy", "Robust").num("requirement_twh", req);
    {
      const PolicySettings& ps = s.robust_policy;  // the preset (pipeline/presets.hpp) as applied
      j.obj("settings").num("nonfossil_min_pct", 100 * ps.nonfossil_min).num("solar_land_pct", 100 * ps.solar_land);
      j.num("wind_land_pct", 100 * ps.wind_land).num("transfer_scale", ps.transfer_scale);
      // build_accel multiplies the published build limits of case build_case (low, central, high).
      j.num("build_accel", ps.build_accel).str("build_case", build_case_name(ps.build_case));
      j.num("nuclear_max_pct", 100 * ps.nuclear_max).end();
    }
    j.num("nonfossil_pct", 100 * a.nonfossil_share()).num("co2_mt", a.co2_mt).num("cost_busd", a.cost_busd);
    j.num("unmet_twh", total(a.unserved)).num("generation_twh", a.total_twh);
    // Information only: the plan's annual cost in nominal money of the target year at the
    // inflation goals. Every headline cost stays in real 2024 USD (core/price_base.hpp).
    {
      const PriceBase& pb = pl.price_base();
      const double inr_lakh_crore = pb.nominal_inr_from_real_usd(a.cost_busd * 1e9, target) / 1e12;
      const double usd_bn = pb.nominal_usd_from_real_usd(a.cost_busd, target);
      j.obj("nominal_outlook").str("label", "Information only: nominal money of " + std::to_string(target) +
                                                ". The headline cost is real 2024 USD.");
      j.num("real_2024_busd", a.cost_busd).integer("year", target);
      j.num("inr_lakh_crore", inr_lakh_crore).num("usd_bn", usd_bn);
      j.num("inr_per_usd_2024", pb.inr_per_usd(kPriceBaseRatePeriod), 4);
      j.num("india_inflation_pct", 100 * pb.inflation_target()).num("us_inflation_pct", 100 * pb.us_inflation_goal());
      j.str("method",
            "INR: real 2024 USD x Rs per USD (RBI calendar-2024 average) x (1 + RBI CPI target)^(year - 2024); USD: real "
            "2024 USD x (1 + FOMC longer-run goal, PCE price index)^(year - 2024). 1 lakh crore = 10^12 rupees. Sources: "
            "data/external/macro/price_indices.csv (secondary).");
      j.end();
      std::printf("nominal outlook (information only; Robust plan %.2f bn USD/yr in real 2024 USD): %d money Rs %.1f "
                  "lakh crore at %.0f%% a year (RBI target) or USD %.1f bn at %.0f%% a year (FOMC goal)\n",
                  a.cost_busd, target, inr_lakh_crore, 100 * pb.inflation_target(), usd_bn,
                  100 * pb.us_inflation_goal());
    }
    j.num("scale_pct", a.total_twh > 0 ? 100 * mix.front().twh / a.total_twh : 1.0).arr("mix");
    for (const Share& m : mix)
      j.obj().str("key", m.g->key).str("label", m.g->label).str("note", m.g->note).num("twh", m.twh).num(
          "share_pct", a.total_twh > 0 ? 100 * m.twh / a.total_twh : 0.0).end();
    j.end();
    // The six energy sources of the official challenge page, region by region (the landing
    // page's map; plan_vectors in allocation/allocate.hpp). Wind is onshore plus offshore.
    // Solar is solar alone: firmed renewables (solar and wind sold round the clock with
    // storage) and bioenergy are not among the six and go to vectors_other. "lead" is left out
    // below 0.5 TWh; radius = sqrt(region TWh / largest region's TWh), for circle areas.
    {
      const PlanVectors pv = plan_vectors(a);
      j.obj("vectors");
      for (const SourceVector& v : pv.sources) {
        j.obj(v.key).str("label", v.label).num("twh", v.twh).num("share_pct", v.share_pct);
        if (v.lead >= 0) {
          j.str("lead", region_name(v.lead)).num("lead_twh", v.region_twh[v.lead]);
          j.num("lead_share_pct", v.region_share_pct[v.lead]);
        }
        j.arr("regions");
        for (int r = 0; r < kRegions; ++r)
          j.obj().str("name", region_name(r)).num("twh", v.region_twh[r]).num("share_pct", v.region_share_pct[r]).num(
              "radius", v.radius[r], 4).end();
        j.end().end();
      }
      j.end();
      j.obj("vectors_other").str("label", "Firmed renewables and bioenergy").num("twh", pv.other_twh);
      j.num("share_pct", pv.other_share_pct);
      j.str("note", "Not among the six sources: solar and wind sold round the clock with storage, and bioenergy. "
                    "With them the six shares add up to 100% of the plan.");
      j.end();
    }
    // Committed builds and the gas kept for peaking in this plan (committed_builds_2047.csv):
    // nuclear and hydro floors = FY2023-24 + committed MW x CUF x 8.76; gas minimum = share x
    // the gas fleet's best NPP year.
    {
      const AllocationInputs& in = pl.allocation_inputs();
      const PolicySettings& ps = s.robust_policy;
      const Bounds b = make_bounds(in, ps, a.requirement);
      double mw[2] = {0, 0}, committed[2] = {0, 0}, floor[2] = {0, 0}, cut[2] = {0, 0};
      for (int r = 0; r < kRegions; ++r)
        for (int k = 0; k < 2; ++k) {
          const int t = k == 0 ? kNuclear : kHydro;
          mw[k] += (k == 0 ? in.committed_nuclear_mw : in.committed_hydro_mw)[r];
          committed[k] += b.committed[r][t];
          floor[k] += b.lo[r][t];
          cut[k] += b.committed_cut[r][t];
        }
      j.obj("committed");
      j.num("nuclear_mw", mw[0], 1).num("nuclear_cuf", in.committed_nuclear_cuf, 2).num("nuclear_committed_twh", committed[0]);
      j.num("nuclear_floor_twh", floor[0]).num("nuclear_cut_twh", cut[0]).num("nuclear_twh", a.national[kNuclear]);
      j.num("hydro_mw", mw[1], 1).num("hydro_cuf", in.committed_hydro_cuf, 2).num("hydro_committed_twh", committed[1]);
      j.num("hydro_floor_twh", floor[1]).num("hydro_cut_twh", cut[1]).num("hydro_twh", a.national[kHydro]);
      j.num("gas_min_share_pct", 100 * ps.gas_min_twh_share).num("gas_fleet_floor_twh", in.fleet_floor_twh[kGas]);
      j.str("gas_fleet_floor_label", fy_label(in.fleet_floor_year[kGas]));
      j.num("gas_min_twh", gas_min_twh(in, ps)).num("gas_twh", a.national[kGas]).end();
    }
    j.arr("regions");  // where the plan generates, and what each region sends to or takes from others
    for (int r = 0; r < kRegions; ++r) {
      double gen = 0;
      for (int t = 0; t < kTechs; ++t) gen += a.gen[r][t];
      j.obj().str("name", region_name(r)).num("requirement_twh", a.requirement[r]).num("generation_twh", gen);
      j.num("net_export_twh", a.net_export[r]).end();
    }
    // Inter-regional corridors of this plan (capability computed as in corridors_2047.csv).
    // "link" is from-to and "pair" lower-higher region index; the page draws them.
    const auto& corridors = pl.allocation_inputs().corridors;
    const PolicySettings& pol = s.robust_policy;
    double flow_max = 0;
    for (double x : a.corridor_flow) flow_max = std::max(flow_max, std::abs(x));
    j.end().num("flow_scale_twh", std::max(1.0, flow_max));
    for (int pass = 0; pass < 2; ++pass) {
      j.arr(pass == 0 ? "corridors" : "flows");  // all corridors; those carrying at least 1 TWh
      for (size_t k = 0; k < corridors.size() && k < a.corridor_flow.size(); ++k) {
        const Corridor& cr = corridors[k];
        const double x = a.corridor_flow[k];
        if (pass == 1 && std::abs(x) < 1.0) continue;
        const int from = x >= 0 ? cr.a : cr.b, to = x >= 0 ? cr.b : cr.a;
        const double cap = cr.mw_2032 * pol.transfer_scale * pol.transfer_util * 8.76e-3;
        j.obj().str("name", cr.name).str("from", region_name(from)).str("to", region_name(to));
        j.str("link", std::to_string(from) + std::to_string(to));
        j.str("pair", std::to_string(std::min(from, to)) + std::to_string(std::max(from, to)));
        j.num("twh", std::abs(x)).num("capability_twh", cap).num("used_pct", cap > 0 ? 100 * std::abs(x) / cap : 0.0);
        j.end();
      }
      j.end();
    }
    const AllocationResult& b = s.base_plan;
    j.obj("base_policy").num("nonfossil_pct", 100 * b.nonfossil_share()).num("co2_mt", b.co2_mt);
    j.num("cost_busd", b.cost_busd).num("unmet_twh", total(b.unserved)).end();
    j.obj("p90").num("requirement_twh", q[5]).num("robust_unmet_twh", s.robust_p90_unserved);
    j.num("levers_solar_land_pct", 100 * s.levers_solar_land).num("levers_build_accel", s.levers_build_accel);
    j.str("levers_build_case", build_case_name(s.levers_build_case));
    j.num("levers_unmet_twh", s.levers_unserved).end();
    // Cost-CO2 trade-off of the recommended plan (epsilon-constraint sweep on CO2).
    // Points are measured from the least-cost plan: CO2 avoided and extra cost.
    {
      const ParetoPoint& first = s.pareto.front();
      const ParetoPoint& last = s.pareto.back();
      const double cut = first.co2_mt - last.co2_mt, extra = last.cost_busd - first.cost_busd;
      double max_mac = 0;
      for (const ParetoPoint& t : s.pareto) max_mac = std::max(max_mac, t.marginal_usd_per_t);
      const auto [xmax, xstep] = nice_axis(std::max(1e-6, cut));
      const auto [ymax, ystep] = nice_axis(std::max(1e-6, 1.05 * extra));
      j.obj("tradeoff");
      j.num("co2_first_mt", first.co2_mt).num("co2_last_mt", last.co2_mt).num("cut_mt", cut);
      j.num("cut_pct", 100 * cut / first.co2_mt).num("extra_busd", extra).num("extra_pct", 100 * extra / first.cost_busd);
      j.num("max_mac_usd_per_t", max_mac).num("coal_last_pct", 100 * last.plan.share(kCoal));
      j.num("nonfossil_last_pct", 100 * last.plan.nonfossil_share()).num("coal_floor_pct", 100 * s.robust_policy.coal_floor);
      j.num("xmax", xmax, 3).num("ymax", ymax, 3).arr("xticks");
      for (double v = 0; v <= xmax + 1e-9; v += xstep) j.obj().num("v", v, 3).num("pct", 100 * v / xmax, 3).end();
      j.end().arr("yticks");
      for (double v = 0; v <= ymax + 1e-9; v += ystep) j.obj().num("v", v, 3).num("pct", 100 * v / ymax, 3).end();
      j.end().arr("points");
      for (const ParetoPoint& t : s.pareto)
        j.obj().num("co2_mt", t.co2_mt).num("cost_busd", t.cost_busd).num("avoided_mt", first.co2_mt - t.co2_mt)
            .num("extra_busd", t.cost_busd - first.cost_busd, 4).num("mac_usd_per_t", t.marginal_usd_per_t)
            .num("nonfossil_pct", 100 * t.plan.nonfossil_share()).num("coal_pct", 100 * t.plan.share(kCoal)).end();
      j.end().end();
    }
    j.end();

    const StorageSizing& z = s.robust_storage.total;
    j.obj("storage").num("power_gw", z.power_gw).num("energy_gwh", z.energy_day_gwh).num("duration_h", z.duration_h);
    j.num("curtailed_pct", z.curtailed_day_pct()).num("shortfall_after_twh", z.unmet_day_twh);
    j.num("shortfall_after_pct", req > 0 ? 100 * z.unmet_day_twh / req : 0.0).num("losses_twh", z.losses_twh);
    j.num("cea_2029_power_gw", s.cea_power_gw).num("cea_2029_energy_gwh", s.cea_energy_gwh);
    j.num("energy_multiple_of_cea", s.cea_energy_gwh > 0 ? z.energy_day_gwh / s.cea_energy_gwh : 0.0);
    // Gas kept for peaking, from the hourly check (evening = 18:00-24:00 IST).
    j.num("gas_hours", z.gas_hours, 0).num("gas_twh", z.gas_twh).num("gas_peak_gw", z.gas_peak_gw);
    j.num("gas_evening_twh", z.gas_evening_twh).num("gas_evening_pct", z.gas_twh > 0 ? 100 * z.gas_evening_twh / z.gas_twh : 0.0);
    j.end();

    const StorageLandResult& L = s.robust_storage;
    j.obj("land").num("solar_wind_km2", L.land.total_km2()).num("share_india_pct", L.pct_india);
    j.num("share_wasteland_pct", L.pct_wasteland).num("solar_share_of_potential_pct", 100 * L.solar_share_of_potential);
    j.num("wind_project_area_km2", L.land.wind_total_area());
    j.num("p90_levers_km2", s.levers_storage.land.total_km2()).num("p90_levers_share_india_pct", s.levers_storage.pct_india);
    j.end();
  }
  j.end();
  std::printf("site summary: %s\n", path.string().c_str());
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
    std::printf("  urban excess gamma prior: mean %.2f sd %.2f (%s)\n", fit.gamma_mean, fit.gamma_sd,
                fit.gamma_source.c_str());
    std::printf("  GSDP price basis: %d state-years at current prices deflated to 2011-12 prices (MoSPI GDP deflator; "
                "data/external/macro/gsdp_price_basis.csv)\n",
                fit.india.gsdp_current_corrected);
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
      {
        std::string listed;
        for (const GsdpBasisRow& b : pl.gsdp_basis().rows())
          listed += (listed.empty() ? "" : ", ") + b.state + " from " + std::to_string(b.from_year);
        row("gsdp_current_price_state_years", fit.india.gsdp_current_corrected,
            "challenge-dataset state-years at current prices (" + (listed.empty() ? std::string("none") : listed) +
                "; data/external/macro/gsdp_price_basis.csv) deflated to constant 2011-12 prices with the MoSPI GDP "
                "deflator (secondary): an inflation correction of a mis-based series, not new estimation data");
      }
      row("gamma_mean", fit.gamma_mean, fit.gamma_source);
      row("gamma_sd", fit.gamma_sd, "");
      row("g_hist_mean", drv.g_hist_mean, "mean ln growth of GSDP per person (organiser data)");
      row("g_hist_sd", drv.g_hist_sd, "");
      row("g_hist_rho", drv.g_hist_rho, "");
    }
    std::printf("damped trend: alpha %.3f beta %.3f phi %.3f sigma %.4f level %.3f trend %.4f\n", fit.damped.alpha,
                fit.damped.beta, fit.damped.phi, fit.damped.sigma, fit.damped.level, fit.damped.trend);
    std::printf("GDP pc growth %d-%d: mean %.4f sd %.4f rho %.2f | convergence lambda %.4f (se %.4f)\n",
                drv.g_hist_from, drv.g_hist_to, drv.g_hist_mean, drv.g_hist_sd, drv.g_hist_rho,
                fit.convergence.lambda, fit.convergence.lambda_se);
    std::printf("states %zu (challenge dataset %d); WPP medium %d %.0f M -> 2047 %.0f M; urban (WUP) %.3f -> %.3f\n",
                drv.states.size(), drv.state_pop_year, drv.base_year, drv.wpp_med.front() / 1e6,
                drv.wpp_med.back() / 1e6, drv.wup_base, drv.wup.back());
    SiteSummary site;  // landing-page numbers, written at the end (write_site_summary)
    site.state_count = static_cast<int>(drv.states.size());
    write_population_consistency(out, pl);
    write_state_panel_check(out, root, pl, site);

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
    {
      // Headline: the country's total in TWh (NPP utility generation), not per person.
      const auto& q = f.quant.q[0][last];
      const HistoryPoint& h = fit.history.back();
      std::printf("  headline: %s needs %.0f TWh (median; 90%% range %.0f-%.0f TWh), +%.0f TWh over the %.0f TWh of %s "
                  "(NPP utility generation, country total)\n",
                  fy_label(f.years[last]).c_str(), q[3], q[0], q[6], q[3] - h.gen_twh, h.gen_twh,
                  fy_label(h.year).c_str());
    }
    write_forecast(out, pl, f);

    // Growth decomposition of the central path to 2047: is income the only driver?
    // Parts in log points (they add up to the total) and as a share of the total.
    {
      const GrowthDecomposition d = pl.decompose_central(c, f.years[last]);
      site.growth = d;
      const std::string from = fy_label(d.base_year), to = fy_label(d.target_year);
      auto text = [](const char* fmt, auto... v) {
        char b[256];
        std::snprintf(b, sizeof b, fmt, v...);
        return std::string(b);
      };
      const struct {
        const char* name;
        std::string desc;
        double v;
      } parts[] = {
          {"population", text("ln(population %s / %s): base-year population growing as UN WPP 2024 medium",
                              to.c_str(), from.c_str()),
           d.population},
          {"income",
           text("beta %.3f x change of ln GSDP per person, less delta/2 x its square (elasticity decline delta %.2f)",
                fi.beta, std::max(0.0, c.elasticity_decline)),
           d.income},
          {"urbanisation",
           text("gamma %.2f x (change of urban share - kappa %.3f x change of ln GSDP per person): urbanisation beyond "
                "the path income implies",
                fit.gamma_mean, fi.kappa),
           d.urbanisation},
          {"model_error",
           text("expected residual in %s minus the base-year residual %+.3f (AR(1), rho %.2f)", to.c_str(),
                fi.resid.back(), fi.resid_rho),
           d.model_error},
          {"total",
           text("ln(central path %s / %s) = ln(%.0f / %.0f TWh); medium drivers, mean growth, full-sample "
                "parameters, no add-on",
                to.c_str(), from.c_str(), d.gen_target_twh, d.gen_base_twh),
           d.total},
      };
      CsvWriter w((out / "growth_decomposition_2047.csv").string());
      w.field("part").field("description").field("log_points").field("share_of_total_pct");
      w.endrow();
      for (const auto& p : parts) {
        w.field(p.name).field(p.desc).field(p.v, 6).field(std::round(10000 * p.v / d.total) / 100);  // 2 decimals
        w.endrow();
      }
      std::printf("growth of the central path %s -> %s (%.0f -> %.0f TWh, %.3f log points):", from.c_str(), to.c_str(),
                  d.gen_base_twh, d.gen_target_twh, d.total);
      for (const auto& p : parts)
        if (std::string(p.name) != "total") std::printf(" %s %+.3f (%.1f%%)", p.name, p.v, 100 * p.v / d.total);
      std::printf("\n");
    }
    // The national elasticity applied state by state: the regional split made visible.
    write_state_table(out, pl, f, c);
    // Published new loads (secondary sources) for a scenario outside the headline.
    const std::vector<NewLoad> new_loads = pl.new_loads();
    write_new_loads(out, new_loads, f.years[last]);
    site.new_loads = new_loads;

    // Holdout: FY2024-25 and FY2025-26 were not used in fitting. NPP publishes
    // conventional generation (thermal, nuclear, hydro) for them but no RES, so the
    // check compares conventional + RES extended at its FY2018-23 NPP growth rate.
    {
      std::map<int, double> conv;
      std::map<int, std::array<double, kRegions>> conv_region;  // same sources by region (landing page)
      const CsvTable t = CsvTable::read((root / "data/raw/npp/npp_region_generation_fy.csv").string());
      for (size_t i = 0; i < t.rows(); ++i) {
        if (t.str(i, t.col("partial")) == "True") continue;
        const std::string src = t.str(i, t.col("source"));
        if (src != "thermal" && src != "nuclear" && src != "hydro") continue;
        const int y = static_cast<int>(t.num(i, t.col("fy_start_year")));
        const double twh = t.num(i, t.col("generation_gwh")) / 1000.0;
        const std::string& region = t.str(i, t.col("region"));
        if (region == "All India") conv[y] += twh;
        for (int r = 0; r < kRegions; ++r)
          if (region == region_name(r)) conv_region[y][r] += twh;
      }
      // Today's regional generation: the latest complete fiscal year with every region.
      for (auto it = conv_region.rbegin(); it != conv_region.rend(); ++it)
        if (std::all_of(it->second.begin(), it->second.end(), [](double v) { return v > 0; })) {
          site.today_year = it->first;
          site.today_twh = it->second;
          break;
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
        site.holdout.push_back({y, tot, q[0], q[3], q[6]});
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
        std::string name, desc;
        std::function<void(ScenarioControls&)> apply;
        const Pipeline* on = nullptr;  // another pipeline (other data options); nullptr: pl
      };
      // Elasticity cases: one OLS standard error of the FY2011-12 onward fit either way,
      // each window on its own, and the mixture recentred on the first-difference estimate.
      const double se = fi.beta_se;
      const double fd_shift = fi.fd_beta - mean(fit.india_beta_boot);
      auto with_value = [](const char* fmt, double v) {
        char b[128];
        std::snprintf(b, sizeof b, fmt, v);
        return std::string(b);
      };
      // New-loads case: which loads are added, the add-on range and how it was scaled.
      std::string new_loads_case;
      {
        const NewLoadTotals a = new_loads_total(new_loads);
        std::string names;
        double scale = 0;
        int year = 0;
        for (const NewLoad& l : new_loads)
          if (l.overlaps == "no") {
            names += (names.empty() ? "" : ", ") + l.load;
            scale = l.scale;
            year = (year == 0 || year == l.year) ? l.year : -1;
          }
        const std::string scaled =
            year > 0 ? with_value(" (published values x %.4f, linear from the base year)", scale) : "";
        char b[512];
        std::snprintf(b, sizeof b,
                      "outside the headline: published new loads absent from the historical series (%s), add-on "
                      "%.0f-%.0f TWh in %d%s; end-use electricity, not grossed up for losses; if supplied from the "
                      "grid; loads that partly overlap the income elasticity not added (secondary sources)",
                      names.empty() ? "none" : names.c_str(), a.low, a.high, f.years[last], scaled.c_str());
        new_loads_case = b;
      }
      // Jammu and Kashmir left out of national income growth: its GSDP in the challenge dataset is
      // at current prices from 2012, which the main run corrects with the MoSPI GDP deflator; this
      // run drops the state from income growth instead, and re-estimates the elasticity.
      PipelineOptions no_jk;
      no_jk.income_exclude_states = {"Jammu and Kashmir"};
      const Pipeline jk(root.string(), CoreModel::kIndia, kDefaultBootstrapReplicates, no_jk);
      char jk_desc[400];
      std::snprintf(jk_desc, sizeof jk_desc,
                    "Jammu and Kashmir left out of national income growth instead of deflating its current-price GSDP "
                    "(2012 onward) with the MoSPI GDP deflator; elasticity re-estimated: S1 beta %.3f, S3 beta %.3f",
                    jk.fitted().india_fit.beta, jk.fitted().india_fit_full.beta);
      const Case cases[] = {
          {"base", "India elasticity with the excess-urbanisation prior", [](ScenarioControls&) {}},
          {"no_urban_term", "gamma = 0: no urbanisation effect beyond income (UN population path still used)",
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
          {"beta_minus_1se", with_value("income elasticity lower by one OLS standard error (%.3f) in every replicate", se),
           [se](ScenarioControls& s) { s.beta_shift = -se; }},
          {"beta_plus_1se", with_value("income elasticity higher by one OLS standard error (%.3f) in every replicate", se),
           [se](ScenarioControls& s) { s.beta_shift = se; }},
          {"beta_window_s1", "elasticity from the FY2011-12 onward window only (one price base)",
           [](ScenarioControls& s) { s.beta_window = 1; }},
          {"beta_window_s3", "elasticity from the FY2006-07 onward window only (spliced price bases)",
           [](ScenarioControls& s) { s.beta_window = 2; }},
          {"beta_first_difference",
           with_value("elasticity recentred on the first-difference estimate (shift %+.3f in every replicate)", fd_shift),
           [fd_shift](ScenarioControls& s) { s.beta_shift = fd_shift; }},
          // Outside the headline: published new loads absent from the historical series
          // (outputs/new_loads_2047.csv), on a copy of the default controls.
          {"new_loads_published", new_loads_case,
           [&new_loads](ScenarioControls& s) { apply_new_loads(s, new_loads); }},
          {"jk_excluded", jk_desc, [](ScenarioControls&) {}, &jk},
      };
      std::printf("sensitivity (2047 national, TWh P5 / P50 / P95):\n");
      for (const Case& k : cases) {
        ScenarioControls s = c;
        k.apply(s);
        const ForecastResult r = (k.on ? *k.on : pl).forecast(s);
        const auto& q = r.quant.q[0][r.years.size() - 1];
        w.field(k.name).field(k.desc).field(q[0], 6).field(q[3], 6).field(q[6], 6);
        w.endrow();
        site.sensitivity.push_back({k.name, k.desc, q[3]});
        std::printf("  %-24s %6.0f %6.0f %6.0f\n", k.name.c_str(), q[0], q[3], q[6]);
      }
    }

    // Cross-country panel core, same scenario settings: an independent check used for
    // justification, not for estimation (organisers' ruling of 16 November 2026).
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
      w.field(core_name(sec.core())).field("Ember total generation (beyond NPP's utility series), calendar year");
      w.field(sec.fitted().history.back().year).field(sec.fitted().history.back().gen_twh, 6).field(r.years[l2]);
      w.field(q2[0], 6).field(q2[3], 6).field(q2[6], 6);
      w.endrow();
      std::printf("%s, %.0f ms: %d P5 %.0f P50 %.0f P95 %.0f TWh\n", core_name(sec.core()), t2.ms(), r.years[l2], q2[0],
                  q2[3], q2[6]);
      site.cross_country_p50 = q2[3];
      site.cross_country_year = r.years[l2];
    }

    {
      const AllocationInputs& ai = pl.allocation_inputs();
      double total = 0, res = 0, coal = 0;
      for (int r = 0; r < kRegions; ++r)
        for (int s = 0; s < kTechs; ++s) {
          total += ai.base_twh[r][s];
          res += ai.base_res_twh[r][s];
          if (s == kCoal) coal += ai.base_twh[r][s];
        }
      std::printf("base mix FY%d-%02d: %.0f TWh = NPP conventional %.0f (primary) + CEA renewables %.0f (secondary); "
                  "coal %.0f TWh; renewable-table states without a region: %d\n",
                  ai.base_year, (ai.base_year + 1) % 100, total, total - res, res, coal, ai.base_unmatched_states);
    }
    const PolicySettings p;
    const StrategyResult st = pl.strategy(f, c, p);
    const AllocationResult& lc = st.least_cost;
    site.base_plan = lc;
    std::printf("strategy (least cost): %s | non-fossil %.1f%% | CO2 %.0f Mt | cost %.0f bn USD/yr\n",
                lc.status.c_str(), 100 * lc.nonfossil_share(), lc.co2_mt, lc.cost_busd);
    std::printf("  national mix:");
    for (int s = 0; s < kTechs; ++s) std::printf(" %s %.1f%%", tech_key(s), 100 * lc.share(s));
    std::printf("\n");
    for (int r = 0; r < kRegions; ++r)
      std::printf("  %-14s requirement %6.0f  net export LC %+6.0f  score %+6.0f TWh\n", region_name(r),
                  st.requirement[r], lc.net_export[r], st.score.net_export[r]);
    write_strategy(out, pl.allocation_inputs(), st);
    write_committed_builds(out, pl.allocation_inputs(), st.requirement);
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
                            "cost_at_p90_busd", "unserved_at_p05_twh", "cost_at_p05_busd"})
        w.field(h);
      w.endrow();
      std::printf("preset matrix (2047 median TWh | non-fossil | coal | CO2 Mt | unserved TWh; the same policy at "
                  "P90 and at P05):\n");
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
          // Low-demand stress test: the same policy at the P05 requirement. The coal floor, the
          // gas minimum and the committed nuclear and hydro floors are fixed in TWh while the
          // non-fossil minimum is a share, so a low enough requirement makes the LP infeasible.
          // Split as in Pipeline::strategy: the national P05 shared by the regional medians.
          const int tl = static_cast<int>(pf.years.size()) - 1;
          const double p05 = pf.mc.quantile(pf.inputs, 0, tl, 0.05);
          double med_sum = 0;
          for (double x : r.requirement) med_sum += x;
          std::array<double, kRegions> req05 = r.requirement;
          for (double& x : req05) x *= p05 / med_sum;
          const AllocationResult a05 = solve_least_cost(pl.allocation_inputs(), ps, req05);
          if (a05.ok)
            w.field(a05.unserved_total(), 4).field(a05.cost_busd, 5);
          else
            w.field("infeasible").field("infeasible");
          w.endrow();
          if (sp.name == "Base" && pp.name == "Robust") {  // the recommended plan on the landing page
            site.robust_plan = a;
            site.robust_plan_ok = a.ok;
            site.robust_policy = ps;
            site.robust_p90_unserved = r90.least_cost.ok ? u90 : -1.0;
          }
          std::printf("  %-12s x %-16s %6.0f | %5.1f%% | %5.1f%% | %5.0f | %6.1f | P90 unmet %6s | P05 (%4.0f TWh) %s %s\n",
                      sp.name.c_str(), pp.name.c_str(), q[3], 100 * a.nonfossil_share(), 100 * a.share(kCoal),
                      a.co2_mt, unserved, r90.least_cost.ok ? std::to_string(std::lround(u90)).c_str() : "infeasible",
                      p05, a05.ok ? "feasible" : "infeasible", a.ok ? "" : a.status.c_str());
        }
      }
    }
    {
      // What it takes to meet the P90 requirement: Robust policy (published high-case build
      // limits) with builds faster than the published limits and more usable solar land
      // (rooftop, agri-PV, canal tops). build_accel multiplies the published additions.
      CsvWriter w((out / "p90_levers.csv").string());
      w.field("build_case").field("build_accel_x_published").field("solar_land").field("unserved_p90_twh").field(
          "cost_busd").field("co2_mt").field("nonfossil_share");
      w.endrow();
      ScenarioControls s90 = c;
      s90.requirement_quantile = 0.9;
      std::printf("P90 levers (Robust policy; unmet TWh at P90):\n");
      for (double accel : {1.0, 1.25, 1.5, 2.0}) {
        std::printf("  build %.2fx published:", accel);
        for (double land : {0.35, 0.45, 0.55}) {
          PolicySettings ps;
          for (const auto& pp : policy_presets())
            if (pp.name == "Robust") pp.apply(ps);
          ps.build_accel = accel;
          ps.solar_land = land;
          const StrategyResult r = pl.strategy(f, s90, ps);
          double u = 0;
          for (double x : r.least_cost.unserved) u += x;
          w.field(build_case_name(ps.build_case)).field(accel, 3).field(land, 3).field(u, 5);
          w.field(r.least_cost.cost_busd, 5).field(r.least_cost.co2_mt, 5);
          w.field(r.least_cost.nonfossil_share(), 4);
          w.endrow();
          std::printf("  land %.0f%% -> %6.0f", 100 * land, u);
        }
        std::printf("\n");
      }
    }
    {
      // Sensitivity of the recommended mix to the published figures behind it: each case is a
      // copy of the Robust preset with one change (cost basis, solar potential set, wind
      // height, build case), solved at the median requirement and re-solved at the P90.
      // unserved_median_twh shows where a case cannot meet the median requirement: its shares,
      // CO2 and cost then describe less generation than the requirement. cost_busd is priced
      // in the case's own cost basis.
      PolicySettings robust;
      for (const auto& pp : policy_presets())
        if (pp.name == "Robust") pp.apply(robust);
      ScenarioControls s90 = c;
      s90.requirement_quantile = 0.9;
      const std::array<double, kRegions> req = pl.strategy(f, c, robust).requirement;
      const std::array<double, kRegions> req90 = pl.strategy(f, s90, robust).requirement;
      const std::vector<MixSensitivityRow> rows = mix_sensitivity(pl.allocation_inputs(), robust, req, req90);
      site.mix_sensitivity = rows;
      CsvWriter w((out / "sensitivity_mix.csv").string());
      for (const char* h : {"case", "non_fossil_pct", "coal_pct", "co2_mt", "cost_busd", "unserved_p90_twh",
                            "unserved_median_twh"})
        w.field(h);
      w.endrow();
      std::printf("mix sensitivity (Robust policy copy; median requirement; unmet TWh at the median and at P90):\n");
      for (const MixSensitivityRow& r : rows) {
        const AllocationResult& a = r.plan;
        w.field(r.name);
        if (a.ok)
          w.field(100 * a.nonfossil_share(), 4).field(100 * a.share(kCoal), 4).field(a.co2_mt, 6).field(a.cost_busd, 5);
        else
          w.field("infeasible").field("infeasible").field("infeasible").field("infeasible");
        if (r.plan_p90.ok)
          w.field(r.plan_p90.unserved_total(), 5);
        else
          w.field("infeasible");
        if (a.ok)
          w.field(a.unserved_total(), 5);
        else
          w.field("infeasible");
        w.endrow();
        std::printf("  %-26s non-fossil %5.1f%% | coal %5.1f%% | CO2 %6.0f Mt | cost %6.1f bn USD | unmet %6.1f | P90 "
                    "unmet %6.1f | %s\n",
                    r.name.c_str(), a.ok ? 100 * a.nonfossil_share() : 0.0, a.ok ? 100 * a.share(kCoal) : 0.0,
                    a.ok ? a.co2_mt : 0.0, a.ok ? a.cost_busd : 0.0, a.ok ? a.unserved_total() : -1.0,
                    r.plan_p90.ok ? r.plan_p90.unserved_total() : -1.0, r.description.c_str());
      }
    }
    {
      // Cost-CO2 trade-off of the recommended plan: Robust policy at the median requirement,
      // CO2 capped in 12 steps from the least-cost plan down to the lowest cap that still
      // meets the requirement; the cap's dual is the marginal abatement cost.
      PolicySettings ps;
      for (const auto& pp : policy_presets())
        if (pp.name == "Robust") pp.apply(ps);
      const StrategyResult r = pl.strategy(f, c, ps);
      site.pareto = pareto_cost_co2(pl.allocation_inputs(), ps, r.requirement, 12);
      CsvWriter w((out / "pareto_2047.csv").string());
      for (const char* h : {"co2_cap_mt", "co2_mt", "cost_busd", "marginal_abatement_usd_per_t", "nonfossil_share",
                            "coal_share", "solar_share", "firmed_re_share", "nuclear_share", "gas_share"})
        w.field(h);
      w.endrow();
      std::printf("cost-CO2 trade-off (Robust policy, median requirement):\n");
      for (const ParetoPoint& t : site.pareto) {
        const AllocationResult& a = t.plan;
        w.field(t.co2_cap_mt, 6).field(t.co2_mt, 6).field(t.cost_busd, 6).field(t.marginal_usd_per_t, 5);
        w.field(a.nonfossil_share(), 4).field(a.share(kCoal), 4).field(a.share(kSolar), 4).field(a.share(kFirmRE), 4);
        w.field(a.share(kNuclear), 4).field(a.share(kGas), 4);
        w.endrow();
        std::printf("  CO2 %7.1f Mt  cost %7.2f bn USD  MAC %6.1f USD/t  non-fossil %5.1f%%  coal %5.1f%%\n", t.co2_mt,
                    t.cost_busd, t.marginal_usd_per_t, 100 * a.nonfossil_share(), 100 * a.share(kCoal));
      }
    }
    write_storage_land(out, pl, f, c, site);
    write_site_summary(out, pl, f, site);
    std::printf("outputs written to %s\n", out.string().c_str());
    return 0;
  } catch (const std::exception& e) {
    std::fprintf(stderr, "error: %s\n", e.what());
    return 1;
  }
}
