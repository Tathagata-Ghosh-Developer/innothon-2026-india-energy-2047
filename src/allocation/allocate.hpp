#pragma once
// 2047 regional generation strategy: least-cost linear program (HiGHS) and a
// transparent resource-score allocation. Both meet the same regional
// requirements; their difference shows what cost minimisation adds.
#include <array>
#include <optional>
#include <string>
#include <vector>

#include "inputs.hpp"

namespace e47 {

// Cost table the LP uses (PolicySettings::cost_basis).
enum CostBasis : int { kCostIEA, kCostCEA };  // AllocationInputs::cost (default), ::cost_cea
// Solar potential set (PolicySettings::solar_potential_set).
enum SolarPotentialSet : int { kSolarNise2025, kSolarNise2014 };  // ground-mounted 2025 (default), technical 2014

// Policy and planning settings. All shares are shares of 2047 generation.
struct PolicySettings {
  double nonfossil_min = 0.70;     // minimum non-fossil share of national generation
  double gas_max = 0.05;           // maximum gas share
  double nuclear_max = 0.10;       // maximum nuclear share (of the requirement; never below the nuclear floor)
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
  // National build limits (build_caps): 2047 generation of each source group <= max(existing-
  // fleet floor, FY2023-24 generation) + build_accel x the published additions to 2047 in case
  // build_case (build_limits.csv, GW per year x CUF x 8.76). Solar, onshore wind and firmed RE
  // share one cap. The non-fossil minimum is a hard row (unserved energy cannot relax it), so
  // a low case with a small multiplier can make the LP infeasible: the default policy on the
  // low case is feasible from 0.75x, the dashboard's lowest setting
  // (test_allocation, default_policy_low_case_feasible).
  int build_case = kBuildCentral;  // kBuildLow, kBuildCentral or kBuildHigh
  double build_accel = 1.0;        // multiplier on the published additions
  int target_year = 2047;
  double unserved_cost = 1000.0;   // USD/MWh penalty that keeps every run feasible
  double co2_cap_mt = -1.0;        // national CO2 cap, Mt per year (<= 0: no cap)
  std::array<double, kTechs> cost_mult{1, 1, 1, 1, 1, 1, 1, 1, 1};
  // Published-figure sensitivities (outputs/sensitivity_mix.csv); the defaults are the main run.
  int cost_basis = kCostIEA;                 // kCostIEA: IEA WEO 2025; kCostCEA: CEA NEP inputs (inputs.hpp)
  // Expected inflation (fraction) that makes the nominal CERC WACC of the CEA basis real (Fisher
  // relation, cea_cost_table). Unset by default: the RBI CPI target read with the inputs
  // (AllocationInputs::cea_inflation_target, from price_indices.csv), so the target is held in
  // that file only; cea_inflation_used gives the value in force. The RBI tolerance band (2 and
  // 6 percent) gives the sensitivity rows. The IEA basis ignores it.
  std::optional<double> cea_inflation;
  int solar_potential_set = kSolarNise2025;  // kSolarNise2025 or kSolarNise2014
  int wind_height = 150;                     // NIWE potential at 150 m (default) or 120 m
  // Gas kept for peaking and as a bridge for variable renewables (gas_min_twh): national gas
  // generation >= this share (0 to 1) of the existing gas fleet's best NPP year
  // (AllocationInputs::fleet_floor_twh, diesel counted with gas as in the base mix). Each region's
  // gas is bounded by its FY2023-24 share of the national gas cap (make_bounds). With no published
  // gas additions the cap is the fleet floor, so at share 1 (the Robust preset) the minimum equals
  // the cap and every region is held at its FY2023-24 share of the fleet floor; the LP chooses gas
  // sites only below share 1. A gas_max too low to hold it makes the LP infeasible.
  double gas_min_twh_share = 0.0;
  // National nuclear floor (nuclear_floor_twh): one LP row, nuclear summed over the regions >= the
  // floor; the LP sites it within each region's nuclear bound (the siting key, make_bounds).
  // nuclear_min_twh is a floor in TWh a year (0 = none); nuclear_mission picks a case of the
  // Nuclear Energy Mission (kMissionNone = none), whose TWh come from nuclear_mission.csv
  // (AllocationInputs::nuclear_mission), so no preset holds a TWh literal. The floor is the larger
  // of the two. The nuclear_max share never cuts it: where the floor is above nuclear_max x the
  // requirement (a low requirement), the floor is the share limit. The national nuclear build cap
  // does: a floor above the national nuclear limit (build cap or summed regional bounds) is not
  // cut, and solve_least_cost returns ok = false with a status that names both.
  double nuclear_min_twh = 0.0;
  int nuclear_mission = kMissionNone;

