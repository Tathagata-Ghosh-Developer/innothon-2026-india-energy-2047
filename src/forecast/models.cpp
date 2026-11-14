#include "models.hpp"

#include <Eigen/Dense>
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>

#include "../core/philox.hpp"

namespace e47 {

namespace {

constexpr double kIncomeRef = 10000.0;  // x = ln(y / 10,000)

std::vector<int> all_countries(const Design& d) {
  std::vector<int> s(d.n_countries());
  std::iota(s.begin(), s.end(), 0);
  return s;
}

// Within-country sufficient statistics for a model that is linear in K parameters.
// feature(row, out) writes the K regressors for one row.
template <int K, class Feature>
void within_stats(const Design& d, const std::vector<int>& sample, Feature feature,
                  Eigen::Matrix<double, K, K>& xtx, Eigen::Matrix<double, K, 1>& xty, double& yty) {
  xtx.setZero();
  xty.setZero();
  yty = 0;
  Eigen::Matrix<double, K, 1> f, fm;
  for (int c : sample) {
    const size_t b = d.start[c], e = d.start[c + 1];
    const double n = static_cast<double>(e - b);
    fm.setZero();
    double ym = 0;
    for (size_t r = b; r < e; ++r) {
      feature(d.rows[r], f);
      fm += f;
      ym += d.rows[r].ln_e;
    }
    fm /= n;
    ym /= n;
    for (size_t r = b; r < e; ++r) {
      feature(d.rows[r], f);
      f -= fm;
      const double yc = d.rows[r].ln_e - ym;
      xtx.noalias() += f * f.transpose();
      xty.noalias() += f * yc;
      yty += yc * yc;
    }
  }
}

template <int K>
Eigen::Matrix<double, K, 1> solve_ols(const Eigen::Matrix<double, K, K>& xtx,
                                      const Eigen::Matrix<double, K, 1>& xty, double& rss, double yty) {
  const Eigen::Matrix<double, K, 1> beta = xtx.ldlt().solve(xty);
  rss = yty - beta.dot(xty);
  return beta;
}

struct QuadFeature {
  void operator()(const DesignRow& r, Eigen::Matrix<double, 4, 1>& f) const {
    f << r.x, r.x * r.x, r.u, r.tau;
  }
};

struct GompertzFeature {
  double b;
  void operator()(const DesignRow& r, Eigen::Matrix<double, 3, 1>& f) const {
    f << -std::exp(-b * r.y), r.u, r.tau;
  }
};

// Residual sum of squares of the Gompertz model with b fixed; (a, c, d) solved by OLS.
double gompertz_rss(const Design& d, const std::vector<int>& sample, double b, Eigen::Vector3d* coef) {
  Eigen::Matrix3d xtx;
  Eigen::Vector3d xty;
  double yty = 0, rss = 0;
  within_stats<3>(d, sample, GompertzFeature{b}, xtx, xty, yty);
  const Eigen::Vector3d beta = solve_ols<3>(xtx, xty, rss, yty);
  if (coef) *coef = beta;
  return rss;
}

std::vector<int> bootstrap_sample(size_t n_countries, uint64_t seed, uint64_t replicate) {
  const DrawRng rng(seed, replicate);
  std::vector<int> s(n_countries);
  for (size_t i = 0; i < n_countries; i += 4) {
    const U32x4 b = rng.bits(static_cast<uint32_t>(i / 4), 0);
    const uint32_t v[4] = {b.x, b.y, b.z, b.w};
    for (size_t k = 0; k < 4 && i + k < n_countries; ++k)
      s[i + k] = static_cast<int>((static_cast<uint64_t>(v[k]) * n_countries) >> 32);
  }
  return s;
}

}  // namespace

DesignRow design_row(double kwh_pc, double gdp_pc, double urban, int year) {
  DesignRow r;
  r.ln_e = std::log(kwh_pc);
  r.x = std::log(gdp_pc / kIncomeRef);
  r.y = gdp_pc / 1000.0;
  r.u = urban;
  r.tau = (year - 2000) / 10.0;
  return r;
}

Design make_design(const Panel& p) {
  Design d;
  d.start.push_back(0);
  for (const auto& c : p.countries) {
    for (const auto& o : c.obs) d.rows.push_back(design_row(o));
    d.start.push_back(d.rows.size());
    d.iso3.push_back(c.iso3);
  }
  return d;
}

double FeQuad::elasticity(double y_dollars) const { return b1 + 2.0 * b2 * std::log(y_dollars / kIncomeRef); }

double FeGompertz::elasticity(double y_dollars) const {
  const double y = y_dollars / 1000.0;
  return a * b * y * std::exp(-b * y);
}

FeQuad fit_fe_quad(const Design& d, const std::vector<int>& sample_in) {
  const std::vector<int> sample = sample_in.empty() ? all_countries(d) : sample_in;
  Eigen::Matrix4d xtx;
  Eigen::Vector4d xty;
  double yty = 0, rss = 0;
  within_stats<4>(d, sample, QuadFeature{}, xtx, xty, yty);
  const Eigen::Vector4d beta = solve_ols<4>(xtx, xty, rss, yty);
  return FeQuad{beta[0], beta[1], beta[2], beta[3]};
}

FeGompertz fit_fe_gompertz(const Design& d, const std::vector<int>& sample_in) {
  const std::vector<int> sample = sample_in.empty() ? all_countries(d) : sample_in;
  // Variable projection: the model is linear in (a, c, d) for fixed b, so search
  // b on a log grid, then refine with golden-section search.
  constexpr int kGrid = 96;
  const double lo = std::log(0.002), hi = std::log(1.0);
  double best_rss = std::numeric_limits<double>::infinity();
  int best = 0;
  for (int i = 0; i < kGrid; ++i) {
    const double b = std::exp(lo + (hi - lo) * i / (kGrid - 1));
    const double rss = gompertz_rss(d, sample, b, nullptr);
    if (rss < best_rss) {
      best_rss = rss;
      best = i;
    }
  }
  double l = lo + (hi - lo) * std::max(best - 1, 0) / (kGrid - 1);
  double h = lo + (hi - lo) * std::min(best + 1, kGrid - 1) / (kGrid - 1);
  const double gr = 0.5 * (std::sqrt(5.0) - 1.0);
  double m1 = h - gr * (h - l), m2 = l + gr * (h - l);
  double f1 = gompertz_rss(d, sample, std::exp(m1), nullptr);
  double f2 = gompertz_rss(d, sample, std::exp(m2), nullptr);
  for (int it = 0; it < 40; ++it) {
    if (f1 < f2) {
      h = m2;
      m2 = m1;
      f2 = f1;
      m1 = h - gr * (h - l);
      f1 = gompertz_rss(d, sample, std::exp(m1), nullptr);
    } else {
      l = m1;
      m1 = m2;
      f1 = f2;
      m2 = l + gr * (h - l);
      f2 = gompertz_rss(d, sample, std::exp(m2), nullptr);
    }
  }
  const double b = std::exp(0.5 * (l + h));
  Eigen::Vector3d coef;
  gompertz_rss(d, sample, b, &coef);
  return FeGompertz{coef[0], b, coef[1], coef[2]};
}

namespace {
template <class Model, class Predict>
FitStats fit_stats_impl(const Design& d, const Model& m, Predict predict) {
  FitStats s;
  s.countries = d.n_countries();
  double tss = 0;
  for (size_t c = 0; c < d.n_countries(); ++c) {
    const size_t b = d.start[c], e = d.start[c + 1];
    double rm = 0, ym = 0;
    for (size_t r = b; r < e; ++r) {
      rm += d.rows[r].ln_e - predict(m, d.rows[r]);
      ym += d.rows[r].ln_e;
    }
    rm /= static_cast<double>(e - b);
    ym /= static_cast<double>(e - b);
    for (size_t r = b; r < e; ++r) {
      const double res = d.rows[r].ln_e - predict(m, d.rows[r]) - rm;
      s.rss += res * res;
      tss += (d.rows[r].ln_e - ym) * (d.rows[r].ln_e - ym);
    }
    s.n += e - b;
  }
  s.r2_within = tss > 0 ? 1.0 - s.rss / tss : 0.0;
  return s;
}
}  // namespace

FitStats fit_stats(const Design& d, const FeQuad& m) {
  return fit_stats_impl(d, m, [](const FeQuad& q, const DesignRow& r) {
    return q.b1 * r.x + q.b2 * r.x * r.x + q.g * r.u + q.d * r.tau;
  });
}

FitStats fit_stats(const Design& d, const FeGompertz& m) {
  return fit_stats_impl(d, m, [](const FeGompertz& q, const DesignRow& r) {
    return -q.a * std::exp(-q.b * r.y) + q.c * r.u + q.d * r.tau;
  });
}

std::vector<FeQuad> bootstrap_fe_quad(const Design& d, int replicates, uint64_t seed) {
  // Per-country within statistics are additive, so each replicate is a weighted sum.
  const int nc = static_cast<int>(d.n_countries());
  std::vector<Eigen::Matrix4d> A(nc);
  std::vector<Eigen::Vector4d> B(nc);
  std::vector<double> Y(nc);
#pragma omp parallel for schedule(static)
  for (int c = 0; c < nc; ++c) within_stats<4>(d, {c}, QuadFeature{}, A[c], B[c], Y[c]);

  std::vector<FeQuad> out(replicates);
#pragma omp parallel for schedule(static)
  for (int r = 0; r < replicates; ++r) {
    const std::vector<int> s = bootstrap_sample(nc, seed, static_cast<uint64_t>(r));
    Eigen::Matrix4d xtx = Eigen::Matrix4d::Zero();
    Eigen::Vector4d xty = Eigen::Vector4d::Zero();
    for (int c : s) {
      xtx += A[c];
      xty += B[c];
    }
    const Eigen::Vector4d beta = xtx.ldlt().solve(xty);
    out[r] = FeQuad{beta[0], beta[1], beta[2], beta[3]};
  }
  return out;
}

std::vector<FeGompertz> bootstrap_fe_gompertz(const Design& d, int replicates, uint64_t seed) {
  std::vector<FeGompertz> out(replicates);
  const size_t nc = d.n_countries();
#pragma omp parallel for schedule(dynamic, 4)
  for (int r = 0; r < replicates; ++r) {
    const std::vector<int> s = bootstrap_sample(nc, seed ^ 0x9E3779B97F4A7C15ull, static_cast<uint64_t>(r));
    out[r] = fit_fe_gompertz(d, s);
  }
  return out;
}

double delta_ln_e(const FeQuad& m, const DesignRow& f, const DesignRow& t) {
  return m.b1 * (t.x - f.x) + m.b2 * (t.x * t.x - f.x * f.x) + m.g * (t.u - f.u) + m.d * (t.tau - f.tau);
}

double delta_ln_e(const FeGompertz& m, const DesignRow& f, const DesignRow& t) {
  return -m.a * (std::exp(-m.b * t.y) - std::exp(-m.b * f.y)) + m.c * (t.u - f.u) + m.d * (t.tau - f.tau);
}

namespace {
double damped_sse(const std::vector<double>& y, double alpha, double beta, double phi, double* level,
                  double* trend) {
  double l = y[0], b = y[1] - y[0], sse = 0;
  for (size_t t = 1; t < y.size(); ++t) {
    const double f = l + phi * b;
    const double e = y[t] - f;
    sse += e * e;
    l = f + alpha * e;
    b = phi * b + alpha * beta * e;
  }
  if (level) *level = l;
  if (trend) *trend = b;
  return sse;
}
}  // namespace

DampedTrend fit_damped_trend(const std::vector<double>& y) {
  if (y.size() < 5) throw std::runtime_error("damped trend needs at least 5 points");
  constexpr int NA = 40, NB = 41, NP = 40;
  const int total = NA * NB * NP;
  double best = std::numeric_limits<double>::infinity();
  int best_i = 0;
#pragma omp parallel
  {
    double lbest = std::numeric_limits<double>::infinity();
    int li = 0;
#pragma omp for schedule(static) nowait
    for (int i = 0; i < total; ++i) {
      const double alpha = 0.025 + 0.025 * (i / (NB * NP));
      const double beta = 0.025 * ((i / NP) % NB);
      const double phi = 0.80 + 0.005 * (i % NP);
      const double s = damped_sse(y, alpha, beta, phi, nullptr, nullptr);
      if (s < lbest) {
        lbest = s;
        li = i;
      }
    }
#pragma omp critical
    if (lbest < best || (lbest == best && li < best_i)) {
      best = lbest;
      best_i = li;
    }
  }
  DampedTrend m;
  m.alpha = 0.025 + 0.025 * (best_i / (NB * NP));
  m.beta = 0.025 * ((best_i / NP) % NB);
  m.phi = 0.80 + 0.005 * (best_i % NP);
  const double sse = damped_sse(y, m.alpha, m.beta, m.phi, &m.level, &m.trend);
  m.n = static_cast<int>(y.size());
  m.sigma = std::sqrt(sse / std::max<int>(1, m.n - 1 - 3));
  return m;
}

double damped_forecast(const DampedTrend& m, int h) {
  double acc = 0, p = 1;
  for (int i = 1; i <= h; ++i) {
    p *= m.phi;
    acc += p;
  }
  return m.level + acc * m.trend;
}

}  // namespace e47
