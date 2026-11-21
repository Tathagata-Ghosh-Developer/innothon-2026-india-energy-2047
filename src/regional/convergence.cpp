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

// Implied convergence speed over a span of years; NaN when 1 + slope * span <= 0
// (overshooting, not "no convergence").
double speed(double slope, double span) {
  const double v = 1.0 + slope * span;
  return v > 0 ? -std::log(v) / span : std::numeric_limits<double>::quiet_NaN();
}

// First year from year0, before year1, whose value (as read) equals neither the year before
// nor the year after; 0 if there is none.
int first_unrepeated_year(const std::map<int, double>& read, int year0, int year1) {
  for (auto it = read.lower_bound(year0); it != read.end() && it->first < year1; ++it) {
    const auto prev = read.find(it->first - 1), next = read.find(it->first + 1);
    const bool repeated =
        (prev != read.end() && prev->second == it->second) || (next != read.end() && next->second == it->second);
    if (!repeated) return it->first;
  }
  return 0;
}

}  // namespace

ConvergenceResult estimate_convergence(const std::string& path, int year0, int year1, int replicates,
                                       unsigned long long seed, const GsdpPriceBasis& basis) {
  const CsvTable t = CsvTable::read(path);
  const size_t c_state = t.col("State"), c_year = t.col("Year");
  const size_t c_pop = t.col("Population"), c_gsdp = t.col("GSDP (crore INR)");

  std::map<std::string, StateIncome> by_state;
  std::map<std::string, std::map<int, double>> read;  // state -> year -> GSDP as read (repeat rule)
  std::map<int, double> nat_gsdp, nat_pop;
  for (size_t r = 0; r < t.rows(); ++r) {
    const double y = t.num(r, c_year), p = t.num(r, c_pop), g_read = t.num(r, c_gsdp);
    if (std::isnan(y) || std::isnan(p) || std::isnan(g_read) || p <= 0 || g_read <= 0) continue;
    const int year = static_cast<int>(y);
    const double g = basis.constant_2011_12(t.str(r, c_state), year, g_read);
    const std::string s = normalize_state(t.str(r, c_state));
    auto& si = by_state[s];
    si.state = s;
    si.pop[year] = p;
    si.ln_rel[year] = std::log(g / p);  // temporarily ln GSDP per capita
    read[s][year] = g_read;
    nat_gsdp[year] += g;
    nat_pop[year] += p;
  }
  ConvergenceResult res;
  res.year0 = year0;
  res.year1 = year1;
  std::vector<double> x, dy, span;
  for (auto& [name, si] : by_state) {
    for (auto& [year, v] : si.ln_rel) v -= std::log(nat_gsdp[year] / nat_pop[year]);
    const int b0 = first_unrepeated_year(read[name], year0, year1);
    auto a = si.ln_rel.find(b0), b = si.ln_rel.find(year1);
    if (b0 > 0 && a != si.ln_rel.end() && b != si.ln_rel.end()) {
      si.base_year = b0;
      x.push_back(a->second);
      dy.push_back((b->second - a->second) / (year1 - b0));
      span.push_back(year1 - b0);
      if (b0 > year0) ++res.n_late_base;
    }
    res.states.push_back(si);
  }
  if (x.size() < 5) throw std::runtime_error("convergence: too few states with a base year and the end year");
  res.n_states = static_cast<int>(x.size());
  res.mean_span = mean(span);
  res.slope = ols_slope(x, dy);
  res.lambda = speed(res.slope, res.mean_span);

  // Bootstrap over states, parallel over replicates; each replicate uses its own mean span.
  std::vector<double> lam(replicates);
  const int n = res.n_states;
#pragma omp parallel for schedule(static)
  for (int r = 0; r < replicates; ++r) {
    const DrawRng rng(seed, static_cast<uint64_t>(r));
    std::vector<double> bx(n), by(n), bs(n);
    for (int i = 0; i < n; ++i) {
      const uint32_t v = rng.bits(static_cast<uint32_t>(i), 9).x;
      const int k = static_cast<int>((static_cast<uint64_t>(v) * n) >> 32);
      bx[i] = x[k];
      by[i] = dy[k];
      bs[i] = span[k];
    }
    lam[r] = speed(ols_slope(bx, by), mean(bs));
  }
  std::vector<double> finite;
  for (double l : lam)
    if (std::isfinite(l)) finite.push_back(l);
  res.lambda_se = stdev(finite);
  if (!std::isfinite(res.lambda)) throw std::runtime_error("convergence: slope implies no finite speed");
  return res;
}

}  // namespace e47