  bool operator==(const PolicySettings&) const = default;
};

// National gas minimum of the policy, TWh: gas_min_twh_share x the gas fleet floor, at most the
// national gas build cap. Throws std::invalid_argument if the share is not in [0, 1].
double gas_min_twh(const AllocationInputs& in, const PolicySettings& p);

// National nuclear floor of the policy, TWh: max(nuclear_min_twh, the TWh of the nuclear_mission
// case); 0 when neither is set. Throws std::invalid_argument on a negative or non-finite
// nuclear_min_twh, an unknown case, or a case with no TWh (inputs not loaded).
double nuclear_floor_twh(const AllocationInputs& in, const PolicySettings& p);

// Expected inflation of the CEA basis under the policy: p.cea_inflation if set, otherwise the
// RBI CPI target of the inputs (AllocationInputs::cea_inflation_target).
double cea_inflation_used(const AllocationInputs& in, const PolicySettings& p);

// Cost table of the policy's cost basis, real 2024 USD/MWh by region and source: the IEA table,
// or the CEA table at cea_inflation_used. Throws std::invalid_argument on an unknown basis or an
// inflation of -100 percent or below.
RegionTech cost_table(const AllocationInputs& in, const PolicySettings& p);
// Generation potential (TWh a year) of source s in region r under the policy's solar set and
// wind height; other sources: AllocationInputs::potential_twh. Throws on an unknown set or height.
double potential_twh(const AllocationInputs& in, const PolicySettings& p, int r, int s);

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
  std::vector<double> build_cap_dual;  // USD/MWh for each build_caps() group, in its order (0 if slack)
  std::array<double, kTechs> national{};    // TWh by source

  double share(int s) const { return total_twh > 0 ? national[s] / total_twh : 0.0; }
  double nonfossil_share() const;
  double unserved_total() const;  // TWh, summed over regions
};

// Bounds actually used for each region and source (after policy fractions). Nuclear and hydro:
// lo = FY2023-24 base + committed builds (AllocationInputs::committed_twh). Committed plants are
// part of the published additions, so the committed TWh of a source is first cut to fit its
// national build cap (and, for nuclear, the nuclear share limit), then each region's floor to its
// regional bound hi. Nuclear hi = max(FY2023-24 base, the share limit x the region's share of
// reactor units); the share limit is nuclear_max x the requirement, or the nuclear floor
// (nuclear_floor_twh) where that is higher. Gas: hi = the national gas build cap x the region's
// FY2023-24 share of gas (no published gas additions: gas comes from the existing fleet).
struct Bounds {
  RegionTech lo{}, hi{};
  RegionTech committed{};      // committed-build TWh inside lo
  RegionTech committed_cut{};  // committed-build TWh left out because a cap or bound is lower
};
Bounds make_bounds(const AllocationInputs& in, const PolicySettings& p, const std::array<double, kRegions>& req);

// National upper bound on the generation of source s, TWh: the lowest of
//   the sum of its regional bounds (make_bounds hi; for firmed RE, which has no bound of its
//   own, the sum of the regional solar and wind bounds it shares),
//   the national build cap of its group (build_caps; solar, onshore wind and firmed RE share one),
//   for nuclear, the share limit (nuclear_max x the total requirement, or the nuclear floor where
//   that is higher), and for coal, coal_cap x base-year coal.
// The gas_max share limits gas as a share of total generation, which the LP chooses, so it is
// not a TWh bound and is left out. +infinity if nothing bounds s. Throws on an unknown source.
double national_upper_bound_twh(const AllocationInputs& in, const PolicySettings& p,
                                const std::array<double, kRegions>& req, int s);

