// Allocation check: loads the real inputs, checks the base-year mix against its
// sources, solves the least-cost LP and the score allocation for a test
// requirement (4,500 TWh split by base-year regional shares), and verifies
// balances and policy constraints.
// Usage: test_allocation <repo_root>
#include <cmath>
#include <cstdio>
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
    std::printf("\nnational %d TWh / record annual increase:", in.rate_last_year);
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

    PolicySettings p;
    Timer t;
    const AllocationResult lp = solve_least_cost(in, p, req);
    std::printf("LP: %s in %.2f ms\n", lp.status.c_str(), t.ms());
    if (!lp.ok) return 1;
    int fails = base_fails;
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
