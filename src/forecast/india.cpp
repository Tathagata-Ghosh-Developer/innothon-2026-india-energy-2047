#include "india.hpp"

#include <Eigen/Dense>
#include <algorithm>
#include <cmath>
#include <map>
#include <stdexcept>

#include "../core/csv.hpp"
#include "../core/philox.hpp"
#include "../core/price_base.hpp"
#include "../regional/convergence.hpp"  // normalize_state
#include "models.hpp"

namespace e47 {

namespace {

constexpr int kSpliceYear = 2011;  // organiser GSDP moves from 2004-05 to 2011-12 prices here

double interp(const std::map<int, double>& m, int year) {
  if (m.empty()) throw std::runtime_error("interp on empty series");
  auto hi = m.lower_bound(year);
  if (hi != m.end() && hi->first == year) return hi->second;
  if (hi == m.begin()) return hi->second;
  if (hi == m.end()) return std::prev(hi)->second;
  auto lo = std::prev(hi);
  const double w = static_cast<double>(year - lo->first) / (hi->first - lo->first);
  return lo->second + w * (hi->second - lo->second);
}

struct StateYear {
  double pop = 0, gsdp = 0;  // gsdp at constant prices (after the price-basis correction)
  double gsdp_read = 0;      // as read: the repeated-value rule compares these
  bool in_income = true;     // false: left out of income growth (GsdpOptions::exclude_states)
};

}  // namespace

IndiaSeries load_india_series(const std::string& npp_csv, const std::string& organiser_csv,
                              const std::string& urban_csv, int first, int last, const GsdpPriceBasis& basis,
                              const GsdpOptions& gsdp) {
  // 1. NPP national generation by fiscal year.
  std::map<int, IndiaObs> gen;
  {
    const CsvTable t = CsvTable::read(npp_csv);
    const size_t cy = t.col("fy_start_year"), ct = t.col("thermal_gwh"), ch = t.col("hydro_gwh"),
                 cn = t.col("nuclear_gwh"), cr = t.col("res_gwh"), cs = t.col("total_gwh");
    for (size_t i = 0; i < t.rows(); ++i) {
      const double y = t.num(i, cy), tot = t.num(i, cs);
      if (std::isnan(y) || std::isnan(tot) || tot <= 0) continue;
      IndiaObs o;
      o.year = static_cast<int>(y);
      o.gen_twh = tot / 1000.0;
      o.thermal_twh = t.num(i, ct) / 1000.0;
      o.hydro_twh = t.num(i, ch) / 1000.0;
      o.nuclear_twh = t.num(i, cn) / 1000.0;
      o.res_twh = t.num(i, cr) / 1000.0;
      gen[o.year] = o;
    }
  }

  // 2. Organiser data: state population and real GSDP. A state-year listed in the price-basis
  //    file is at current prices and is deflated to constant 2011-12 prices here.
  std::map<int, std::map<std::string, StateYear>> org;
  IndiaSeries s;
  {
    std::vector<std::string> excluded;
    for (const std::string& e : gsdp.exclude_states) excluded.push_back(normalize_state(e));
    const CsvTable t = CsvTable::read(organiser_csv);
    const size_t cs = t.col("State"), cy = t.col("Year"), cp = t.col("Population"), cg = t.col("GSDP (crore INR)");
    for (size_t i = 0; i < t.rows(); ++i) {
      const double y = t.num(i, cy), p = t.num(i, cp), g = t.num(i, cg);
      if (std::isnan(y) || std::isnan(p) || std::isnan(g) || p <= 0 || g <= 0) continue;
      const std::string& name = t.str(i, cs);
      const int year = static_cast<int>(y);
      StateYear v{p, g, g, true};
      if (basis.is_current(name, year)) {
        v.gsdp = basis.constant_2011_12(name, year, g);
        ++s.gsdp_current_corrected;
      }
      v.in_income = std::find(excluded.begin(), excluded.end(), normalize_state(name)) == excluded.end();
      org[year][lower(trim(name))] = v;
    }
  }
  if (org.empty()) throw std::runtime_error("organiser dataset is empty");

  // 3. Urban share, UN WUP 2018 (national definition).
  std::map<int, double> wup;
  {
    const CsvTable t = CsvTable::read(urban_csv);
    for (size_t i = 0; i < t.rows(); ++i) {
      if (t.str(i, t.col("state")) != "India") continue;
      if (t.str(i, t.col("source")).find("World Urbanization Prospects 2018") == std::string::npos) continue;
      wup[static_cast<int>(t.num(i, t.col("year")))] = t.num(i, t.col("value")) / 100.0;
    }
  }

  s.organiser_first = org.begin()->first;
  s.organiser_last = org.rbegin()->first;
  first = std::max(first, s.organiser_first);
  last = std::min(last, s.organiser_last);

  // Year-on-year growth over the states reported in both years, so that a state
  // entering or leaving the dataset does not move the national series.
  // Population growth uses every matched state. Income per person uses only the
  // states whose GSDP actually changed: a value copied unchanged from the year
  // before is a placeholder (several states repeat one figure for 2006-2010),
  // and counting it as zero growth would understate national growth. The rule
  // compares the values as read, before any price-basis correction.
  std::map<int, double> dp, dpc;
  for (int t = first + 1; t <= last; ++t) {
    auto a = org.find(t - 1), b = org.find(t);
    if (a == org.end() || b == org.end()) throw std::runtime_error("organiser data missing year " + std::to_string(t));
    double p0 = 0, p1 = 0, q0 = 0, q1 = 0, g0 = 0, g1 = 0;
    for (const auto& [st, v1] : b->second) {
      auto it = a->second.find(st);
      if (it == a->second.end()) continue;
      p0 += it->second.pop;
      p1 += v1.pop;
      if (v1.gsdp_read == it->second.gsdp_read) {
        ++s.stale_pairs_skipped;
        continue;
      }
      if (!v1.in_income || !it->second.in_income) {
        ++s.income_excluded_pairs;
        continue;
      }
      q0 += it->second.pop;
      q1 += v1.pop;
      g0 += it->second.gsdp;
      g1 += v1.gsdp;
    }
    if (g0 <= 0 || q0 <= 0) throw std::runtime_error("no state with GSDP growth in year " + std::to_string(t));
    dp[t] = std::log(p1 / p0);
    dpc[t] = std::log(g1 / g0) - std::log(q1 / q0);
  }
  // Base-year change: the jump from 2004-05 to 2011-12 prices is not growth.
  // Impute that year's per-person growth as the mean of its neighbours.
  if (first < kSpliceYear && kSpliceYear <= last) {
    if (!dpc.count(kSpliceYear - 1) || !dpc.count(kSpliceYear + 1))
      throw std::runtime_error("cannot splice the 2011 GSDP price base: a neighbouring year is missing");
    s.splice_growth = 0.5 * (dpc[kSpliceYear - 1] + dpc[kSpliceYear + 1]);
    dpc[kSpliceYear] = s.splice_growth;
  }

  // Levels: population anchored on the last year's organiser total; income per
  // person anchored on 2011 (first year at 2011-12 prices), thousand INR.
  std::map<int, double> ln_p, ln_y;
  ln_p[first] = 0;
  ln_y[first] = 0;
  for (int t = first + 1; t <= last; ++t) {
    ln_p[t] = ln_p[t - 1] + dp[t];
    ln_y[t] = ln_y[t - 1] + dpc[t];
  }
  double pop_last = 0;
  for (const auto& [st, v] : org.at(last)) pop_last += v.pop;
  double y_anchor = 0;
  const int anchor = (first <= kSpliceYear && kSpliceYear <= last) ? kSpliceYear : last;
  {
    double p = 0, g = 0;
    for (const auto& [st, v] : org.at(anchor)) {
      p += v.pop;
      g += v.gsdp;
    }
    y_anchor = g * 1e7 / p / 1000.0;  // crore INR -> INR, then thousand INR per person
  }

  for (int t = first; t <= last; ++t) {
    auto g = gen.find(t);
    if (g == gen.end()) throw std::runtime_error("NPP generation missing fiscal year " + std::to_string(t));
    IndiaObs o = g->second;
    o.pop = pop_last * std::exp(ln_p[t] - ln_p[last]);
    o.y = y_anchor * std::exp(ln_y[t] - ln_y[anchor]);
    o.urban = interp(wup, t);
    s.obs.push_back(o);
    if (t > first) s.gpc_growth.push_back(dpc[t]);
  }
  return s;
}

namespace {

constexpr int kCovidYear = 2020;  // FY2020-21

const IndiaObs& obs_at(const IndiaSeries& s, int year) {
  const int i = year - s.obs.front().year;
  if (i < 0 || i >= static_cast<int>(s.obs.size())) throw std::runtime_error("India series has no year " + std::to_string(year));
  return s.obs[i];
}

double x_of(const IndiaObs& o) { return std::log(o.y / 10.0); }

// Design matrix [1, x, D2020 (if in window)] and response ln e for first..last.
void india_design(const IndiaSeries& s, int first, int last, Eigen::MatrixXd& X, Eigen::VectorXd& y) {
  const int n = last - first + 1;
  const bool covid = first <= kCovidYear && kCovidYear <= last;
  X.resize(n, covid ? 3 : 2);
  y.resize(n);
  for (int i = 0; i < n; ++i) {
    const IndiaObs& o = obs_at(s, first + i);
    X(i, 0) = 1.0;
    X(i, 1) = x_of(o);
    if (covid) X(i, 2) = (first + i == kCovidYear) ? 1.0 : 0.0;
    y(i) = o.ln_e();
  }
}

}  // namespace

IndiaFit fit_india(const IndiaSeries& s, int first, int last) {
  IndiaFit f;
  f.first = first;
  f.last = last;
  Eigen::MatrixXd X;
  Eigen::VectorXd y;
  india_design(s, first, last, X, y);
  f.n = static_cast<int>(y.size());
  if (f.n < X.cols() + 2) throw std::runtime_error("India fit: too few years");
  const Eigen::MatrixXd xtx = X.transpose() * X;
  const Eigen::VectorXd b = xtx.ldlt().solve(X.transpose() * y);
  const Eigen::VectorXd r = y - X * b;
  const double s2 = r.squaredNorm() / (f.n - X.cols());
  const Eigen::MatrixXd cov = s2 * xtx.inverse();
  f.alpha = b(0);
  f.beta = b(1);
  f.beta_se = std::sqrt(cov(1, 1));
  f.covid = X.cols() > 2 ? b(2) : 0.0;
  f.resid_sd = std::sqrt(s2);
  f.resid.assign(r.data(), r.data() + r.size());
  double num = 0;
  for (int i = 1; i < f.n; ++i) num += (r(i) - r(i - 1)) * (r(i) - r(i - 1));
  f.dw = num / r.squaredNorm();
  // AR(1) persistence from the Durbin-Watson statistic (rho = 1 - DW / 2). With
  // 13 to 18 points a direct lag regression is dominated by the slow drift of the
  // early, supply-constrained years; the DW relation uses every residual.
  f.resid_rho = std::clamp(1.0 - 0.5 * f.dw, 0.0, 0.9);
  // Slope of the urban share on x, same regressors (so the excess urban share is
  // orthogonal to every column of X in the sample).
  {
    Eigen::VectorXd u(f.n);
    for (int i = 0; i < f.n; ++i) u(i) = obs_at(s, first + i).urban;
    f.kappa = xtx.ldlt().solve(X.transpose() * u)(1);
  }
  // First differences, leaving out the pandemic fall and rebound.
  {
    double sxy = 0, sxx = 0;
    for (int t = first + 1; t <= last; ++t) {
      // Pandemic fall and rebound, and the imputed (not observed) 2011 price-base change.
      if (t == kCovidYear || t == kCovidYear + 1 || t == kSpliceYear) continue;
      const double dx = x_of(obs_at(s, t)) - x_of(obs_at(s, t - 1));
      const double dy = obs_at(s, t).ln_e() - obs_at(s, t - 1).ln_e();
      sxy += dx * dy;
      sxx += dx * dx;
    }
    f.fd_beta = sxx > 0 ? sxy / sxx : 0.0;
  }
  return f;
}

std::vector<double> bootstrap_india_beta(const IndiaSeries& s, const IndiaFit& f, int replicates, int block,
                                         uint64_t seed) {
  Eigen::MatrixXd X;
  Eigen::VectorXd y;
  india_design(s, f.first, f.last, X, y);
  const int n = static_cast<int>(y.size());
  const Eigen::MatrixXd xtx = X.transpose() * X;
  const Eigen::LDLT<Eigen::MatrixXd> solver(xtx);
  const Eigen::VectorXd fitted = y - Eigen::Map<const Eigen::VectorXd>(f.resid.data(), n);
  // Resampling pool: residuals of the years without a dummy (a dummy year's
  // residual is zero by construction), rescaled for the degrees of freedom used
  // by the fit so the bootstrap does not understate the spread.
  std::vector<double> pool;
  for (int i = 0; i < n; ++i)
    if (!(X.cols() > 2 && X(i, 2) != 0.0)) pool.push_back(f.resid[static_cast<size_t>(i)]);
  const int m = static_cast<int>(pool.size());
  const double scale = std::sqrt(static_cast<double>(m) / std::max(1, n - static_cast<int>(X.cols())));
  for (double& e : pool) e *= scale;
  std::vector<double> out(replicates);
#pragma omp parallel for schedule(static)
  for (int r = 0; r < replicates; ++r) {
    const DrawRng rng(seed, static_cast<uint64_t>(r));
    Eigen::VectorXd ys(n);
    int i = 0, k = 0;
    while (i < n) {
      const uint32_t v = rng.bits(static_cast<uint32_t>(k++), 3).x;
      const int start = static_cast<int>((static_cast<uint64_t>(v) * m) >> 32);
      for (int j = 0; j < block && i < n; ++j, ++i) ys(i) = fitted(i) + pool[static_cast<size_t>((start + j) % m)];
    }
    out[r] = solver.solve(X.transpose() * ys)(1);
  }
  return out;
}

IndiaBacktest run_india_backtest(const IndiaSeries& s, int fit_first, int origin, int last, double gamma) {
  IndiaBacktest b;
  b.origin = origin;
  b.fit_first = fit_first;
  const IndiaFit f = fit_india(s, fit_first, origin);
  b.beta = f.beta;
  std::vector<double> ln_hist;
  for (int t = s.obs.front().year; t <= origin; ++t) ln_hist.push_back(std::log(obs_at(s, t).gen_twh));
  const DampedTrend dt = fit_damped_trend(ln_hist);
  const IndiaObs& o0 = obs_at(s, origin);
  int n = 0;
  for (int t = origin + 1; t <= last; ++t) {
    const IndiaObs& o = obs_at(s, t);
    IndiaBacktestPoint p;
    p.year = t;
    p.actual_twh = o.gen_twh;
    const double dx = x_of(o) - x_of(o0), du = o.urban - o0.urban;
    const double scale = o0.gen_twh * o.pop / o0.pop;
    p.elasticity_twh = scale * std::exp(india_delta_ln_e(f.beta, gamma, f.kappa, dx, du));
    p.elasticity_no_urban_twh = scale * std::exp(india_delta_ln_e(f.beta, 0.0, f.kappa, dx, du));
    p.damped_twh = std::exp(damped_forecast(dt, t - origin));
    const double pred[3] = {p.elasticity_twh, p.elasticity_no_urban_twh, p.damped_twh};
    for (int m = 0; m < 3; ++m) {
      const double ape = std::abs(pred[m] - p.actual_twh) / p.actual_twh;
      b.mape[m] += ape;
      b.ape_last[m] = ape;
    }
    b.points.push_back(p);
    ++n;
  }
  if (n > 0)
    for (double& m : b.mape) m /= n;
  return b;
}

}  // namespace e47
