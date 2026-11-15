// Cost-CO2 trade-off of the 2047 plan: CO2 cap (epsilon-constraint) in the
// least-cost LP, its dual as the marginal abatement cost, and the Pareto sweep.
// Uses the real inputs and a 4,500 TWh requirement split by base-year shares.
// Usage: test_pareto <repo_root>
#include <cmath>
#include <cstdio>
#include <string>

#include "allocation/allocate.hpp"

using namespace e47;

namespace {

int fails = 0;

void check(bool ok, const char* what) {
  std::printf("  %-66s %s\n", what, ok ? "ok" : "FAIL");
  if (!ok) ++fails;
}

double unserved(const AllocationResult& a) {
  double u = 0;
  for (double x : a.unserved) u += x;
  return u;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const std::string root = argc > 1 ? argv[1] : ".";
    const AllocationInputs in = load_allocation_inputs(root);
    std::array<double, kRegions> req{};
    {
      double base_total = 0;
      std::array<double, kRegions> base_r{};
      for (int r = 0; r < kRegions; ++r) {
        for (int s = 0; s < kTechs; ++s) base_r[r] += in.base_twh[r][s];
        base_total += base_r[r];
      }
      for (int r = 0; r < kRegions; ++r) req[r] = 4500.0 * base_r[r] / base_total;
    }
    const PolicySettings p0;
    const AllocationResult free = solve_least_cost(in, p0, req);
    std::printf("unconstrained: %s, CO2 %.1f Mt, cost %.3f bn USD, unserved %.2f TWh\n", free.status.c_str(), free.co2_mt,
                free.cost_busd, unserved(free));
    check(free.ok, "unconstrained plan solves");

    std::printf("cap above the unconstrained CO2\n");
    {
      PolicySettings p = p0;
      p.co2_cap_mt = free.co2_mt + 100.0;
      const AllocationResult a = solve_least_cost(in, p, req);
      check(a.ok, "solves");
      check(std::abs(a.cost_busd - free.cost_busd) < 1e-6 * free.cost_busd, "cost unchanged");
      check(std::abs(a.co2_cap_dual) < 1e-6, "cap dual is zero (cap slack)");
    }

    std::printf("binding cap 200 Mt below the unconstrained CO2\n");
    PolicySettings pb = p0;
    pb.co2_cap_mt = free.co2_mt - 200.0;
    const AllocationResult b = solve_least_cost(in, pb, req);
    {
      check(b.ok, "solves");
      check(b.co2_mt <= pb.co2_cap_mt + 0.5, "CO2 at or below the cap");
      check(b.cost_busd > free.cost_busd, "cost rises");
      check(b.co2_cap_dual > 0.0, "cap dual (marginal abatement cost) is positive");
      check(unserved(b) < 1e-6, "no unserved energy");
      std::printf("    CO2 %.1f Mt, cost %.3f bn USD, marginal abatement cost %.1f USD/t\n", b.co2_mt, b.cost_busd,
                  b.co2_cap_dual);
    }

    std::printf("dual matches the slope of cost against the cap\n");
    {
      PolicySettings p = pb;
      p.co2_cap_mt = pb.co2_cap_mt - 5.0;
      const AllocationResult c = solve_least_cost(in, p, req);
      // 1 USD/t x 1 Mt = 1e6 USD = 1e-3 bn USD.
      const double slope = (c.cost_busd - b.cost_busd) / 5.0 * 1000.0;  // USD per tonne
      std::printf("    finite-difference slope %.1f USD/t vs dual %.1f USD/t\n", slope, b.co2_cap_dual);
      check(c.ok && slope >= b.co2_cap_dual * 0.9 - 1e-6, "slope at least the dual (convex frontier, one-sided)");
      check(c.ok && slope <= c.co2_cap_dual * 1.1 + 1e-6, "slope at most the dual of the tighter cap");
    }

    std::printf("Pareto sweep\n");
    {
      const std::vector<ParetoPoint> pts = pareto_cost_co2(in, p0, req, 8);
      check(pts.size() >= 3, "at least three frontier points");
      bool first = !pts.empty() && std::abs(pts.front().cost_busd - free.cost_busd) < 1e-6 * free.cost_busd;
      check(first, "first point is the unconstrained least-cost plan");
      bool co2_down = true, cost_up = true, mac_up = true, adequate = true;
      for (size_t i = 1; i < pts.size(); ++i) {
        co2_down &= pts[i].co2_mt < pts[i - 1].co2_mt;
        cost_up &= pts[i].cost_busd >= pts[i - 1].cost_busd - 1e-9;
        mac_up &= pts[i].marginal_usd_per_t >= pts[i - 1].marginal_usd_per_t - 1e-6;
      }
      for (const auto& q : pts) adequate &= q.unserved_twh < 1e-6;
      check(co2_down, "CO2 strictly falls along the frontier");
      check(cost_up, "cost never falls along the frontier");
      check(mac_up, "marginal abatement cost never falls (convex LP frontier)");
      check(adequate, "every point meets the requirement in full");
      for (const auto& q : pts)
        std::printf("    cap %8.1f  CO2 %8.1f Mt  cost %7.2f bn USD  MAC %7.1f USD/t\n", q.co2_cap_mt, q.co2_mt,
                    q.cost_busd, q.marginal_usd_per_t);
    }
    std::printf(fails ? "FAILED (%d)\n" : "ok\n", fails);
    return fails ? 1 : 0;
  } catch (const std::exception& e) {
    std::printf("error: %s\n", e.what());
    return 2;
  }
}
