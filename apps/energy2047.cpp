// energy2047: runs the full pipeline and writes results to <repo_root>/outputs/.
//
// Usage: energy2047 [repo_root] [draws]
#include <omp.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

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
  double levers_unserved = -1, levers_solar_land = 0, levers_build_accel = 0;
  double cea_power_gw = 0, cea_energy_gwh = 0;
  std::vector<ParetoPoint> pareto;  // cost-CO2 trade-off, Robust policy, median requirement
};

// Storage and land back-calculated from the least-cost plan: Base and Robust policy
// at the median and P90 requirement, plus the Robust plan with the P90 levers
// (45% usable solar land, build rates 3.5x the record year), which meets the P90.
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
  j.num("captive_share_pct", 100 * fit.captive_share);
  j.num("base_with_captive_twh", h.gen_twh * (1 + fit.captive_share));
  j.num("p50_with_captive_twh", q[3] * (1 + fit.captive_share));
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
      j.num("build_accel", ps.build_accel).num("nuclear_max_pct", 100 * ps.nuclear_max).end();
    }
    j.num("nonfossil_pct", 100 * a.nonfossil_share()).num("co2_mt", a.co2_mt).num("cost_busd", a.cost_busd);
    j.num("unmet_twh", total(a.unserved)).num("generation_twh", a.total_twh);
    j.num("scale_pct", a.total_twh > 0 ? 100 * mix.front().twh / a.total_twh : 1.0).arr("mix");
    for (const Share& m : mix)
      j.obj().str("key", m.g->key).str("label", m.g->label).str("note", m.g->note).num("twh", m.twh).num(
          "share_pct", a.total_twh > 0 ? 100 * m.twh / a.total_twh : 0.0).end();
    j.end().arr("regions");  // where the plan generates, and what each region sends to or takes from others
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
    j.num("levers_unmet_twh", s.levers_unserved).end();
    // Cost-CO2 trade-off of the recommended plan (epsilon-constraint sweep on CO2).
    j.arr("tradeoff");
    for (const ParetoPoint& t : s.pareto)
      j.obj().num("co2_mt", t.co2_mt).num("cost_busd", t.cost_busd).num("mac_usd_per_t", t.marginal_usd_per_t)
          .num("nonfossil_pct", 100 * t.plan.nonfossil_share()).num("coal_pct", 100 * t.plan.share(kCoal)).end();
    j.end();
    j.end();

    const StorageSizing& z = s.robust_storage.total;
    j.obj("storage").num("power_gw", z.power_gw).num("energy_gwh", z.energy_day_gwh).num("duration_h", z.duration_h);
    j.num("curtailed_pct", z.curtailed_day_pct()).num("shortfall_after_twh", z.unmet_day_twh);
    j.num("shortfall_after_pct", req > 0 ? 100 * z.unmet_day_twh / req : 0.0).num("losses_twh", z.losses_twh);
    j.num("cea_2029_power_gw", s.cea_power_gw).num("cea_2029_energy_gwh", s.cea_energy_gwh);
    j.num("energy_multiple_of_cea", s.cea_energy_gwh > 0 ? z.energy_day_gwh / s.cea_energy_gwh : 0.0).end();

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
    SiteSummary site;  // landing-page numbers, written at the end (write_site_summary)

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
        site.sensitivity.push_back({k.name, k.desc, q[3]});
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
      site.cross_country_p50 = q2[3];
      site.cross_country_year = r.years[l2];
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
          if (sp.name == "Base" && pp.name == "Robust") {  // the recommended plan on the landing page
            site.robust_plan = a;
            site.robust_plan_ok = a.ok;
            site.robust_policy = ps;
            site.robust_p90_unserved = r90.least_cost.ok ? u90 : -1.0;
          }
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
