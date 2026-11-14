#include "allocate.hpp"

#include <Highs.h>

#include <algorithm>
#include <cmath>
#include <numeric>

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

}  // namespace

double AllocationResult::nonfossil_share() const {
  double f = 0;
  for (int s = 0; s < kTechs; ++s)
    if (tech_is_fossil(s)) f += national[s];
  return total_twh > 0 ? 1.0 - f / total_twh : 0.0;
}

Bounds make_bounds(const AllocationInputs& in, const PolicySettings& p, const std::array<double, kRegions>& req) {
  Bounds b;
  const double total_req = std::accumulate(req.begin(), req.end(), 0.0);
  double units = 0;
  for (double u : in.nuclear_units) units += u;
  for (int r = 0; r < kRegions; ++r) {
    const auto& base = in.base_twh[r];
    const auto& pot = in.potential_twh[r];
    auto& lo = b.lo[r];
    auto& hi = b.hi[r];
    lo[kCoal] = p.coal_floor * base[kCoal];
    hi[kCoal] = std::max(lo[kCoal], p.coal_cap * base[kCoal]);
    lo[kGas] = 0;
    hi[kGas] = kHighsInf;
    lo[kNuclear] = base[kNuclear];
    hi[kNuclear] = std::max(base[kNuclear], p.nuclear_max * total_req * (units > 0 ? in.nuclear_units[r] / units : 0));
    lo[kHydro] = base[kHydro];
    hi[kHydro] = std::max(base[kHydro], p.hydro_frac * pot[kHydro]);
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

AllocationResult solve_least_cost(const AllocationInputs& in, const PolicySettings& p,
                                  const std::array<double, kRegions>& req) {
  const int nc = static_cast<int>(in.corridors.size());
  const int ubase = kRegions * kTechs + 2 * nc;  // unserved energy variables
  const int nvar = ubase + kRegions;
  const Bounds b = make_bounds(in, p, req);
  const double total_req = std::accumulate(req.begin(), req.end(), 0.0);

  std::vector<double> cost(nvar, 0.0), lo(nvar, 0.0), hi(nvar, kHighsInf);
  for (int r = 0; r < kRegions; ++r)
    for (int s = 0; s < kTechs; ++s) {
      const int v = gvar(r, s);
      cost[v] = in.cost[r][s] * p.cost_mult[s] + p.carbon_price * in.emission_t_per_mwh[s];
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
  // Build-rate limits from India's record annual generation increases.
  const double years = static_cast<double>(p.target_year - in.rate_last_year);
  auto rate_cap = [&](std::initializer_list<int> techs, std::initializer_list<int> evidence) {
    double cap = 0;
    for (int s : evidence) cap += in.national_last_twh[s] + years * p.build_accel * in.max_annual_increase[s];
    const int row = rb.add(-kHighsInf, cap);
    for (int r = 0; r < kRegions; ++r)
      for (int s : techs) rb.set(row, gvar(r, s), 1.0);
  };
  rate_cap({kSolar, kWind, kFirmRE}, {kSolar, kWind});
  rate_cap({kNuclear}, {kNuclear});
  rate_cap({kHydro}, {kHydro});
  rate_cap({kBio}, {kBio});
  rate_cap({kGas}, {kGas});

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
      res.cost_busd += g * in.cost[r][s] * p.cost_mult[s] * 1e-3;  // TWh x USD/MWh = million USD
    }
    res.unserved[r] = sol.col_value[ubase + r];
    res.net_export[r] = prod - (req[r] - res.unserved[r]);
    res.total_twh += prod;
    res.marginal_cost[r] = sol.row_dual[balance[r]];
  }
  res.corridor_flow.resize(nc);
  for (int c = 0; c < nc; ++c) res.corridor_flow[c] = sol.col_value[fvar(c, 0)] - sol.col_value[fvar(c, 1)];
  return res;
}

double resource_score(const AllocationInputs& in, const Bounds& b, int r, int s) {
  switch (s) {
    case kCoal:
    case kGas:
      return in.base_twh[r][s];  // existing fleet and fuel logistics
    case kNuclear:
      return in.nuclear_units[r];
    case kFirmRE:
      return b.hi[r][kSolar] + b.hi[r][kWind];
    default:
      return b.hi[r][s];  // usable generation potential
  }
}

AllocationResult allocate_by_score(const AllocationInputs& in, const PolicySettings& p,
                                   const std::array<double, kRegions>& req,
                                   const std::array<double, kTechs>& national) {
  const Bounds b = make_bounds(in, p, req);
  AllocationResult res;
  res.requirement = req;
  for (int s = 0; s < kTechs; ++s) {
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
      res.cost_busd += res.gen[r][s] * in.cost[r][s] * p.cost_mult[s] * 1e-3;
    }
    res.net_export[r] = prod - req[r];
    res.total_twh += prod;
  }
  res.ok = true;
  res.status = "score allocation";
  return res;
}

}  // namespace e47
