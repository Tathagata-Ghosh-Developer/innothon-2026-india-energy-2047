// Allocation check: loads the real inputs, checks the base-year mix against its
// sources, checks the published build limits, solves the least-cost LP and the
// score allocation for a test requirement (4,500 TWh split by base-year regional
// shares), and verifies balances and policy constraints.
// Usage: test_allocation <repo_root>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <numeric>
#include <string>

#include "allocation/allocate.hpp"
#include "core/csv.hpp"
#include "core/stats.hpp"

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
// still allows base-year generation: cap = max(base, base + accel x limit).
int build_limit_below_base_feasible(const AllocationInputs& in0, const std::array<double, kRegions>& req) {
  Checker check;
  std::printf("build_limit_below_base_feasible\n");
  // Only the caps can bind: no share or firm floors.
  PolicySettings p;
  p.nonfossil_min = 0;
  p.firm_national = 0;
  p.firm_regional = 0;
  for (double limit : {0.0, -500.0}) {
    AllocationInputs in = in0;
    for (auto& v : in.published_build_twh) v.fill(limit);
    const std::vector<BuildCap> caps = build_caps(in, p);
    bool at_base = !caps.empty();
    for (const BuildCap& c : caps) at_base &= std::abs(c.cap_twh - c.base_twh) < 1e-9;
    char what[96];
    std::snprintf(what, sizeof what, "limit %.0f TWh: every cap equals its base", limit);
    check(at_base, what);
    const AllocationResult a = solve_least_cost(in, p, req);
    std::snprintf(what, sizeof what, "limit %.0f TWh: LP feasible (%s)", limit, a.status.c_str());
    check(a.ok, what);
    if (!a.ok) continue;
    for (int s : {kSolar, kWind, kNuclear, kHydro}) {
      std::snprintf(what, sizeof what, "limit %.0f TWh: %s at its base %.2f TWh", limit, tech_key(s),
                    in.national_last_twh[s]);
      check(std::abs(a.national[s] - in.national_last_twh[s]) < 1e-6 * std::max(1.0, in.national_last_twh[s]), what);
    }
  }
  PolicySettings z = p;
  z.build_accel = 0;
  bool at_base = true;
  for (const BuildCap& c : build_caps(in0, z)) at_base &= std::abs(c.cap_twh - c.base_twh) < 1e-9;
  check(at_base, "build_accel 0: every cap equals its base");
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
