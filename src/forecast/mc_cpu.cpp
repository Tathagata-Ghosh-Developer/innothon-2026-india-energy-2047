#include <omp.h>

#include <algorithm>
#include <cstring>
#include <limits>

#include "../core/stats.hpp"
#include "mc.hpp"

namespace e47 {

double McResult::quantile(const McInputs& in, int series, int t, double q) const {
  return hist_quantile(bins(series, t), kBins, in.hist_lo[series][t], in.hist_hi[series][t], q);
}

double McResult::mean(const McInputs& in, int series, int t) const {
  const uint64_t* b = bins(series, t);
  const double lo = in.hist_lo[series][t], w = (in.hist_hi[series][t] - lo) / kBins;
  double s = 0, n = 0;
  for (int i = 0; i < kBins; ++i) {
    s += static_cast<double>(b[i]) * (lo + w * (i + 0.5));
    n += static_cast<double>(b[i]);
  }
  return n > 0 ? s / n : 0.0;
}

void calibrate_hist_ranges(McInputs& in, const std::vector<float>& boot, uint64_t seed, int pilot) {
  constexpr int N = kSeries * kMaxYears;
  const int nt = omp_get_max_threads();
  std::vector<double> mn(static_cast<size_t>(nt) * N, std::numeric_limits<double>::infinity());
  std::vector<double> mx(static_cast<size_t>(nt) * N, 0.0);
#pragma omp parallel
  {
    const int tid = omp_get_thread_num();
    double* lmn = mn.data() + static_cast<size_t>(tid) * N;
    double* lmx = mx.data() + static_cast<size_t>(tid) * N;
#pragma omp for schedule(static)
    for (int i = 0; i < pilot; ++i) {
      // Distinct seed so the pilot does not reuse the production draws.
      simulate_draw<double>(
          in, boot.data(), seed ^ 0x5DEECE66Dull, static_cast<uint64_t>(i),
          [&](int s, int t, double v) {
            const int k = s * kMaxYears + t;
            lmn[k] = std::min(lmn[k], v);
            lmx[k] = std::max(lmx[k], v);
          },
          nullptr);
    }
  }
  for (int s = 0; s < kSeries; ++s)
    for (int t = 0; t < kMaxYears; ++t) {
      double a = std::numeric_limits<double>::infinity(), b = 0;
      for (int k = 0; k < nt; ++k) {
        a = std::min(a, mn[static_cast<size_t>(k) * N + s * kMaxYears + t]);
        b = std::max(b, mx[static_cast<size_t>(k) * N + s * kMaxYears + t]);
      }
      if (!(a < b)) {
        a = 0;
        b = 1;
      }
      in.hist_lo[s][t] = static_cast<float>(std::max(0.0, a * 0.7));
      in.hist_hi[s][t] = static_cast<float>(b * 1.4);
    }
}

McResult run_mc_cpu(const McInputs& in, const std::vector<float>& boot, uint64_t seed, uint64_t draws, int n_keep,
                    bool single_precision) {
  constexpr size_t H = static_cast<size_t>(kSeries) * kMaxYears * kBins;
  McResult r;
  r.draws = draws;
  r.n_years = in.n_years;
  r.hist.assign(H, 0);
  n_keep = static_cast<int>(std::min<uint64_t>(static_cast<uint64_t>(std::max(n_keep, 0)), draws));
  r.keep.assign(static_cast<size_t>(n_keep) * kKeepFeatures, 0.0f);
  const int nt = omp_get_max_threads();
  r.device = "CPU " + std::to_string(nt) + " threads (" + (single_precision ? "float" : "double") + ")";

  Timer timer;
  std::vector<uint32_t> local(H * static_cast<size_t>(nt), 0);
  const int64_t n = static_cast<int64_t>(draws);
#pragma omp parallel
  {
    uint32_t* h = local.data() + H * static_cast<size_t>(omp_get_thread_num());
#pragma omp for schedule(static)
    for (int64_t i = 0; i < n; ++i) {
      float* kp = i < n_keep ? r.keep.data() + static_cast<size_t>(i) * kKeepFeatures : nullptr;
      if (single_precision) {
        simulate_draw<float>(
            in, boot.data(), seed, static_cast<uint64_t>(i),
            [&](int s, int t, float v) { ++h[(static_cast<size_t>(s) * kMaxYears + t) * kBins + hist_bin(in, s, t, v)]; },
            kp);
      } else {
        simulate_draw<double>(
            in, boot.data(), seed, static_cast<uint64_t>(i),
            [&](int s, int t, double v) { ++h[(static_cast<size_t>(s) * kMaxYears + t) * kBins + hist_bin(in, s, t, v)]; },
            kp);
      }
    }
    // Parallel merge: each thread sums one slice of the histogram across threads.
#pragma omp barrier
#pragma omp for schedule(static)
    for (int64_t j = 0; j < static_cast<int64_t>(H); ++j) {
      uint64_t s = 0;
      for (int k = 0; k < nt; ++k) s += local[H * static_cast<size_t>(k) + static_cast<size_t>(j)];
      r.hist[static_cast<size_t>(j)] = s;
    }
  }
  r.ms = timer.ms();
  return r;
}

}  // namespace e47
