// Allocation check: loads the real inputs, solves the least-cost LP and the
// score allocation for a test requirement (4,500 TWh split by base-year
// regional shares), and verifies balances and policy constraints.
// Usage: test_allocation <repo_root>
#include <cmath>
#include <cstdio>
#include <numeric>

#include "allocation/allocate.hpp"
#include "core/stats.hpp"

using namespace e47;

int main(int argc, char** argv) {
  try {
    const std::string root = argc > 1 ? argv[1] : ".";
    const AllocationInputs in = load_allocation_inputs(root);
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
    int fails = 0;
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