// National build limit on one group of sources: one LP row, sum of the group's generation
// over regions <= cap_twh. Each source in `evidence` adds its starting level, max(existing-
// fleet floor, FY2023-24 generation) (AllocationInputs::fleet_floor_twh: the best NPP year
// FY2018-19 to FY2025-26 for coal, gas, nuclear and hydro; renewables start from FY2023-24),
// plus max(0, build_accel x its published additions to target_year for build_case); a source
// without published rows uses the record fallback instead (years x build_accel x its record
// annual increase). The cap is never below the group's start, so never below its FY2023-24
// base, and base-year generation always fits within the caps. Groups: solar + onshore
// wind + firmed RE (solar and wind with storage, on the solar and wind limits), offshore
// wind, nuclear, hydro, bioenergy, gas, coal (the national coal ceiling, coal_cap x base
// coal, applies as well).
struct BuildCap {
  std::string name;
  std::vector<int> techs;     // sources the row sums
  double base_twh = 0;        // national FY2023-24 generation of the evidence sources
  double start_twh = 0;       // sum of max(existing-fleet floor, FY2023-24) over the evidence sources
  double cap_twh = 0;
  bool published = true;      // false: a source of the group used the record fallback
};
std::vector<BuildCap> build_caps(const AllocationInputs& in, const PolicySettings& p);

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

// Sensitivity of the mix to the published figures behind it (outputs/sensitivity_mix.csv).
// Each case is a copy of `base` with one setting changed; `base` itself is never modified.
// Cases, in order: base; cost_basis_cea (CEA NEP cost basis, real WACC at the RBI target of 4
// percent); cost_basis_cea_pi2 and cost_basis_cea_pi6 (the same at 2 and 6 percent, the RBI
// tolerance band); solar_potential_nise2014; wind_120m; build_low and build_central
// (build_case low and central; the Robust preset, the usual base, is already on the high case);
// nuclear_mission_low and nuclear_mission_high (the low and high cases of the Nuclear Energy
// Mission floor, nuclear_mission.csv; the Nuclear mission preset is the central case). Each case
// is solved at `requirement` and at `requirement_p90`; a case whose nuclear floor is above the
// national nuclear limit comes back ok = false with the reason. Solves run in parallel.
struct MixSensitivityRow {
  std::string name, description;
  PolicySettings policy;
  AllocationResult plan;      // at the requirement passed (the median)
  AllocationResult plan_p90;  // the same policy at the P90 requirement
};
std::vector<MixSensitivityRow> mix_sensitivity(const AllocationInputs& in, const PolicySettings& base,
                                               const std::array<double, kRegions>& requirement,
                                               const std::array<double, kRegions>& requirement_p90);
// Everything one mix_sensitivity call reads, to cache its rows (CachedTask in core/cached_task.hpp):
// the dashboard runs the 20 solves only while its mix evidence view is open, and again only when
// one of these changes. `inputs` is compared by address: a pipeline's inputs never change.
struct MixSensitivityKey {
  const AllocationInputs* inputs = nullptr;
  PolicySettings policy;
  std::array<double, kRegions> requirement{}, requirement_p90{};
  bool operator==(const MixSensitivityKey&) const = default;
};

// One plan by the six energy sources of the official challenge page (the landing page's
// regional map; site_summary.json plan.vectors), in its order: coal, solar, wind (onshore +
// offshore), gas, hydro, nuclear. Firmed renewables (solar and wind sold round the clock with
// storage) and bioenergy are not among the six: PlanVectors::other_twh holds them, so the six
// shares plus other_share_pct make 100% of the plan.
//   twh       = the source's generation summed over the regions
//   share_pct = 100 x twh / AllocationResult::total_twh
//   region_share_pct[r] = 100 x region_twh[r] / twh (0 when twh is 0)
//   radius[r] = sqrt(region_twh[r] / the largest region's TWh), in [0, 1] (0 when twh is 0):
//               the radius of a circle whose area is proportional to the region's TWh
//   lead      = the region with the most TWh, or -1 when twh < lead_min_twh
struct SourceVector {
  const char* key = "";
  const char* label = "";
  std::vector<int> techs;
  double twh = 0, share_pct = 0;
  int lead = -1;
  std::array<double, kRegions> region_twh{}, region_share_pct{}, radius{};
};
struct PlanVectors {
  std::array<SourceVector, 6> sources;
  double other_twh = 0, other_share_pct = 0;  // firmed renewables and bioenergy
};
PlanVectors plan_vectors(const AllocationResult& a, double lead_min_twh = 0.5);

}  // namespace e47
