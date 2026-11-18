// Allocation check: loads the real inputs, checks the base-year mix against its
// sources, checks the published build limits, solves the least-cost LP and the
// score allocation for a test requirement (4,500 TWh split by base-year regional
// shares), and verifies balances and policy constraints.
// Usage: test_allocation <repo_root>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <numeric>
#include <string>

#include "allocation/allocate.hpp"
#include "core/csv.hpp"
#include "core/stats.hpp"
#include "pipeline/pipeline.hpp"
#include "pipeline/presets.hpp"

using namespace e47;

namespace {

int region_of(const std::string& name) {
  for (int r = 0; r < kRegions; ++r)
    if (name == region_name(r)) return r;
  return -1;
}

// The FY2023-24 base mix is NPP's regional conventional generation (primary) plus
// CEA renewable generation by state (secondary). Small hydro is a renewable in the
// CEA table and part of the model's hydro, so the conventional comparison takes
// hydro without it (NPP's hydro is stations above 25 MW).
int base_mix_matches_npp(const std::string& root, const AllocationInputs& in) {
  int fails = 0;
  auto check = [&](bool ok, const std::string& what) {
    std::printf("  %-74s %s\n", what.c_str(), ok ? "ok" : "FAIL");
    if (!ok) ++fails;
  };
  std::printf("base_mix_matches_npp\n");
  // NPP regional totals (thermal + nuclear + hydro) and thermal; Bhutan import is a region of its own.
  std::array<double, kRegions> npp{}, npp_thermal{};
  {
    const CsvTable t = CsvTable::read(root + "/data/raw/npp/npp_region_generation_fy.csv");
    for (size_t i = 0; i < t.rows(); ++i) {
      if (t.str(i, t.col("fiscal_year")) != "2023-24") continue;
      const int r = region_of(t.str(i, t.col("region")));
      const std::string s = t.str(i, t.col("source"));
      if (r >= 0 && s == "total") npp[r] = t.num(i, t.col("generation_gwh")) / 1000.0;
      if (r >= 0 && s == "thermal") npp_thermal[r] = t.num(i, t.col("generation_gwh")) / 1000.0;
    }
  }
  // NPP station sums by state (Sub-report 16): coal share of thermal by region, where thermal
  // is coal and lignite + gas + diesel stations.
  std::array<double, kRegions> st_coal{}, st_thermal{};
  {
    const CsvTable t = CsvTable::read(root + "/data/raw/npp/npp_state_generation_fy.csv");
    for (size_t i = 0; i < t.rows(); ++i) {
      if (t.str(i, t.col("fiscal_year")) != "2023-24" || t.str(i, t.col("partial")) == "True") continue;
      const int r = region_of(t.str(i, t.col("region")));
      const std::string s = t.str(i, t.col("source"));
      const double v = t.num(i, t.col("generation_gwh"));
      if (r < 0 || std::isnan(v)) continue;
      if (s == "coal_lignite") st_coal[r] += v;
      if (s == "coal_lignite" || s == "gas" || s == "diesel") st_thermal[r] += v;
    }
  }
  // CEA renewable generation by region and type (region rows of the same table).
  RegionTech cea{};
  {
    const CsvTable t = CsvTable::read(root + "/data/raw/generation/state/cea_state_res_by_type_fy.csv");
    for (size_t i = 0; i < t.rows(); ++i) {
      if (t.str(i, t.col("fiscal_year")) != "2023-24" || t.str(i, t.col("row_type")) != "region_total") continue;
      const int r = region_of(t.str(i, t.col("region")));
      const std::string s = t.str(i, t.col("source"));
      const int k = s == "solar" ? kSolar : s == "wind" ? kWind : s == "hydro_small" ? kHydro
                  : s == "biomass" || s == "other" ? kBio : -1;
      if (r >= 0 && k >= 0) cea[r][k] += t.num(i, t.col("generation_gwh")) / 1000.0;
    }
  }
  check(in.base_year == 2023, "base year FY2023-24 (fiscal years by start year)");
  for (int r = 0; r < kRegions; ++r) {
    const auto& b = in.base_twh[r];
    const double conv = b[kCoal] + b[kGas] + b[kNuclear] + b[kHydro] - in.base_res_twh[r][kHydro];
    std::printf("  %-14s coal+gas+nuclear+hydro (excl. small hydro) %7.1f TWh, NPP %7.1f TWh\n", region_name(r), conv,
                npp[r]);
    check(npp[r] > 0 && std::abs(conv / npp[r] - 1.0) < 0.005,
          std::string(region_name(r)) + ": conventional within 0.5% of NPP FY2023-24");
    check(npp_thermal[r] > 0 && std::abs((b[kCoal] + b[kGas]) / npp_thermal[r] - 1.0) < 0.005,
          std::string(region_name(r)) + ": coal + gas within 0.5% of NPP FY2023-24 thermal");
    const double want = st_thermal[r] > 0 ? st_coal[r] / st_thermal[r] : kNaN;
    const double got = b[kCoal] + b[kGas] > 0 ? b[kCoal] / (b[kCoal] + b[kGas]) : kNaN;
    std::printf("  %-14s coal share of thermal %.6f, NPP station sums %.6f\n", region_name(r), got, want);
    check(std::abs(got - want) < 1e-6,
          std::string(region_name(r)) + ": coal share = NPP station sums (Sub-report 16) within 1e-6");
  }
  for (int r = 0; r < kRegions; ++r)
    for (int k : {kSolar, kWind, kHydro, kBio}) {
      if (cea[r][k] <= 0) continue;  // CEA shows none
      const double v = in.base_res_twh[r][k];
      char what[96];
      std::snprintf(what, sizeof what, "%s: %s positive (base %.2f, CEA %.2f TWh)", region_name(r),
                    k == kHydro ? "small hydro" : tech_key(k), v, cea[r][k]);
      check(v > 0 && (k == kHydro || in.base_twh[r][k] >= v), what);
    }
  std::printf("  unmatched renewable-table states: %d\n", in.base_unmatched_states);
  check(in.base_unmatched_states == 0, "every renewable-table state has a grid region");
  bool labelled = !in.base_source_notes.empty();
  for (const std::string& n : in.base_source_notes) {
    std::printf("    %s\n", n.c_str());
    labelled &= n.rfind("Primary: ", 0) == 0 || n.rfind("Secondary: ", 0) == 0;
  }
  check(labelled, "base-mix provenance lines, each primary or secondary");
  return fails;
}

// Checks shared by the build-limit tests.
struct Checker {
  int fails = 0;
  void operator()(bool ok, const std::string& what) {
    std::printf("  %-74s %s\n", what.c_str(), ok ? "ok" : "FAIL");
    if (!ok) ++fails;
  }
};

// The national build cap whose row sums source s.
const BuildCap* cap_with(const std::vector<BuildCap>& caps, int s, size_t* index = nullptr) {
  for (size_t i = 0; i < caps.size(); ++i)
    for (int t : caps[i].techs)
      if (t == s) {
        if (index) *index = i;
        return &caps[i];
      }
  return nullptr;
}

double group_total(const AllocationResult& a, const BuildCap& c) {
  double g = 0;
  for (int t : c.techs) g += a.national[t];
  return g;
}

// The published limits read from build_limits.csv: every model source except firmed RE
// (solar and wind with storage, which uses their limits) has rows, cases are ordered,
// and the inputs hold the cumulative TWh from FY2023-24 to 2047 for each case.
int published_limits_loaded(const AllocationInputs& in) {
  Checker check;
  std::printf("published_limits_loaded\n");
  check(in.rate_last_year == 2023, "build limits start from the FY2023-24 base (rate_last_year 2023)");
  std::printf("  %-14s %10s %10s %10s %10s %10s\n", "source", "base TWh", "low", "central", "high", "NPP rec.");
  for (int s = 0; s < kTechs; ++s) {
    double base = 0;
    for (int r = 0; r < kRegions; ++r) base += in.base_twh[r][s];
    std::printf("  %-14s %10.1f %10.1f %10.1f %10.1f %10.1f\n", tech_key(s), in.national_last_twh[s],
                in.published_build_twh[kBuildLow][s], in.published_build_twh[kBuildCentral][s],
                in.published_build_twh[kBuildHigh][s], in.max_annual_increase[s]);
    check(std::abs(in.national_last_twh[s] - base) < 1e-9,
          std::string(tech_key(s)) + ": national base = FY2023-24 base mix summed over regions");
    if (s == kFirmRE) {
      check(!in.has_published_build[s], "firmed_re: no rows of its own (uses the solar and wind limits)");
      continue;
    }
    check(in.has_published_build[s], std::string(tech_key(s)) + ": published rows");
    const auto& v = in.published_build_twh;
    check(v[kBuildLow][s] >= 0 && v[kBuildLow][s] <= v[kBuildCentral][s] && v[kBuildCentral][s] <= v[kBuildHigh][s],
          std::string(tech_key(s)) + ": 0 <= low <= central <= high");
    for (int k = 0; k < kBuildCases; ++k) {
      const double want = published_build_twh(in.build_limits, s, k, in.rate_last_year, 2047);
      check(std::abs(v[k][s] - want) < 1e-9, std::string(tech_key(s)) + " " + build_case_name(k) +
                                                 ": cumulative TWh from the rows, FY2024-25 to 2047");
    }
  }
  // Solar central by hand: 32 GW/yr for 2026-31 (and the two years before the file starts),
  // 50 for 2032-39, 90 for 2040-47, all at 22% CUF.
  const double solar = (32.0 * 8 + 50.0 * 8 + 90.0 * 8) * 0.22 * 8.76;
  check(std::abs(in.published_build_twh[kBuildCentral][kSolar] - solar) < 1e-6,
        "solar central = (32 x 8 + 50 x 8 + 90 x 8) GW x 0.22 x 8.76");
  check(in.max_annual_increase[kHydro] > 0 && in.max_annual_increase[kNuclear] > 0 &&
            in.max_annual_increase[kCoal] + in.max_annual_increase[kGas] > 0 &&
            in.max_annual_increase[kSolar] + in.max_annual_increase[kWind] + in.max_annual_increase[kBio] > 0,
        "NPP record annual increases read (hydro, nuclear, thermal, renewables)");
  int lines = 0;
  for (const std::string& n : in.notes)
    if (n.rfind("Build limits", 0) == 0 || n.rfind("Build-limit base", 0) == 0 || n.rfind("Record annual increase", 0) == 0) {
      std::printf("    %s\n", n.c_str());
      ++lines;
    }
  check(lines == 3, "provenance notes: published limits, their base, NPP record cross-check");
  return check.fails;
}

// A low published solar limit binds: the solar group (solar, onshore wind and firmed RE,
// which is solar and wind with storage) generates exactly its cap, the cap's dual is
// positive, and the LP stays feasible under the default policy.
int build_limit_binds(const AllocationInputs& in0, const std::array<double, kRegions>& req) {
  Checker check;
  std::printf("build_limit_binds\n");
  AllocationInputs in = in0;
  PolicySettings p;
  in.published_build_twh[p.build_case][kSolar] = 1000.0;  // well below the central 2,652 TWh
  in.published_build_twh[p.build_case][kWind] = 0.0;
  const std::vector<BuildCap> caps = build_caps(in, p);
  size_t i = 0;
  const BuildCap* c = cap_with(caps, kSolar, &i);
  check(c != nullptr && cap_with(caps, kWind) == c && cap_with(caps, kFirmRE) == c,
        "solar, onshore wind and firmed RE share one cap");
  if (!c) return check.fails + 1;
  const double base = in.national_last_twh[kSolar] + in.national_last_twh[kWind];
  check(std::abs(c->cap_twh - (base + 1000.0)) < 1e-9, "cap = base solar + base wind + 1 x published");
  const AllocationResult a = solve_least_cost(in, p, req);
  std::printf("  %s; solar group %.3f TWh, cap %.3f TWh, dual %.2f USD/MWh\n", a.status.c_str(),
              a.ok ? group_total(a, *c) : 0.0, c->cap_twh, a.ok ? a.build_cap_dual[i] : 0.0);
  check(a.ok, "LP feasible");
  if (!a.ok) return check.fails;
  check(std::abs(group_total(a, *c) - c->cap_twh) < 1e-6 * c->cap_twh, "solar group total equals the cap");
  check(a.build_cap_dual.size() == caps.size() && a.build_cap_dual[i] > 0, "binding cap has a positive dual");
  // The multiplier scales the published part only.
  PolicySettings p2 = p;
  p2.build_accel = 1.5;
  check(std::abs(cap_with(build_caps(in, p2), kSolar)->cap_twh - (base + 1500.0)) < 1e-9,
        "build_accel 1.5: cap = base + 1.5 x published");
  // Every cap of the default plan holds.
  const AllocationResult d = solve_least_cost(in0, p, req);
  const std::vector<BuildCap> caps0 = build_caps(in0, p);
  bool within = d.ok;
  for (const BuildCap& k : caps0) within &= group_total(d, k) <= k.cap_twh * (1 + 1e-9) + 1e-6;
  check(within, "default plan: every group within its cap");
  return check.fails;
}

// A published limit of 0, or one below base (a negative limit, or a multiplier of 0),
// still allows base-year generation: cap = max(start, start + accel x limit), where the start
// is max(existing-fleet floor, FY2023-24) and so never below base (renewables: the base).
int build_limit_below_base_feasible(const AllocationInputs& in0, const std::array<double, kRegions>& req) {
  Checker check;
  std::printf("build_limit_below_base_feasible\n");
  // Only the caps can bind: no share or firm floors.
  PolicySettings p;
  p.nonfossil_min = 0;
  p.firm_national = 0;
  p.firm_regional = 0;
  auto start_of = [&](int s) { return std::max(in0.national_last_twh[s], in0.fleet_floor_twh[s]); };
  for (double limit : {0.0, -500.0}) {
    AllocationInputs in = in0;
    for (auto& v : in.published_build_twh) v.fill(limit);
    const std::vector<BuildCap> caps = build_caps(in, p);
    bool at_start = !caps.empty();
    for (const BuildCap& c : caps)
      at_start &= std::abs(c.cap_twh - c.start_twh) < 1e-9 && c.start_twh >= c.base_twh;
    char what[96];
    std::snprintf(what, sizeof what, "limit %.0f TWh: every cap equals its start (>= its base)", limit);
    check(at_start, what);
    const AllocationResult a = solve_least_cost(in, p, req);
    std::snprintf(what, sizeof what, "limit %.0f TWh: LP feasible (%s)", limit, a.status.c_str());
    check(a.ok, what);
    if (!a.ok) continue;
    for (int s : {kSolar, kWind, kNuclear, kHydro}) {
      std::snprintf(what, sizeof what, "limit %.0f TWh: %s at its start %.2f TWh (base %.2f)", limit, tech_key(s),
                    start_of(s), in.national_last_twh[s]);
      check(std::abs(a.national[s] - start_of(s)) < 1e-6 * std::max(1.0, start_of(s)), what);
    }
  }
  PolicySettings z = p;
  z.build_accel = 0;
  bool at_start = true;
  for (const BuildCap& c : build_caps(in0, z)) at_start &= std::abs(c.cap_twh - c.start_twh) < 1e-9;
  check(at_start, "build_accel 0: every cap equals its start");
  check(solve_least_cost(in0, z, req).ok, "build_accel 0: LP feasible");
  // Default policy, solar limit 0: still feasible, solar at or above its base.
  AllocationInputs in = in0;
  const PolicySettings d;
  in.published_build_twh[d.build_case][kSolar] = 0;
  const AllocationResult a = solve_least_cost(in, d, req);
  check(a.ok && a.national[kSolar] >= in.national_last_twh[kSolar] - 1e-6,
        "default policy, solar limit 0: LP feasible, solar >= base");
  return check.fails;
}

// A period ending in 2040 still allows growth in 2041-2047: the last period's rate is held
// past its end. Years before the first period (FY2024-25, FY2025-26 in the real file) take
// the first period's rate.
int build_limit_holds_last_period(const std::string& tmp_dir, const AllocationInputs& in0,
                                  const std::array<double, kRegions>& req) {
  Checker check;
  std::printf("build_limit_holds_last_period\n");
  const std::string path = tmp_dir + "/e47_build_limits_test.csv";
  {
    std::ofstream f(path);
    f << "tech,period_start,period_end,gw_per_year_low,gw_per_year_central,gw_per_year_high,cuf,source_name,"
         "source_url,source_location,notes\n"
         "solar,2026,2040,5,10,20,0.2,test,,,\n"
         "wind,2026,2030,1,2,3,0.25,test,,,\n"
         "wind,2031,2040,2,4,6,0.30,test,,,\n"
         "pumped_storage,2026,2047,1,1,1,,test,,,\n";
  }
  const std::vector<BuildLimitRow> rows = read_build_limits(path);
  std::filesystem::remove(path);
  check(rows.size() == 3, "three model-source rows read (pumped storage skipped)");
  const double r = 10 * 0.2 * 8.76;  // central solar, TWh a year added per build year
  const double to2040 = published_build_twh(rows, kSolar, kBuildCentral, 2023, 2040);
  const double to2047 = published_build_twh(rows, kSolar, kBuildCentral, 2023, 2047);
  std::printf("  solar central: to 2040 %.3f TWh, to 2047 %.3f TWh (%.3f a build year)\n", to2040, to2047, r);
  check(std::abs(to2040 - 17 * r) < 1e-9, "to 2040: 17 build years (2024-2040) x 10 GW x 0.2 x 8.76");
  check(std::abs(to2047 - 24 * r) < 1e-9, "to 2047: 24 build years, 2041-2047 at the 2026-2040 rate");
  const double wind = published_build_twh(rows, kWind, kBuildHigh, 2023, 2047);
  check(std::abs(wind - (7 * 3 * 0.25 + 17 * 6 * 0.30) * 8.76) < 1e-9,
        "wind high: 2024-2030 at the first period's rate, 2031-2047 at the last");
  check(published_build_twh(rows, kNuclear, kBuildCentral, 2023, 2047) == 0, "a source with no rows adds 0");
  // In the LP: the solar group can grow to base + the 2047 value, beyond the 2040 value.
  AllocationInputs in = in0;
  PolicySettings p;
  in.published_build_twh[p.build_case][kSolar] = to2047;
  in.published_build_twh[p.build_case][kWind] = 1000.0;
  const AllocationResult a = solve_least_cost(in, p, req);
  const std::vector<BuildCap> caps = build_caps(in, p);
  const BuildCap* c = cap_with(caps, kSolar);
  const double base = in.national_last_twh[kSolar] + in.national_last_twh[kWind];
  std::printf("  LP %s: solar group %.3f TWh, cap %.3f TWh\n", a.status.c_str(), a.ok && c ? group_total(a, *c) : 0.0,
              c ? c->cap_twh : 0.0);
  check(a.ok && c &&std::abs(group_total(a, *c) - (base + 1000.0 + to2047)) < 1e-6 * (base + 1000.0 + to2047),
        "LP: solar group reaches base + wind + solar to 2047 (growth in 2041-2047 allowed)");
  return check.fails;
}

// Existing fleets are not frozen at their FY2023-24 output: the cap of each conventional
// source starts from its highest NPP year over the complete fiscal years FY2018-19 to
// FY2025-26 (primary), or from FY2023-24 if that is higher. Hydro and nuclear: NPP regional
// generation (Sub-report 2) summed over the five regions. Coal and gas: NPP station sums by
// state (Sub-report 16); gas with diesel, as in the base mix. NPP hydro is stations above
// 25 MW, so the hydro floor adds the base small hydro (CEA). Renewables keep the FY2023-24
// base. The NPP maxima are computed here from the CSV files, not taken from the loader.
int fleet_floor_from_npp_history(const std::string& root, const AllocationInputs& in) {
  Checker check;
  std::printf("fleet_floor_from_npp_history\n");
  constexpr int kFirst = 2018, kLast = 2025;  // FY2018-19 .. FY2025-26; FY2026-27 is partial
  std::map<int, std::array<double, kTechs>> by_year;
  double gas_only_max = 0;
  {
    const CsvTable t = CsvTable::read(root + "/data/raw/npp/npp_region_generation_fy.csv");
    for (size_t i = 0; i < t.rows(); ++i) {
      const int y = static_cast<int>(t.num(i, t.col("fy_start_year")));
      const std::string s = t.str(i, t.col("source"));
      const int k = s == "hydro" ? kHydro : s == "nuclear" ? kNuclear : -1;
      // All India (subtotal) and Bhutan import are not one of the five regions.
      if (y < kFirst || y > kLast || k < 0 || region_of(t.str(i, t.col("region"))) < 0 ||
          t.str(i, t.col("partial")) == "True")
        continue;
      by_year[y][k] += t.num(i, t.col("generation_gwh")) / 1000.0;
    }
  }
  {
    std::map<int, double> gas_only;
    const CsvTable t = CsvTable::read(root + "/data/raw/npp/npp_state_generation_fy.csv");
    for (size_t i = 0; i < t.rows(); ++i) {
      const int y = static_cast<int>(t.num(i, t.col("fy_start_year")));
      const std::string s = t.str(i, t.col("source"));
      const int k = s == "coal_lignite" ? kCoal : s == "gas" || s == "diesel" ? kGas : -1;
      const double v = t.num(i, t.col("generation_gwh"));
      if (y < kFirst || y > kLast || k < 0 || region_of(t.str(i, t.col("region"))) < 0 ||
          t.str(i, t.col("partial")) == "True" || std::isnan(v))
        continue;
      by_year[y][k] += v / 1000.0;
      if (s == "gas") gas_only[y] += v / 1000.0;
    }
    for (const auto& [y, v] : gas_only) gas_only_max = std::max(gas_only_max, v);
  }
  check(static_cast<int>(by_year.size()) == kLast - kFirst + 1, "eight complete NPP fiscal years, FY2018-19 to FY2025-26");
  std::array<double, kTechs> npp_max{};
  std::array<int, kTechs> npp_year{};
  for (const auto& [y, v] : by_year)
    for (int s : {kCoal, kGas, kNuclear, kHydro})
      if (v[s] > npp_max[s]) npp_max[s] = v[s], npp_year[s] = y;
  double small_hydro = 0;
  for (int r = 0; r < kRegions; ++r) small_hydro += in.base_res_twh[r][kHydro];
  std::array<double, kTechs> floor = npp_max;
  floor[kHydro] += small_hydro;  // NPP hydro is stations above 25 MW; small hydro is in the base
  for (int s : {kCoal, kGas, kNuclear, kHydro})
    std::printf("  %-8s NPP max %8.2f TWh (FY%d-%02d)%s, FY2023-24 base %8.2f TWh\n", tech_key(s), npp_max[s],
                npp_year[s], (npp_year[s] + 1) % 100,
                s == kHydro ? (" + small hydro " + std::to_string(small_hydro)).c_str() : "", in.national_last_twh[s]);
  std::printf("  gas alone (no diesel) NPP max %.2f TWh\n", gas_only_max);

  // build_accel 0: each cap is its starting level, max(fleet floor, FY2023-24).
  PolicySettings z;
  z.build_accel = 0;
  const std::vector<BuildCap> caps0 = build_caps(in, z);
  for (int s : {kCoal, kGas, kNuclear, kHydro}) {
    const BuildCap* c = cap_with(caps0, s);
    const double want = std::max(floor[s], in.national_last_twh[s]);
    char what[128];
    std::snprintf(what, sizeof what, "build_accel 0: %s cap %.2f = max(NPP fleet floor, FY2023-24) %.2f TWh", tech_key(s),
                  c ? c->cap_twh : 0.0, want);
    check(c && std::abs(c->cap_twh - want) < 1e-6 * want, what);
  }
  for (int s : {kSolar, kOffshore, kBio}) {
    const BuildCap* c = cap_with(caps0, s);
    double base = 0;
    if (c)
      for (int t : c->techs) base += in.national_last_twh[t];
    check(c && std::abs(c->cap_twh - base) < 1e-9 * std::max(1.0, base),
          std::string("build_accel 0: ") + (c ? c->name : tech_key(s)) + " cap = FY2023-24 base (renewables keep it)");
  }
  // Default policy (central case, 1x): the gas cap (no published gas additions) is at least
  // the NPP max gas year; the hydro cap is at least the NPP max hydro year + base small hydro.
  const PolicySettings d;
  const std::vector<BuildCap> caps = build_caps(in, d);
  const BuildCap* gas = cap_with(caps, kGas);
  const BuildCap* hydro = cap_with(caps, kHydro);
  std::printf("  default policy: gas cap %.2f TWh, hydro cap %.2f TWh\n", gas ? gas->cap_twh : 0.0,
              hydro ? hydro->cap_twh : 0.0);
  check(gas && gas->cap_twh >= npp_max[kGas] - 1e-9 && npp_max[kGas] >= gas_only_max,
        "gas cap >= NPP max gas year (station sums, with diesel as in the base)");
  check(hydro && hydro->cap_twh >= npp_max[kHydro] + small_hydro - 1e-9,
        "hydro cap >= NPP max hydro year + base small hydro");
  check(hydro && std::abs(hydro->cap_twh - (floor[kHydro] + in.published_build_twh[d.build_case][kHydro])) <
                     1e-6 * hydro->cap_twh,
        "hydro cap = fleet floor + 1 x central published additions");
  // Provenance: one primary note naming each floor and its year.
  bool note = false;
  for (const std::string& n : in.notes) {
    if (n.rfind("Existing-fleet floor", 0) != 0) continue;
    std::printf("    %s\n", n.c_str());
    bool all = n.find("primary") != std::string::npos && n.find("small hydro") != std::string::npos;
    for (int s : {kCoal, kGas, kNuclear, kHydro}) {
      char fy[16];
      std::snprintf(fy, sizeof fy, "FY%d-%02d", npp_year[s], (npp_year[s] + 1) % 100);
      all &= n.find(fy) != std::string::npos;
    }
    note = all;
  }
  check(note, "provenance note: each floor with its NPP year, labelled primary, small hydro scope stated");
  return check.fails;
}

// The lowest build multiplier the dashboard offers (0.75x) on the low case stays feasible
// under the default policy at the default median requirement. The non-fossil row is hard
// (unmet requirement cannot relax it): with the coal floor at 0.6 it needs about 1,813 TWh of
// non-fossil generation, more than the low case allows at 0.5x.
int default_policy_low_case_feasible(const std::string& root) {
  Checker check;
  std::printf("default_policy_low_case_feasible\n");
  const Pipeline pl(root, CoreModel::kIndia);
  ScenarioControls c;  // default scenario; plans for the median (requirement_quantile 0.5)
  c.draws = 1u << 18;  // CPU; the median is within Monte Carlo noise of the default draws
  c.use_gpu = false;
  const ForecastResult f = pl.forecast(c);
  PolicySettings p;
  p.build_case = kBuildLow;
  p.build_accel = 0.75;
  const StrategyResult s = pl.strategy(f, c, p);
  double req = 0, unmet = 0;
  for (int r = 0; r < kRegions; ++r) {
    req += s.requirement[r];
    unmet += s.least_cost.unserved[r];
  }
  const AllocationResult& a = s.least_cost;
  std::printf("  median requirement %.1f TWh; low case 0.75x: %s, non-fossil %.1f%%, unmet %.1f TWh\n", req,
              a.status.c_str(), a.ok ? 100 * a.nonfossil_share() : 0.0, a.ok ? unmet : 0.0);
  check(c.requirement_quantile == 0.5 && req > 0, "default scenario, median requirement");
  check(a.ok, "default policy, low case, 0.75x published: LP feasible");
  // For the record (not checked): the old slider minimum, and the lowest feasible multiplier.
  PolicySettings q = p;
  q.build_accel = 0.5;
  const AllocationInputs& in = pl.allocation_inputs();
  const bool half_ok = solve_least_cost(in, q, s.requirement).ok;
  std::printf("  low case 0.50x (old slider minimum): %s\n", half_ok ? "feasible" : "infeasible");
  if (!half_ok && a.ok) {
    double lo = 0.5, hi = 0.75;
    while (hi - lo > 1e-4) {
      q.build_accel = 0.5 * (lo + hi);
      (solve_least_cost(in, q, s.requirement).ok ? hi : lo) = q.build_accel;
    }
    std::printf("  low case: lowest feasible multiplier %.4fx\n", hi);
  }
  return check.fails;
}

// Emission factors come from the latest period of the CEA station-weighted rates in
// emission_factors.csv (CEA CO2 Baseline Database v22, FY2025-26), whatever the row order.
int emission_factor_latest_period(const std::string& root, const std::string& tmp_dir, const AllocationInputs& in) {
  Checker check;
  std::printf("emission_factor_latest_period\n");
  // Latest period of the coal and gas station-weighted rows, read here from the file itself.
  std::string latest;
  std::map<std::string, std::map<std::string, double>> by_period;  // period -> fuel -> tCO2/MWh
  {
    const CsvTable t = CsvTable::read(root + "/data/external/techno_economic/emission_factors.csv");
    for (size_t i = 0; i < t.rows(); ++i) {
      if (t.str(i, t.col("metric")) != "station_weighted_avg_emission_rate") continue;
      const std::string fuel = t.str(i, t.col("fuel")), period = t.str(i, t.col("period"));
      if (fuel != "coal" && fuel != "gas") continue;
      by_period[period][fuel] = t.num(i, t.col("value"));
      latest = std::max(latest, period);
    }
  }
  const auto& want = by_period[latest];
  std::printf("  latest period %s: coal %.3f, gas %.3f tCO2/MWh; loaded %s: coal %.3f, gas %.3f\n", latest.c_str(),
              want.count("coal") ? want.at("coal") : kNaN, want.count("gas") ? want.at("gas") : kNaN,
              in.emission_period.c_str(), in.emission_t_per_mwh[kCoal], in.emission_t_per_mwh[kGas]);
  check(latest == "2025-26", "latest station-weighted period in the file is 2025-26 (CEA database v22)");
  check(in.emission_period == latest, "loaded emission factors are from the latest period");
  check(want.count("coal") && in.emission_t_per_mwh[kCoal] == want.at("coal") && want.at("coal") == 0.971,
        "coal 0.971 tCO2/MWh (v22, FY2025-26)");
  check(want.count("gas") && in.emission_t_per_mwh[kGas] == want.at("gas") && want.at("gas") == 0.47,
        "gas 0.470 tCO2/MWh (v22, FY2025-26)");
  bool zero = true;
  for (int s = 0; s < kTechs; ++s)
    if (!tech_is_fossil(s)) zero &= in.emission_t_per_mwh[s] == 0.0;
  check(zero, "non-fossil sources emit nothing at the stack");
  bool note = false;
  for (const std::string& n : in.notes)
    note |= n.rfind("Emission factors", 0) == 0 && n.find("2025-26") != std::string::npos &&
            n.find("Version 22.0") != std::string::npos && n.find("secondary") != std::string::npos;
  check(note, "provenance note names the period, the database version and the source status");

  // Row order and other metrics do not matter: the latest station-weighted period wins.
  const std::string path = tmp_dir + "/e47_emission_factors_test.csv";
  {
    std::ofstream f(path);
    f << "fuel,metric,period,value,unit,basis\n"
         "coal,station_weighted_avg_emission_rate,2023-24,0.90,t,actual\n"
         "grid,grid_weighted_average_incl_RES,2031-32,0.43,t,projection\n"
         "coal,station_weighted_avg_emission_rate,2025-26,0.95,t,actual\n"
         "gas,station_weighted_avg_emission_rate,2024-25,0.40,t,actual\n"
         "gas,station_weighted_avg_emission_rate,2025-26,0.45,t,actual\n"
         "lignite,station_weighted_avg_emission_rate,2026-27,1.30,t,actual\n"
         "coal,station_weighted_avg_emission_rate,2024-25,0.92,t,actual\n"
         "coal (Indian),fuel_emission_factor_gcv,database v21,90.6,g,assumption\n";
  }
  const StationEmissionFactors ef = read_station_emission_factors(path);
  check(ef.period == "2025-26" && ef.coal == 0.95 && ef.gas == 0.45,
        "shuffled file: 2025-26 coal and gas picked (lignite and grid rows ignored)");
  {
    // Later rows that are not reported values for a fiscal year: a projection, and a period
    // that is not a fiscal year. Both have coal and gas, and both must be ignored.
    std::ofstream f(path, std::ios::app);
    f << "coal,station_weighted_avg_emission_rate,2031-32,0.80,t,projection\n"
         "gas,station_weighted_avg_emission_rate,2031-32,0.40,t,projection\n"
         "coal,station_weighted_avg_emission_rate,database v23,0.70,t,actual\n"
         "gas,station_weighted_avg_emission_rate,database v23,0.30,t,actual\n";
  }
  const StationEmissionFactors ep = read_station_emission_factors(path);
  check(ep.period == "2025-26" && ep.coal == 0.95 && ep.gas == 0.45,
        "projected 2031-32 rows and a non-fiscal-year period ignored: still 2025-26");
  {
    std::ofstream f(path, std::ios::app);
    f << "coal,station_weighted_avg_emission_rate,2026-27,0.99,t,actual\n";  // a period with coal but no gas
  }
  bool threw = false;
  try {
    read_station_emission_factors(path);
  } catch (const std::exception&) {
    threw = true;
  }
  check(threw, "latest period without a gas row: error, not a mix of periods");
  {
    std::ofstream f(path);  // no basis column: actual and projected rows cannot be told apart
    f << "fuel,metric,period,value,unit\n"
         "coal,station_weighted_avg_emission_rate,2025-26,0.95,t\n"
         "gas,station_weighted_avg_emission_rate,2025-26,0.45,t\n";
  }
  threw = false;
  try {
    read_station_emission_factors(path);
  } catch (const std::exception&) {
    threw = true;
  }
  check(threw, "file without a basis column: error");
  std::filesystem::remove(path);
  return check.fails;
}

// CEA-basis levelised cost: CRF(wacc, life) x capex / (CUF x 8,760) + fixed O&M + fuel, in
// Rs/kWh, then USD/MWh at the RBI rate. Known cases first, then the loaded table.
int cea_lcoe_formula(const std::string& root, const AllocationInputs& in) {
  Checker check;
  std::printf("cea_lcoe_formula\n");
  check(std::abs(capital_recovery_factor(0.10, 1) - 1.1) < 1e-12, "CRF(10%, 1 year) = 1.1");
  check(std::abs(capital_recovery_factor(0.0, 20) - 0.05) < 1e-12, "CRF(0, 20 years) = 1/20");
  // 8.76 crore per MW at CUF 1: capital per kWh = 10 x CRF; O&M 1% = 0.1 Rs/kWh; fuel 2.
  const double a = cea_lcoe_rs_per_kwh(8.76, 1.0, 20, 1.0, 2.0, 0.0);
  const double b = cea_lcoe_rs_per_kwh(8.76, 1.0, 1, 1.0, 2.0, 0.10);
  std::printf("  known cases: %.12f (want 2.6), %.12f (want 13.1) Rs/kWh\n", a, b);
  check(std::abs(a - 2.6) < 1e-9, "8.76 Cr/MW, 1% O&M, 20 y, CUF 1, fuel 2, 0%: 0.5 + 0.1 + 2 = 2.6 Rs/kWh");
  check(std::abs(b - 13.1) < 1e-9, "8.76 Cr/MW, 1% O&M, 1 y, CUF 1, fuel 2, 10%: 11 + 0.1 + 2 = 13.1 Rs/kWh");

  // Inputs read here from the files themselves.
  std::map<std::string, double> basis;
  {
    const CsvTable t = CsvTable::read(root + "/data/external/techno_economic/cea_cost_basis.csv");
    for (size_t i = 0; i < t.rows(); ++i) basis[t.str(i, t.col("parameter"))] = t.num(i, t.col("value"));
  }
  const double wacc = 0.01 * (basis["debt_share"] * 0.01 * basis["interest_rate"] +
                              (1.0 - 0.01 * basis["debt_share"]) * basis["return_on_equity_pretax"]);
  const double inr = basis["inr_per_usd"];
  std::printf("  pre-tax WACC %.5f (loaded %.5f); Rs per USD %.4f (loaded %.4f)\n", wacc, in.cea_wacc, inr,
              in.cea_inr_per_usd);
  check(std::abs(wacc - 0.12585) < 1e-12 && std::abs(in.cea_wacc - wacc) < 1e-12,
        "pre-tax WACC = 0.70 x 10.71% + 0.30 x 16.96% = 12.585% (CERC)");
  check(inr == 74.5039 && in.cea_inr_per_usd == inr, "Rs 74.5039 per USD (RBI, FY2021-22 average)");
  struct Row {
    double capex, om, life, cuf;
  };
  std::map<std::string, Row> nep;  // latest year_basis row of each source
  {
    const CsvTable t = CsvTable::read(root + "/data/external/techno_economic/cea_costs.csv");
    std::map<std::string, std::string> year;
    for (size_t i = 0; i < t.rows(); ++i) {
      const std::string tech = t.str(i, t.col("tech")), y = t.str(i, t.col("year_basis"));
      if (year.count(tech) && year[tech] >= y) continue;
      year[tech] = y;
      nep[tech] = {t.num(i, t.col("capex_cr_per_mw")), t.num(i, t.col("fixed_om_pct")), t.num(i, t.col("life_years")),
                   t.num(i, t.col("cuf"))};
    }
  }
  const Row& c = nep["coal"];
  const double coal = cea_lcoe_rs_per_kwh(c.capex, c.om, c.life, c.cuf, basis["coal_fuel_cost"], wacc) * 1000.0 / inr;
  const Row& s = nep["solar"];
  const double solar = cea_lcoe_rs_per_kwh(s.capex, s.om, s.life, s.cuf, 0.0, wacc) * 1000.0 / inr;
  std::printf("  coal %.4f USD/MWh (loaded %.4f); solar (Rs %.2f Cr/MW, the 2029-30 row) %.4f (loaded %.4f)\n", coal,
              in.cea_lcoe_usd_per_mwh[kCoal], s.capex, solar, in.cea_lcoe_usd_per_mwh[kSolar]);
  check(s.capex == 4.1, "solar uses the latest NEP row (Rs 4.1 Cr/MW, 2029-30)");
  check(std::abs(in.cea_lcoe_usd_per_mwh[kCoal] - coal) < 1e-9, "coal: NEP capex, O&M, life, CUF + CEA fuel 2.4 Rs/kWh");
  check(std::abs(in.cea_lcoe_usd_per_mwh[kSolar] - solar) < 1e-9, "solar: NEP capex, O&M, life, CUF, no fuel");
  const double bio_fuel = 4200.0 / basis["biomass_gcv"] * basis["biomass_fuel_price"] / 1000.0;
  const Row& o = nep["bioenergy"];
  const double bio = cea_lcoe_rs_per_kwh(o.capex, o.om, o.life, o.cuf, bio_fuel, wacc) * 1000.0 / inr;
  check(std::abs(in.cea_lcoe_usd_per_mwh[kBio] - bio) < 1e-9, "bioenergy: fuel = 4200 kcal/kWh / 3100 kcal/kg x CERC price");
  bool flat = true, solar_scaled = true, kept = true;
  for (int r = 0; r < kRegions; ++r) {
    flat &= std::abs(in.cost_cea[r][kCoal] - coal) < 1e-9;
    // Regional resource quality as in the IEA basis: same CUF scaling, so the ratio is constant.
    solar_scaled &= std::abs(in.cost_cea[r][kSolar] / in.cost[r][kSolar] - in.cost_cea[0][kSolar] / in.cost[0][kSolar]) < 1e-12;
    solar_scaled &= std::abs(in.cost_cea[r][kWind] / in.cost[r][kWind] - in.cost_cea[0][kWind] / in.cost[0][kWind]) < 1e-12;
    kept &= in.cost_cea[r][kNuclear] == in.cost[r][kNuclear] && in.cost_cea[r][kGas] == in.cost[r][kGas];
  }
  check(flat, "coal: one CEA-basis cost in every region");
  check(solar_scaled, "solar and wind: regional CUF scaling as in the IEA basis");
  check(kept, "nuclear and gas: no NEP capex, so they keep the IEA cost");
  std::printf("  %-14s %10s %10s\n", "source", "IEA", "CEA");
  for (int k = 0; k < kTechs; ++k) {
    double iea = 0, cea = 0;
    for (int r = 0; r < kRegions; ++r) {
      iea += in.cost[r][k] / kRegions;
      cea += in.cost_cea[r][k] / kRegions;
    }
    std::printf("  %-14s %10.2f %10.2f   USD/MWh, mean over regions%s\n", tech_key(k), iea, cea,
                in.cea_from_iea[k] ? " (IEA kept)" : "");
  }
  bool note = false, nominal = false;
  for (const std::string& n : in.notes) {
    if (n.rfind("CEA cost basis", 0) != 0) continue;
    note |= n.find("nuclear and gas keep the IEA") != std::string::npos;
    nominal |= n.find("level nominal annuity") != std::string::npos && n.find("real 2021-22") != std::string::npos;
  }
  check(note, "provenance note flags nuclear and gas kept at IEA cost");
  check(nominal, "provenance note discloses nominal CERC rates on real NEP capex");
  return check.fails;
}

// The sensitivity cases run on copies: the policy passed in, a fresh default policy and the
// Robust preset are the same before and after; each case changes one setting of the copy.
int sensitivity_from_copy(const AllocationInputs& in, const std::array<double, kRegions>& req) {
  Checker check;
  std::printf("sensitivity_from_copy\n");
  const PolicySettings defaults;
  check(defaults.cost_basis == kCostIEA && defaults.solar_potential_set == kSolarNise2025 && defaults.wind_height == 150,
        "defaults: IEA costs, NISE 2025 solar, 150 m wind");
  PolicySettings robust;
  for (const auto& pp : policy_presets())
    if (pp.name == "Robust") pp.apply(robust);
  const PolicySettings robust_before = robust;
  std::array<double, kRegions> req90 = req;
  for (double& x : req90) x *= 1.3;
  const double default_cost_before = solve_least_cost(in, PolicySettings{}, req).cost_busd;
  const AllocationResult direct = solve_least_cost(in, robust, req);

  const std::vector<MixSensitivityRow> rows = mix_sensitivity(in, robust, req, req90);

  check(robust == robust_before, "policy passed in unchanged");
  check(PolicySettings{} == defaults, "fresh default policy unchanged");
  PolicySettings robust_again;
  for (const auto& pp : policy_presets())
    if (pp.name == "Robust") pp.apply(robust_again);
  check(robust_again == robust_before, "Robust preset unchanged");
  check(std::abs(solve_least_cost(in, PolicySettings{}, req).cost_busd - default_cost_before) < 1e-9,
        "default plan cost unchanged after the cases");
  const char* names[] = {"base", "cost_basis_cea", "solar_potential_nise2014", "wind_120m", "build_low", "build_central"};
  bool named = rows.size() == 6;
  for (size_t i = 0; named && i < rows.size(); ++i) named &= rows[i].name == names[i];
  check(named, "six cases in order: base, cost_basis_cea, solar_potential_nise2014, wind_120m, build_low, build_central");
  if (!named) return check.fails;
  auto with = [&](void (*f)(PolicySettings&)) {
    PolicySettings p = robust_before;
    f(p);
    return p;
  };
  check(rows[0].policy == robust_before, "base: the policy as passed");
  check(rows[1].policy == with([](PolicySettings& p) { p.cost_basis = kCostCEA; }), "cost_basis_cea: cost_basis only");
  check(rows[2].policy == with([](PolicySettings& p) { p.solar_potential_set = kSolarNise2014; }),
        "solar_potential_nise2014: solar_potential_set only");
  check(rows[3].policy == with([](PolicySettings& p) { p.wind_height = 120; }), "wind_120m: wind_height only");
  check(rows[4].policy == with([](PolicySettings& p) { p.build_case = kBuildLow; }), "build_low: build_case low only");
  check(rows[5].policy == with([](PolicySettings& p) { p.build_case = kBuildCentral; }),
        "build_central: build_case central only");
  check(robust_before.build_case == kBuildHigh && !(rows[5].policy == rows[0].policy),
        "Robust is on the high case, so build_central differs from base");
  bool ok = true;
  for (const MixSensitivityRow& r : rows) {
    ok &= r.plan.ok && r.plan_p90.ok;
    std::printf("  %-26s non-fossil %5.1f%%  coal %5.1f%%  CO2 %7.1f Mt  cost %7.2f bn USD  P90 unmet %7.1f TWh\n",
                r.name.c_str(), 100 * r.plan.nonfossil_share(), 100 * r.plan.share(kCoal), r.plan.co2_mt,
                r.plan.cost_busd, r.plan_p90.unserved_total());
  }
  check(ok, "every case solves at the median and the P90 requirement");
  check(std::abs(rows[0].plan.cost_busd - direct.cost_busd) < 1e-9 && std::abs(rows[0].plan.co2_mt - direct.co2_mt) < 1e-9,
        "base row = the policy solved directly");
  double cea_cost = 0;
  for (int r = 0; r < kRegions; ++r)
    for (int s = 0; s < kTechs; ++s) cea_cost += rows[1].plan.gen[r][s] * in.cost_cea[r][s] * 1e-3;
  check(std::abs(rows[1].plan.cost_busd - cea_cost) < 1e-9 * cea_cost, "cost_basis_cea: plan priced at the CEA-basis costs");
  const Bounds b2 = make_bounds(in, rows[2].policy, req), b3 = make_bounds(in, rows[3].policy, req);
  bool solar = true, wind = true;
  for (int r = 0; r < kRegions; ++r) {
    solar &= b2.hi[r][kSolar] == std::max(in.base_twh[r][kSolar], robust.solar_land * in.solar_potential_nise2014_twh[r]);
    wind &= b3.hi[r][kWind] == std::max(in.base_twh[r][kWind], robust.wind_land * in.wind_120m_potential_twh[r]);
  }
  check(solar, "solar_potential_nise2014: solar bound from the NISE 2014 potential");
  check(wind, "wind_120m: wind bound from the NIWE 120 m potential");
  auto throws = [&](void (*f)(PolicySettings&)) {
    try {
      solve_least_cost(in, with(f), req);
    } catch (const std::invalid_argument&) {
      return true;
    }
    return false;
  };
  check(throws([](PolicySettings& p) { p.cost_basis = 2; }) && throws([](PolicySettings& p) { p.wind_height = 100; }) &&
            throws([](PolicySettings& p) { p.solar_potential_set = 2; }),
        "unknown cost basis, wind height or solar set: error");
  return check.fails;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const std::string root = argc > 1 ? argv[1] : ".";
    const AllocationInputs in = load_allocation_inputs(root);
    const int base_fails = base_mix_matches_npp(root, in);
    std::printf("%-14s", "region");
    for (int s = 0; s < kTechs; ++s) std::printf("%11s", tech_key(s));
    std::printf("\n");
    for (int r = 0; r < kRegions; ++r) {
      std::printf("%-14s", region_name(r));
      for (int s = 0; s < kTechs; ++s) std::printf("%11.1f", in.base_twh[r][s]);
      std::printf("   base TWh\n%-14s", "");
      for (int s = 0; s < kTechs; ++s) std::printf("%11.1f", in.potential_twh[r][s]);
      std::printf("   potential TWh\n%-14s", "");
      for (int s = 0; s < kTechs; ++s) std::printf("%11.1f", in.cost[r][s]);
      std::printf("   USD/MWh\n");
    }
    std::printf("EF coal %.3f gas %.3f; corridors %zu; nuclear units:", in.emission_t_per_mwh[kCoal],
                in.emission_t_per_mwh[kGas], in.corridors.size());
    for (double u : in.nuclear_units) std::printf(" %.0f", u);
    std::printf("\nnational FY%d-%02d TWh / NPP record annual increase (fallback only):", in.rate_last_year,
                (in.rate_last_year + 1) % 100);
    for (int s = 0; s < kTechs; ++s)
      if (in.national_last_twh[s] > 0) std::printf(" %s %.0f/+%.1f", tech_key(s), in.national_last_twh[s], in.max_annual_increase[s]);
    std::printf("\n");

    double base_total = 0;
    std::array<double, kRegions> base_r{};
    for (int r = 0; r < kRegions; ++r) {
      for (int s = 0; s < kTechs; ++s) base_r[r] += in.base_twh[r][s];
      base_total += base_r[r];
    }
    std::array<double, kRegions> req{};
    for (int r = 0; r < kRegions; ++r) req[r] = 4500.0 * base_r[r] / base_total;

    int fails = base_fails;
    fails += published_limits_loaded(in);
    fails += build_limit_binds(in, req);
    fails += build_limit_below_base_feasible(in, req);
    fails += build_limit_holds_last_period(std::filesystem::temp_directory_path().string(), in, req);
    fails += fleet_floor_from_npp_history(root, in);
    fails += default_policy_low_case_feasible(root);
    fails += emission_factor_latest_period(root, std::filesystem::temp_directory_path().string(), in);
    fails += cea_lcoe_formula(root, in);
    fails += sensitivity_from_copy(in, req);

    PolicySettings p;
    Timer t;
    const AllocationResult lp = solve_least_cost(in, p, req);
    std::printf("LP: %s in %.2f ms\n", lp.status.c_str(), t.ms());
    if (!lp.ok) return 1;
    for (int r = 0; r < kRegions; ++r) {
      std::printf("%-14s req %7.1f  prod %7.1f  net export %7.1f  unserved %5.1f  marginal %6.1f USD/MWh\n",
                  region_name(r), req[r], req[r] - lp.unserved[r] + lp.net_export[r], lp.net_export[r], lp.unserved[r],
                  lp.marginal_cost[r]);
    }
    std::printf("national:");
    for (int s = 0; s < kTechs; ++s) std::printf(" %s %.1f%%", tech_key(s), 100 * lp.share(s));
    std::printf("\nnon-fossil %.1f%%, CO2 %.0f Mt, cost %.1f bn USD/yr\n", 100 * lp.nonfossil_share(), lp.co2_mt,
                lp.cost_busd);
    if (lp.nonfossil_share() < p.nonfossil_min - 1e-6) ++fails;
    if (lp.share(kGas) > p.gas_max + 1e-6) ++fails;

    const AllocationResult sc = allocate_by_score(in, p, req, lp.national);
    std::printf("score allocation net exports:");
    for (int r = 0; r < kRegions; ++r) std::printf(" %s %.1f", region_name(r), sc.net_export[r]);
    std::printf("\n");
    for (int s = 0; s < kTechs; ++s)
      if (std::abs(sc.national[s] - lp.national[s]) > 1e-3 * std::max(1.0, lp.national[s])) {
        std::printf("score total mismatch for %s: %.2f vs %.2f\n", tech_key(s), sc.national[s], lp.national[s]);
        ++fails;
      }
    std::printf("%s\n", fails ? "FAIL" : "ok");
    return fails ? 1 : 0;
  } catch (const std::exception& e) {
    std::printf("error: %s\n", e.what());
    return 2;
  }
}
