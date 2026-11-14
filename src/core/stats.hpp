#pragma once
// Small statistics helpers used across the forecast and allocation code.
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <vector>

namespace e47 {

inline double mean(const std::vector<double>& v) {
  if (v.empty()) return 0.0;
  return std::accumulate(v.begin(), v.end(), 0.0) / static_cast<double>(v.size());
}

inline double stdev(const std::vector<double>& v) {
  if (v.size() < 2) return 0.0;
  const double m = mean(v);
  double s = 0;
  for (double x : v) s += (x - m) * (x - m);
  return std::sqrt(s / static_cast<double>(v.size() - 1));
}

// Linear-interpolated quantile (type 7, the R and NumPy default). Sorts a copy.
inline double quantile(std::vector<double> v, double q) {
  if (v.empty()) return 0.0;
  std::sort(v.begin(), v.end());
  const double h = (static_cast<double>(v.size()) - 1.0) * q;
  const size_t lo = static_cast<size_t>(std::floor(h));
  const size_t hi = std::min(lo + 1, v.size() - 1);
  return v[lo] + (h - static_cast<double>(lo)) * (v[hi] - v[lo]);
}

// Quantile from a fixed-width histogram over [lo, hi). Interpolates inside the bin.
inline double hist_quantile(const uint64_t* counts, int bins, double lo, double hi, double q) {
  uint64_t total = 0;
  for (int b = 0; b < bins; ++b) total += counts[b];
  if (total == 0) return lo;
  const double target = q * static_cast<double>(total);
  const double w = (hi - lo) / bins;
  double cum = 0;
  for (int b = 0; b < bins; ++b) {
    const double c = static_cast<double>(counts[b]);
    if (cum + c >= target && c > 0) return lo + w * (b + (target - cum) / c);
    cum += c;
  }
  return hi;
}

class Timer {
 public:
  Timer() : t0_(std::chrono::steady_clock::now()) {}
  double ms() const {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0_).count();
  }

 private:
  std::chrono::steady_clock::time_point t0_;
};

}  // namespace e47
