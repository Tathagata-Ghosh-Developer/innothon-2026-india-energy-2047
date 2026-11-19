#include "allocate.hpp"

#include <Highs.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numeric>
#include <stdexcept>
#include <string>

namespace e47 {

namespace {

constexpr double kHoursPerYearTWhPerMW = 8.76e-3;

// Variable layout: g[r][s] then (corridor, direction) flows.
int gvar(int r, int s) { return r * kTechs + s; }
int fvar(int c, int dir) { return kRegions * kTechs + 2 * c + dir; }

struct RowBuilder {
  std::vector<double> lo, hi;
  std::vector<std::vector<std::pair<int, double>>> rows;
  int add(double l, double h) {
    lo.push_back(l);
    hi.push_back(h);
    rows.emplace_back();
    return static_cast<int>(rows.size()) - 1;
  }
  void set(int row, int col, double v) { rows[row].push_back({col, v}); }
};

// National build cap (TWh) of the group whose row sums source s; no limit if none does.
double cap_of(const std::vector<BuildCap>& caps, int s) {
  for (const BuildCap& c : caps)
    for (int t : c.techs)
      if (t == s) return c.cap_twh;
  return kHighsInf;
}

}  // namespace

double AllocationResult::nonfossil_share() const {
  double f = 0;
  for (int s = 0; s < kTechs; ++s)
    if (tech_is_fossil(s)) f += national[s];
  return total_twh > 0 ? 1.0 - f / total_twh : 0.0;
}

double AllocationResult::unserved_total() const { return std::accumulate(unserved.begin(), unserved.end(), 0.0); }

const RegionTech& cost_table(const AllocationInputs& in, const PolicySettings& p) {
  if (p.cost_basis == kCostIEA) return in.cost;
  if (p.cost_basis == kCostCEA) return in.cost_cea;
  throw std::invalid_argument("cost_basis must be 0 (IEA) or 1 (CEA)");
}

double potential_twh(const AllocationInputs& in, const PolicySettings& p, int r, int s) {
  if (s == kSolar) {
    if (p.solar_potential_set == kSolarNise2025) return in.potential_twh[r][kSolar];
    if (p.solar_potential_set == kSolarNise2014) return in.solar_potential_nise2014_twh[r];
    throw std::invalid_argument("solar_potential_set must be 0 (NISE 2025) or 1 (NISE 2014)");
  }
  if (s == kWind) {
    if (p.wind_height == 150) return in.potential_twh[r][kWind];
    if (p.wind_height == 120) return in.wind_120m_potential_twh[r];
    throw std::invalid_argument("wind_height must be 150 or 120");
  }
  return in.potential_twh[r][s];
}

double gas_min_twh(const AllocationInputs& in, const PolicySettings& p) {
  if (!(p.gas_min_twh_share >= 0.0 && p.gas_min_twh_share <= 1.0))
    throw std::invalid_argument("gas_min_twh_share must be between 0 and 1");
  if (p.gas_min_twh_share == 0.0) return 0.0;
  return std::min(p.gas_min_twh_share * in.fleet_floor_twh[kGas], cap_of(build_caps(in, p), kGas));
}

Bounds make_bounds(const AllocationInputs& in, const PolicySettings& p, const std::array<double, kRegions>& req) {
  Bounds b;
  const double total_req = std::accumulate(req.begin(), req.end(), 0.0);
  double units = 0;
  for (double u : in.nuclear_units) units += u;
  // Committed builds raise the nuclear and hydro floors. They are part of the published
  // additions, so together they fit within the national build cap of the source (nuclear also
  // within its national share limit): fit[s] is the share of the committed TWh that does.
  const std::vector<BuildCap> caps = build_caps(in, p);
  std::array<double, kTechs> fit{};
  double base_gas = 0;
  for (int r = 0; r < kRegions; ++r) base_gas += in.base_twh[r][kGas];
  for (int s : {kNuclear, kHydro}) {
    double base = 0, committed = 0;
    for (int r = 0; r < kRegions; ++r) {
      base += in.base_twh[r][s];
      committed += in.committed_twh[r][s];
    }
    double room = cap_of(caps, s);
    if (s == kNuclear) room = std::min(room, p.nuclear_max * total_req);
    fit[s] = committed > 0 ? std::clamp((room - base) / committed, 0.0, 1.0) : 0.0;
  }
  const double gas_cap = cap_of(caps, kGas);
  for (int r = 0; r < kRegions; ++r) {
    const auto& base = in.base_twh[r];
    std::array<double, kTechs> pot{};
    for (int s = 0; s < kTechs; ++s) pot[s] = potential_twh(in, p, r, s);
    auto& lo = b.lo[r];
    auto& hi = b.hi[r];
    lo[kCoal] = p.coal_floor * base[kCoal];
    hi[kCoal] = std::max(lo[kCoal], p.coal_regional_max * base[kCoal]);
    // Gas: no published additions, so each region keeps its share of the existing fleet's cap.
    lo[kGas] = 0;
    hi[kGas] = base_gas > 0 && gas_cap < kHighsInf ? gas_cap * base[kGas] / base_gas : kHighsInf;
    hi[kNuclear] = std::max(base[kNuclear], p.nuclear_max * total_req * (units > 0 ? in.nuclear_units[r] / units : 0));
    hi[kHydro] = std::max(base[kHydro], p.hydro_frac * pot[kHydro]);
    for (int s : {kNuclear, kHydro}) {
      const double add = std::min(fit[s] * in.committed_twh[r][s], hi[s] - base[s]);
      lo[s] = base[s] + add;
      b.committed[r][s] = add;
      b.committed_cut[r][s] = in.committed_twh[r][s] - add;
    }
    lo[kSolar] = base[kSolar];
    hi[kSolar] = std::max(base[kSolar], p.solar_land * pot[kSolar]);
    lo[kWind] = base[kWind];
    hi[kWind] = std::max(base[kWind], p.wind_land * pot[kWind]);
    lo[kOffshore] = 0;
    hi[kOffshore] = p.offshore_frac * pot[kOffshore];
    lo[kBio] = 0;
    hi[kBio] = std::max(base[kBio], p.bio_frac * pot[kBio]);
    lo[kFirmRE] = 0;
    hi[kFirmRE] = kHighsInf;  // limited by the shared solar and wind resource row
  }
  return b;
}

std::vector<BuildCap> build_caps(const AllocationInputs& in, const PolicySettings& p) {
  if (p.build_case < 0 || p.build_case >= kBuildCases)
    throw std::invalid_argument("build_case must be 0 (low), 1 (central) or 2 (high)");
  struct Group {
    const char* name;
    std::vector<int> techs, evidence;
  };
  static const Group groups[] = {
      {"Solar, onshore wind and firmed RE", {kSolar, kWind, kFirmRE}, {kSolar, kWind}},
      {"Offshore wind", {kOffshore}, {kOffshore}},
      {"Nuclear", {kNuclear}, {kNuclear}},
      {"Hydro", {kHydro}, {kHydro}},
      {"Bioenergy", {kBio}, {kBio}},
      {"Gas", {kGas}, {kGas}},
      {"Coal", {kCoal}, {kCoal}},
  };
  const double years = static_cast<double>(p.target_year - in.rate_last_year);
  std::vector<BuildCap> caps;
  for (const Group& g : groups) {
    BuildCap c;
    c.name = g.name;
    c.techs = g.techs;
    for (int s : g.evidence) {
      double add;
      if (in.has_published_build[s]) {
        add = p.target_year == in.build_target_year
                  ? in.published_build_twh[p.build_case][s]
                  : published_build_twh(in.build_limits, s, p.build_case, in.rate_last_year, p.target_year);
      } else {
        add = years * in.max_annual_increase[s];
        c.published = false;
      }
      // Existing fleets start from their best NPP year if it beats FY2023-24 (fleet_floor_twh
      // is 0 for renewables); the cap is never below this starting level.
      const double start = std::max(in.national_last_twh[s], in.fleet_floor_twh[s]);
      c.base_twh += in.national_last_twh[s];
      c.start_twh += start;
      c.cap_twh += start + std::max(0.0, p.build_accel * add);
    }
    caps.push_back(std::move(c));
  }
  return caps;
}

AllocationResult solve_least_cost(const AllocationInputs& in, const PolicySettings& p,
                                  const std::array<double, kRegions>& req) {
  const int nc = static_cast<int>(in.corridors.size());
  const int ubase = kRegions * kTechs + 2 * nc;  // unserved energy variables
  const int nvar = ubase + kRegions;
  const double gas_min = gas_min_twh(in, p);  // throws on a share outside [0, 1]
  const Bounds b = make_bounds(in, p, req);
  const double total_req = std::accumulate(req.begin(), req.end(), 0.0);
  const RegionTech& unit_cost = cost_table(in, p);

  std::vector<double> cost(nvar, 0.0), lo(nvar, 0.0), hi(nvar, kHighsInf);
  for (int r = 0; r < kRegions; ++r)
    for (int s = 0; s < kTechs; ++s) {
      const int v = gvar(r, s);
      cost[v] = unit_cost[r][s] * p.cost_mult[s] + p.carbon_price * in.emission_t_per_mwh[s];
      lo[v] = b.lo[r][s];
      hi[v] = b.hi[r][s];
    }
  for (int c = 0; c < nc; ++c) {
    const double cap = in.corridors[c].mw_2032 * p.transfer_scale * p.transfer_util * kHoursPerYearTWhPerMW;
    for (int d = 0; d < 2; ++d) {
      hi[fvar(c, d)] = cap;
      cost[fvar(c, d)] = 0.5;  // small wheeling cost so flows are not degenerate
    }
  }
  for (int r = 0; r < kRegions; ++r) cost[ubase + r] = p.unserved_cost;

  RowBuilder rb;
  // Regional balance: production + imports after losses - exports = requirement.
  std::array<int, kRegions> balance{};
  for (int r = 0; r < kRegions; ++r) {
    balance[r] = rb.add(req[r], req[r]);
    for (int s = 0; s < kTechs; ++s) rb.set(balance[r], gvar(r, s), 1.0);
    rb.set(balance[r], ubase + r, 1.0);
  }
  for (int c = 0; c < nc; ++c) {
    const auto& k = in.corridors[c];
    // direction 0: a -> b, direction 1: b -> a
    rb.set(balance[k.a], fvar(c, 0), -1.0);
    rb.set(balance[k.b], fvar(c, 0), 1.0 - k.loss);
    rb.set(balance[k.b], fvar(c, 1), -1.0);
    rb.set(balance[k.a], fvar(c, 1), 1.0 - k.loss);
  }
  for (int r = 0; r < kRegions; ++r) {
    // Solar, onshore wind and firmed RE share one renewable resource.
    const int row = rb.add(-kHighsInf, b.hi[r][kSolar] + b.hi[r][kWind]);
    rb.set(row, gvar(r, kSolar), 1.0);
    rb.set(row, gvar(r, kWind), 1.0);
    rb.set(row, gvar(r, kFirmRE), 1.0);
    // Regional firm generation.
    const int firm = rb.add(p.firm_regional * req[r], kHighsInf);
    for (int s = 0; s < kTechs; ++s)
      if (tech_is_firm(s)) rb.set(firm, gvar(r, s), 1.0);
  }
  // National rows. Total generation G = sum of all g.
  const int firm_nat = rb.add(p.firm_national * total_req, kHighsInf);
  const int nonfossil = rb.add(0.0, kHighsInf);     // sum(nonfossil) - nf * G >= 0
  const int gas = rb.add(-kHighsInf, 0.0);          // sum(gas) - gmax * G <= 0
  const int nuclear = rb.add(-kHighsInf, p.nuclear_max * total_req);
  for (int r = 0; r < kRegions; ++r)
    for (int s = 0; s < kTechs; ++s) {
      const int v = gvar(r, s);
      if (tech_is_firm(s)) rb.set(firm_nat, v, 1.0);
      rb.set(nonfossil, v, (tech_is_fossil(s) ? 0.0 : 1.0) - p.nonfossil_min);
      rb.set(gas, v, (s == kGas ? 1.0 : 0.0) - p.gas_max);
      if (s == kNuclear) rb.set(nuclear, v, 1.0);
    }
  // Gas kept for peaking and as a bridge (gas_min_twh_share): national gas at least a share of the
  // existing fleet's best NPP year, sited by the LP within the regional gas bounds.
  if (gas_min > 0) {
    const int row = rb.add(gas_min, kHighsInf);
    for (int r = 0; r < kRegions; ++r) rb.set(row, gvar(r, kGas), 1.0);
  }
  // National build limits: max(existing-fleet floor, FY2023-24 generation) + build_accel x the
  // published additions to 2047 (build_caps). Firmed RE is solar and wind with storage, so it
  // shares their cap.
  const std::vector<BuildCap> caps = build_caps(in, p);
  std::vector<int> cap_rows;
  for (const BuildCap& c : caps) {
    const int row = rb.add(-kHighsInf, c.cap_twh);
    for (int r = 0; r < kRegions; ++r)
      for (int s : c.techs) rb.set(row, gvar(r, s), 1.0);
    cap_rows.push_back(row);
  }
  {
    // National coal ceiling: existing plus under-construction fleet. The published coal
    // build limit above applies as well.
    double base_coal = 0;
    for (int r = 0; r < kRegions; ++r) base_coal += in.base_twh[r][kCoal];
    const int row = rb.add(-kHighsInf, p.coal_cap * base_coal);
    for (int r = 0; r < kRegions; ++r) rb.set(row, gvar(r, kCoal), 1.0);
  }
  // Optional national CO2 cap: TWh x t/MWh = Mt, so the row is in Mt and its dual
  // is in million USD per Mt = USD per tonne.
  int co2_row = -1;
  if (p.co2_cap_mt > 0) {
    co2_row = rb.add(-kHighsInf, p.co2_cap_mt);
    for (int r = 0; r < kRegions; ++r)
      for (int s = 0; s < kTechs; ++s)
        if (in.emission_t_per_mwh[s] > 0) rb.set(co2_row, gvar(r, s), in.emission_t_per_mwh[s]);
  }

  // Column-wise matrix for HiGHS.
  HighsLp lp;
  lp.num_col_ = nvar;
  lp.num_row_ = static_cast<HighsInt>(rb.rows.size());
  lp.col_cost_ = cost;
  lp.col_lower_ = lo;
  lp.col_upper_ = hi;
  lp.row_lower_ = rb.lo;
  lp.row_upper_ = rb.hi;
  lp.sense_ = ObjSense::kMinimize;
  std::vector<std::vector<std::pair<int, double>>> cols(nvar);
  for (int row = 0; row < lp.num_row_; ++row)
    for (auto [col, v] : rb.rows[row])
      if (v != 0.0) cols[col].push_back({row, v});
  lp.a_matrix_.format_ = MatrixFormat::kColwise;
  lp.a_matrix_.num_col_ = lp.num_col_;
  lp.a_matrix_.num_row_ = lp.num_row_;
  lp.a_matrix_.start_.assign(1, 0);  // HighsSparseMatrix starts with {0}; do not push a second 0
  lp.a_matrix_.index_.clear();
  lp.a_matrix_.value_.clear();
  for (int c = 0; c < nvar; ++c) {
    for (auto [row, v] : cols[c]) {
      lp.a_matrix_.index_.push_back(row);
      lp.a_matrix_.value_.push_back(v);
    }
    lp.a_matrix_.start_.push_back(static_cast<HighsInt>(lp.a_matrix_.index_.size()));
  }

  Highs h;
  h.setOptionValue("output_flag", false);
  h.setOptionValue("threads", 1);
  h.passModel(lp);
  h.run();

  AllocationResult res;
  res.requirement = req;
  const HighsModelStatus st = h.getModelStatus();
  res.status = h.modelStatusToString(st);
  res.ok = st == HighsModelStatus::kOptimal;
  if (!res.ok) return res;

  const HighsSolution& sol = h.getSolution();
  // Independent feasibility check of the returned point against our own rows.
  double worst = 0;
  int worst_row = -1;
  for (int row = 0; row < lp.num_row_; ++row) {
    double act = 0;
    for (auto [col, v] : rb.rows[row]) act += v * sol.col_value[col];
    const double viol = std::max(rb.lo[row] - act, act - rb.hi[row]);
    if (viol > worst) {
      worst = viol;
      worst_row = row;
    }
  }
  if (worst > 1e-4 * std::max(1.0, total_req)) {
    res.ok = false;
    res.status = "solution violates row " + std::to_string(worst_row) + " by " + std::to_string(worst);
    return res;
  }
  for (int r = 0; r < kRegions; ++r) {
    double prod = 0;
    for (int s = 0; s < kTechs; ++s) {
      const double g = sol.col_value[gvar(r, s)];
      res.gen[r][s] = g;
      res.national[s] += g;
      prod += g;
      res.co2_mt += g * in.emission_t_per_mwh[s];
      res.cost_busd += g * unit_cost[r][s] * p.cost_mult[s] * 1e-3;  // TWh x USD/MWh = million USD
    }
    res.unserved[r] = sol.col_value[ubase + r];
    res.net_export[r] = prod - (req[r] - res.unserved[r]);
    res.total_twh += prod;
    res.marginal_cost[r] = sol.row_dual[balance[r]];
  }
  // Minimisation with a <= row: the dual is <= 0; the abatement cost is its negative.
  if (co2_row >= 0) res.co2_cap_dual = std::max(0.0, -sol.row_dual[co2_row]);
  for (int row : cap_rows) res.build_cap_dual.push_back(std::max(0.0, -sol.row_dual[row]));
  res.corridor_flow.resize(nc);
  double circulating = 0;
  for (int c = 0; c < nc; ++c) {
    const double f0 = sol.col_value[fvar(c, 0)], f1 = sol.col_value[fvar(c, 1)];
    res.corridor_flow[c] = f0 - f1;
    circulating = std::max(circulating, std::min(f0, f1));
  }
  // Flow in both directions of a corridor only burns energy as losses; the share
  // rows are written on gross generation, so a solution that needs it is really
  // infeasible.
  if (circulating > 1e-3) {
    res.ok = false;
    res.status = "infeasible (needs circulating flows of " + std::to_string(circulating) + " TWh)";
  }
  return res;
}

std::vector<ParetoPoint> pareto_cost_co2(const AllocationInputs& in, const PolicySettings& base,
                                         const std::array<double, kRegions>& req, int points) {
  auto unserved = [](const AllocationResult& a) {
    double u = 0;
    for (double x : a.unserved) u += x;
    return u;
  };
  auto adequate = [&](const AllocationResult& a) { return a.ok && unserved(a) < 1e-6; };

  std::vector<ParetoPoint> out;
  // The trade-off is between resource cost and CO2, so no carbon price is charged
  // inside the sweep (otherwise the cap's dual would be net of that price).
  PolicySettings p = base;
  p.carbon_price = 0.0;
  p.co2_cap_mt = -1.0;
  AllocationResult free = solve_least_cost(in, p, req);
  if (!adequate(free) || points < 1) return out;

  // Lowest cap that still meets the requirement in full (below it the LP would
  // rather leave demand unserved): bisection on the cap.
  double lo = 0.0, hi = free.co2_mt;
  for (int it = 0; it < 40 && hi - lo > 1e-4 * free.co2_mt; ++it) {
    p.co2_cap_mt = 0.5 * (lo + hi);
    (adequate(solve_least_cost(in, p, req)) ? hi : lo) = p.co2_cap_mt;
  }
  const double cap_min = hi;

  out.push_back({free.co2_mt, free.co2_mt, free.cost_busd, 0.0, 0.0, free});
  std::vector<ParetoPoint> swept(static_cast<size_t>(points));
#pragma omp parallel for schedule(dynamic)
  for (int k = 0; k < points; ++k) {
    PolicySettings q = p;
    q.co2_cap_mt = free.co2_mt - (free.co2_mt - cap_min) * (k + 1) / points;
    AllocationResult a = solve_least_cost(in, q, req);
    const double u = unserved(a);
    swept[static_cast<size_t>(k)] = {q.co2_cap_mt, a.co2_mt, a.cost_busd, a.co2_cap_dual, u, std::move(a)};
  }
  for (auto& q : swept)
    if (adequate(q.plan)) out.push_back(std::move(q));

  // Order by falling CO2 and keep only non-dominated, distinct points.
  std::sort(out.begin() + 1, out.end(), [](const ParetoPoint& a, const ParetoPoint& b) { return a.co2_mt > b.co2_mt; });
  std::vector<ParetoPoint> front;
  for (auto& q : out) {
    if (!front.empty() && q.co2_mt >= front.back().co2_mt - 1e-6) continue;  // no CO2 gain
    while (!front.empty() && q.cost_busd <= front.back().cost_busd - 1e-9) front.pop_back();  // q dominates
    front.push_back(std::move(q));
  }
  return front;
}

double resource_score(const AllocationInputs& in, const Bounds& b, int r, int s) {
  switch (s) {
    case kCoal:
    case kGas:
      return in.base_twh[r][s];  // existing fleet and fuel logistics
    case kNuclear:
      return in.nuclear_units[r];
    case kFirmRE:
      return b.hi[r][kFirmRE] < kHighsInf ? b.hi[r][kFirmRE] : b.hi[r][kSolar] + b.hi[r][kWind];
    default:
      return b.hi[r][s];  // usable generation potential
  }
}

std::vector<TornadoRow> policy_tornado(const AllocationInputs& in, const PolicySettings& base,
                                       const std::array<double, kRegions>& req, double req_lo, double req_hi) {
  struct Spec {
    const char* name;
    double lo, hi;
    void (*apply)(PolicySettings&, double);
    const char* fmt;
  };
  static const Spec specs[] = {
      {"Non-fossil minimum share", 0.60, 0.85, [](PolicySettings& p, double v) { p.nonfossil_min = v; }, "%.0f%%"},
      {"Coal floor (x FY2023-24 coal)", 0.40, 0.80, [](PolicySettings& p, double v) { p.coal_floor = v; }, "%.2f"},
      {"National firm share", 0.35, 0.55, [](PolicySettings& p, double v) { p.firm_national = v; }, "%.0f%%"},
      {"Regional firm share", 0.15, 0.35, [](PolicySettings& p, double v) { p.firm_regional = v; }, "%.0f%%"},
      {"Usable solar land", 0.15, 0.35, [](PolicySettings& p, double v) { p.solar_land = v; }, "%.0f%%"},
      {"Usable wind land", 0.15, 0.35, [](PolicySettings& p, double v) { p.wind_land = v; }, "%.0f%%"},
      {"Corridor capacity (x 2032 plan)", 1.0, 3.0, [](PolicySettings& p, double v) { p.transfer_scale = v; }, "%.1fx"},
      {"Build-rate acceleration (x published limit)", 0.75, 1.5,
       [](PolicySettings& p, double v) { p.build_accel = v; }, "%.2fx"},
      {"Gas kept for peaking (share of fleet floor)", 0.0, 1.0,
       [](PolicySettings& p, double v) { p.gas_min_twh_share = v; }, "%.0f%%"},
      {"Carbon price (USD/t)", 0.0, 60.0, [](PolicySettings& p, double v) { p.carbon_price = v; }, "%.0f"},
      {"Solar cost", 0.7, 1.3, [](PolicySettings& p, double v) { p.cost_mult[kSolar] = v; }, "%.1fx"},
      {"Nuclear cost", 0.7, 1.3, [](PolicySettings& p, double v) { p.cost_mult[kNuclear] = v; }, "%.1fx"},
      {"Coal cost", 0.7, 1.3, [](PolicySettings& p, double v) { p.cost_mult[kCoal] = v; }, "%.1fx"},
      {"Firmed RE cost (storage)", 0.7, 1.5, [](PolicySettings& p, double v) { p.cost_mult[kFirmRE] = v; }, "%.1fx"},
  };
  constexpr int ns = static_cast<int>(sizeof(specs) / sizeof(specs[0]));
  const int nrow = ns + 1;  // + requirement uncertainty
  (void)gas_min_twh(in, base);  // an invalid setting throws here, not inside the parallel region
  std::vector<TornadoRow> rows(nrow);
  auto label = [](const char* fmt, double v) {
    char b[32];
    const bool pct = std::string(fmt).find('%') != std::string::npos && std::string(fmt).find("%%") != std::string::npos;
    std::snprintf(b, sizeof b, fmt, pct ? 100 * v : v);
    return std::string(b);
  };
#pragma omp parallel for schedule(dynamic)
  for (int k = 0; k < 2 * nrow; ++k) {
    const int i = k / 2;
    const bool high = k % 2 == 1;
    PolicySettings p = base;
    std::array<double, kRegions> r = req;
    if (i < ns) {
      specs[i].apply(p, high ? specs[i].hi : specs[i].lo);
    } else {
      for (double& x : r) x *= high ? req_hi : req_lo;
    }
    AllocationResult a = solve_least_cost(in, p, r);
    if (high)
      rows[i].high = std::move(a);
    else
      rows[i].low = std::move(a);
  }
  for (int i = 0; i < ns; ++i) {
    rows[i].name = specs[i].name;
    rows[i].low_label = label(specs[i].fmt, specs[i].lo);
    rows[i].high_label = label(specs[i].fmt, specs[i].hi);
  }
  rows[ns].name = "2047 requirement (P10 / P90)";
  rows[ns].low_label = "P10";
  rows[ns].high_label = "P90";
  return rows;
}

std::vector<MixSensitivityRow> mix_sensitivity(const AllocationInputs& in, const PolicySettings& base,
                                               const std::array<double, kRegions>& req,
                                               const std::array<double, kRegions>& req_p90) {
  struct Spec {
    const char* name;
    const char* description;
    void (*apply)(PolicySettings&);
  };
  static const Spec specs[] = {
      {"base", "the policy as given", [](PolicySettings&) {}},
      {"cost_basis_cea", "CEA NEP 2022-32 cost basis (CERC WACC, CEA and CERC fuel, RBI rate); nuclear and gas at IEA",
       [](PolicySettings& p) { p.cost_basis = kCostCEA; }},
      {"solar_potential_nise2014", "solar potential: NISE 2014 technical potential instead of NISE 2025 ground-mounted",
       [](PolicySettings& p) { p.solar_potential_set = kSolarNise2014; }},
      {"wind_120m", "onshore wind potential: NIWE 120 m instead of 150 m", [](PolicySettings& p) { p.wind_height = 120; }},
      {"build_low", "published build limits: low case", [](PolicySettings& p) { p.build_case = kBuildLow; }},
      {"build_central", "published build limits: central case", [](PolicySettings& p) { p.build_case = kBuildCentral; }},
  };
  constexpr int ns = static_cast<int>(sizeof(specs) / sizeof(specs[0]));
  std::vector<MixSensitivityRow> rows(ns);
  for (int i = 0; i < ns; ++i) {
    rows[i].name = specs[i].name;
    rows[i].description = specs[i].description;
    rows[i].policy = base;  // a copy; base is never written
    specs[i].apply(rows[i].policy);
    // An invalid setting throws here, not inside the parallel region.
    (void)cost_table(in, rows[i].policy);
    (void)potential_twh(in, rows[i].policy, 0, kSolar);
    (void)potential_twh(in, rows[i].policy, 0, kWind);
    (void)build_caps(in, rows[i].policy);
    (void)gas_min_twh(in, rows[i].policy);
  }
#pragma omp parallel for schedule(dynamic)
  for (int k = 0; k < 2 * ns; ++k) {
    MixSensitivityRow& row = rows[k / 2];
    if (k % 2 == 0)
      row.plan = solve_least_cost(in, row.policy, req);
    else
      row.plan_p90 = solve_least_cost(in, row.policy, req_p90);
  }
  return rows;
}

AllocationResult allocate_by_score(const AllocationInputs& in, const PolicySettings& p,
                                   const std::array<double, kRegions>& req,
                                   const std::array<double, kTechs>& national) {
  Bounds b = make_bounds(in, p, req);
  const RegionTech& unit_cost = cost_table(in, p);
  AllocationResult res;
  res.requirement = req;
  for (int s = 0; s < kTechs; ++s) {
    // Firmed RE uses whatever solar plus wind resource the region has left
    // (solar and wind are allocated first in source order).
    if (s == kFirmRE)
      for (int r = 0; r < kRegions; ++r)
        b.hi[r][kFirmRE] = std::max(0.0, b.hi[r][kSolar] + b.hi[r][kWind] - res.gen[r][kSolar] - res.gen[r][kWind]);
    // Water-filling: start at the lower bounds, then share the remainder by score
    // among regions still below their upper bound.
    std::array<double, kRegions> g{};
    double assigned = 0;
    for (int r = 0; r < kRegions; ++r) {
      g[r] = b.lo[r][s];
      assigned += g[r];
    }
    double remaining = std::max(0.0, national[s] - assigned);
    for (int iter = 0; iter < kRegions && remaining > 1e-9; ++iter) {
      double wsum = 0;
      for (int r = 0; r < kRegions; ++r)
        if (g[r] < b.hi[r][s] - 1e-9) wsum += resource_score(in, b, r, s);
      if (wsum <= 0) break;
      double spent = 0;
      for (int r = 0; r < kRegions; ++r) {
        if (g[r] >= b.hi[r][s] - 1e-9) continue;
        const double add = std::min(remaining * resource_score(in, b, r, s) / wsum, b.hi[r][s] - g[r]);
        g[r] += add;
        spent += add;
      }
      remaining -= spent;
    }
    for (int r = 0; r < kRegions; ++r) {
      res.gen[r][s] = g[r];
      res.national[s] += g[r];
    }
  }
  for (int r = 0; r < kRegions; ++r) {
    double prod = 0;
    for (int s = 0; s < kTechs; ++s) {
      prod += res.gen[r][s];
      res.co2_mt += res.gen[r][s] * in.emission_t_per_mwh[s];
      res.cost_busd += res.gen[r][s] * unit_cost[r][s] * p.cost_mult[s] * 1e-3;
    }
    res.net_export[r] = prod - req[r];
    res.total_twh += prod;
  }
  res.ok = true;
  res.status = "score allocation";
  return res;
}

PlanVectors plan_vectors(const AllocationResult& a, double lead_min_twh) {
  static const struct {
    const char* key;
    const char* label;
    std::vector<int> techs;
  } six[6] = {{"coal", "Coal", {kCoal}},       {"solar", "Solar", {kSolar}}, {"wind", "Wind", {kWind, kOffshore}},
              {"gas", "Natural gas", {kGas}}, {"hydro", "Hydro", {kHydro}}, {"nuclear", "Nuclear", {kNuclear}}};
  PlanVectors pv;
  const double total = a.total_twh;
  for (int k = 0; k < 6; ++k) {
    SourceVector& v = pv.sources[k];
    v.key = six[k].key;
    v.label = six[k].label;
    v.techs = six[k].techs;
    int largest = 0;
    for (int r = 0; r < kRegions; ++r) {
      for (int t : v.techs) v.region_twh[r] += a.gen[r][t];
      v.twh += v.region_twh[r];
      if (v.region_twh[r] > v.region_twh[largest]) largest = r;
    }
    v.share_pct = total > 0 ? 100 * v.twh / total : 0.0;
    const double top = v.region_twh[largest];
    for (int r = 0; r < kRegions; ++r) {
      v.region_share_pct[r] = v.twh > 0 ? 100 * v.region_twh[r] / v.twh : 0.0;
      v.radius[r] = top > 0 ? std::sqrt(std::max(0.0, v.region_twh[r]) / top) : 0.0;
    }
    v.lead = v.twh >= lead_min_twh && top > 0 ? largest : -1;
  }
  for (int s : {kFirmRE, kBio}) pv.other_twh += a.national[s];
  pv.other_share_pct = total > 0 ? 100 * pv.other_twh / total : 0.0;
  return pv;
}

}  // namespace e47
