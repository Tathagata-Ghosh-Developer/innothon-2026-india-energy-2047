#include "convergence.hpp"

#include <cctype>
#include <cmath>
#include <limits>
#include <stdexcept>

#include "../core/csv.hpp"
#include "../core/philox.hpp"
#include "../core/price_base.hpp"
#include "../core/stats.hpp"

namespace e47 {

std::string normalize_state(const std::string& name) {
  std::string s = lower(trim(name));
  std::string out;
  for (size_t i = 0; i < s.size(); ++i) {
    const char c = s[i];
    if (c == '&') {
      out += " and ";
    } else if (std::isalnum(static_cast<unsigned char>(c))) {
      out.push_back(c);
    } else {
      out.push_back(' ');
    }
  }
  // collapse whitespace
  std::string r;
  bool space = false;
  for (char c : out) {
    if (c == ' ') {
      space = !r.empty();
    } else {
      if (space) r.push_back(' ');
      r.push_back(c);
      space = false;
    }
  }
  return r;
}

namespace {

// OLS slope of y on x with intercept.
double ols_slope(const std::vector<double>& x, const std::vector<double>& y) {
  const double mx = mean(x), my = mean(y);
  double sxy = 0, sxx = 0;
  for (size_t i = 0; i < x.size(); ++i) {
    sxy += (x[i] - mx) * (y[i] - my);
    sxx += (x[i] - mx) * (x[i] - mx);
  }
  return sxx > 0 ? sxy / sxx : 0.0;
}

// Implied convergence speed; NaN when 1 + slope * span <= 0 (overshooting, not "no convergence").
double speed(double slope, int span) {
  const double v = 1.0 + slope * span;
  return v > 0 ? -std::log(v) / span : std::numeric_limits<double>::quiet_NaN();
}

}  // namespace

ConvergenceResult estimate_convergence(const std::string& path, int year0, int year1, int replicates,
                                       unsigned long long seed, const GsdpPriceBasis* basis) {
  const CsvTable t = CsvTable::read(path);
  const size_t c_state = t.col("State"), c_year = t.col("Year");
  const size_t c_pop = t.col("Population"), c_gsdp = t.col("GSDP (crore INR)");

  std::map<std::string, StateIncome> by_state;
  std::map<int, double> nat_gsdp, nat_pop;
  for (size_t r = 0; r < t.rows(); ++r) {
    const double y = t.num(r, c_year), p = t.num(r, c_pop), g_read = t.num(r, c_gsdp);
    if (std::isnan(y) || std::isnan(p) || std::isnan(g_read) || p <= 0 || g_read <= 0) continue;
    const int year = static_cast<int>(y);
    const double g = basis ? basis->constant_2011_12(t.str(r, c_state), year, g_read) : g_read;
    const std::string s = normalize_state(t.str(r, c_state));
    auto& si = by_state[s];
    si.state = s;
    si.pop[year] = p;
    si.ln_rel[year] = std::log(g / p);  // temporarily ln GSDP per capita
    nat_gsdp[year] += g;
    nat_pop[year] += p;
  }
  ConvergenceResult res;
  res.year0 = year0;
  res.year1 = year1;
  std::vector<double> x, dy;
  for (auto& [name, si] : by_state) {
    for (auto& [year, v] : si.ln_rel) v -= std::log(nat_gsdp[year] / nat_pop[year]);
    auto a = si.ln_rel.find(year0), b = si.ln_rel.find(year1);
    if (a != si.ln_rel.end() && b != si.ln_rel.end()) {
      x.push_back(a->second);
      dy.push_back((b->second - a->second) / (year1 - year0));
    }
    res.states.push_back(si);
  }
  if (x.size() < 5) throw std::runtime_error("convergence: too few states with both years");
  res.n_states = static_cast<int>(x.size());
  res.slope = ols_slope(x, dy);
  res.lambda = speed(res.slope, year1 - year0);

  // Bootstrap over states, parallel over replicates.
  std::vector<double> lam(replicates);
  const int n = res.n_states;
#pragma omp parallel for schedule(static)
  for (int r = 0; r < replicates; ++r) {
    const DrawRng rng(seed, static_cast<uint64_t>(r));
    std::vector<double> bx(n), by(n);
    for (int i = 0; i < n; ++i) {
      const uint32_t v = rng.bits(static_cast<uint32_t>(i), 9).x;
      const int k = static_cast<int>((static_cast<uint64_t>(v) * n) >> 32);
      bx[i] = x[k];
      by[i] = dy[k];
    }
    lam[r] = speed(ols_slope(bx, by), year1 - year0);
  }
  std::vector<double> finite;
  for (double l : lam)
    if (std::isfinite(l)) finite.push_back(l);
  res.lambda_se = stdev(finite);
  if (!std::isfinite(res.lambda)) throw std::runtime_error("convergence: slope implies no finite speed");
  return res;
}

}  // namespace e47
