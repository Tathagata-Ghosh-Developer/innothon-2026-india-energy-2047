#pragma once
// 2047 regional generation strategy: least-cost linear program (HiGHS) and a
// transparent resource-score allocation. Both meet the same regional
// requirements; their difference shows what cost minimisation adds.
#include <array>
#include <string>
#include <vector>

#include "inputs.hpp"

namespace e47 {

// Policy and planning settings. All shares are shares of 2047 generation.
struct PolicySettings {
  double nonfossil_min = 0.70;     // minimum non-fossil share of national generation
  double gas_max = 0.05;           // maximum gas share
  double nuclear_max = 0.10;       // maximum nuclear share
  double coal_floor = 0.60;        // regional coal >= floor x base-year coal (existing young fleet)
  double coal_cap = 1.10;          // national coal <= cap x base-year coal (no coal beyond pipeline)
  double coal_regional_max = 1.50; // regional coal <= this x its base-year coal (fleet and pithead siting)
  double firm_national = 0.45;     // firm generation >= share x national requirement
  double firm_regional = 0.25;     // each region's own firm generation >= share x its requirement
  double solar_land = 0.25;        // usable fraction of solar generation potential by 2047
  double wind_land = 0.25;         // usable fraction of onshore wind potential
  double offshore_frac = 0.50;     // usable fraction of offshore potential
  double hydro_frac = 0.70;        // usable fraction of identified hydro potential
  double bio_frac = 0.50;          // usable fraction of bioenergy potential
  double transfer_scale = 2.0;     // 2047 corridor capability / 2032 plan
  double transfer_util = 0.60;     // average corridor utilisation over the year
  double carbon_price = 0.0;       // USD per tCO2
  // National 2047 generation of each source <= last year + years x accel x record annual
  // increase (solar, onshore wind and firmed RE share the solar plus wind record).
  double build_accel = 2.0;
  int target_year = 2047;
  double unserved_cost = 1000.0;   // USD/MWh penalty that keeps every run feasible
  double co2_cap_mt = -1.0;        // national CO2 cap, Mt per year (<= 0: no cap)
  std::array<double, kTechs> cost_mult{1, 1, 1, 1, 1, 1, 1, 1, 1};
};

struct AllocationResult {
  bool ok = false;
  std::string status;
  RegionTech gen{};                         // TWh
  std::array<double, kRegions> requirement{};
  std::array<double, kRegions> net_export{};  // production - requirement (before losses)
  std::array<double, kRegions> marginal_cost{};  // dual of the regional balance, USD/MWh
  std::array<double, kRegions> unserved{};       // TWh not met (0 when the system is adequate)
  std::vector<double> corridor_flow;        // positive = a -> b, TWh
  double total_twh = 0;
  double cost_busd = 0;   // billion USD per year
  double co2_mt = 0;      // MtCO2 per year
  double co2_cap_dual = 0;  // marginal abatement cost at the CO2 cap, USD per tonne (0 if no cap or slack)
  std::array<double, kTechs> national{};    // TWh by source

  double share(int s) const { return total_twh > 0 ? national[s] / total_twh : 0.0; }
  double nonfossil_share() const;
};

// Bounds actually used for each region and source (after policy fractions).
struct Bounds {
  RegionTech lo{}, hi{};
};
Bounds make_bounds(const AllocationInputs& in, const PolicySettings& p, const std::array<double, kRegions>& req);

AllocationResult solve_least_cost(const AllocationInputs& in, const PolicySettings& p,
                                  const std::array<double, kRegions>& requirement);

// National totals by source are distributed across regions in proportion to
// resource strength (generation potential, existing fleet, nuclear sites),
// within each region's bounds.
AllocationResult allocate_by_score(const AllocationInputs& in, const PolicySettings& p,
                                   const std::array<double, kRegions>& requirement,
                                   const std::array<double, kTechs>& national_twh);

// Score used by allocate_by_score for region r and source s.
double resource_score(const AllocationInputs& in, const Bounds& b, int r, int s);

// Cost-CO2 trade-off (epsilon-constraint): the least-cost plan is re-solved under
// a national CO2 cap stepped from the unconstrained plan's emissions down to the
// lowest cap that still meets the requirement in full. The LP frontier is convex,
// and each cap's dual is the marginal abatement cost at that point.
struct ParetoPoint {
  double co2_cap_mt = 0;          // cap applied (the first point is uncapped)
  double co2_mt = 0;
  double cost_busd = 0;
  double marginal_usd_per_t = 0;  // dual of the cap, USD per tonne
  double unserved_twh = 0;
  AllocationResult plan;
};
std::vector<ParetoPoint> pareto_cost_co2(const AllocationInputs& in, const PolicySettings& p,
                                         const std::array<double, kRegions>& requirement, int points);

// One-at-a-time sensitivity of the least-cost strategy: each setting is moved to
// a low and a high value with everything else fixed. Solves run in parallel.
struct TornadoRow {
  std::string name;
  std::string low_label, high_label;
  AllocationResult low, high;
};
std::vector<TornadoRow> policy_tornado(const AllocationInputs& in, const PolicySettings& p,
                                       const std::array<double, kRegions>& requirement, double req_low_scale,
                                       double req_high_scale);

}  // namespace e47
