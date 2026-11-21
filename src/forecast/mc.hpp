#pragma once
// Monte Carlo ensemble for national and regional generation requirement.
//
// One draw = one internally consistent future: a population path, an urban
// share path, a GDP-per-capita path with persistent and transitory shocks, a
// model choice (FE-Quad, FE-Gompertz, damped trend) with parameters taken from
// a bootstrap replicate, an inter-state income convergence speed, and an optional
// add-on (a fixed range, plus independent demand-feature draws in the joint feature
// scenario). The draw yields national generation and its split into the five grid
// regions for every projection year.
//
// simulate_draw() is shared by the CPU (OpenMP) and GPU (CUDA) drivers. Random
// inputs come from Philox keyed by (seed, draw index), so both drivers see the
// same random numbers for the same draw.
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "../core/hd.hpp"
#include "../core/philox.hpp"

namespace e47 {

constexpr int kMaxYears = 32;
constexpr int kMaxStates = 40;
constexpr int kRegions = 5;
constexpr int kSeries = 1 + kRegions;  // 0 = national, 1..5 = regions
constexpr int kBins = 1024;
constexpr int kBootStride = 8;          // q_b1 q_b2 q_g q_d g_a g_b g_c g_d
constexpr int kKeepFeatures = 8;        // per-draw inputs kept for sensitivity analysis
constexpr int kMaxFeatures = 16;        // demand-feature draws per Monte Carlo draw (joint feature scenario)

enum Model : int { kFeQuad = 0, kFeGompertz = 1, kDamped = 2 };

// Distribution of one demand-feature draw: triangular(lo, mode, hi) or uniform(lo, hi).
enum FeatureKind : int { kFeatureTriangular = 0, kFeatureUniform = 1 };

// One demand feature of the joint feature scenario (host side; pack_features copies it into
// McInputs): each draw adds sign x a value from the distribution, TWh in the final year.
struct FeatureDraw {
  int kind = kFeatureTriangular;
  double sign = 1, lo = 0, mode = 0, hi = 0;
};

inline const char* region_name(int r) {
  static const char* names[kRegions] = {"Northern", "Western", "Southern", "Eastern", "North-Eastern"};
  return names[r];
}

// Plain-old-data inputs; small enough (< 64 KB) for CUDA constant memory.
struct McInputs {
  int base_year;  // last observed year
  int n_years;    // projection horizon in years after base_year

  // India at the base year
  float ln_e0;     // ln(kWh generated per person)
  float y0;        // income per person in thousands: INR at 2011-12 prices (India core) or PPP $ (cross-country core)
  float u0;        // urban share
  float gen0_twh;  // total generation, TWh

  // Index t = 0 is base_year + 1.
  float pop_med[kMaxYears];  // population, millions
  float pop_sd[kMaxYears];   // standard deviation, millions
  float urb_med[kMaxYears];  // urban share
  float urb_sd[kMaxYears];
  float g_mean[kMaxYears];   // mean growth of ln(GDP per capita)
  float g_level_sd;          // persistent shift of annual growth
  float g_shock_sd;          // transitory annual shock
  float g_rho;               // AR(1) persistence of shocks

  float w_model[3];  // model weights (sum to 1)
  int n_boot;        // bootstrap replicates available
  // 1 = slot 1 is also a quadratic-form model whose parameters sit in p[4..7]
  // (India-only core). 0 = slot 1 is the Gompertz model (cross-country core).
  int quad_only;
  // Model error of the quadratic-form models (India core): an AR(1) residual that
  // starts at the base year's residual and reverts to the fitted relation.
  // All zero = no model-error term (cross-country core).
  float resid0;          // residual of ln(kWh per person) in the base year
  float resid_rho;       // AR(1) persistence of the residual
  float resid_innov_sd;  // innovation s.d. = residual s.d. x sqrt(1 - rho^2)

  // Damped trend on ln(total generation)
  float dt_level, dt_trend, dt_phi, dt_alpha, dt_beta, dt_sigma;

  // Optional add-on, TWh in the final year (signed), drawn uniformly between addon_lo and
  // addon_hi and ramped linearly from the base year: a demand-feature scenario or the
  // dashboard's add-on sliders, never the headline.
  float addon_lo, addon_hi;
  // Optional demand-feature draws (the joint feature scenario): n_features independent
  // draws, each feat_sign x a value from triangular(feat_lo, feat_mode, feat_hi) or
  // uniform(feat_lo, feat_hi) (feat_kind), TWh in the final year, added to the add-on with
  // the same ramp. 0 = none. Their uniforms come from a stream of the draw used nowhere
  // else, so every other random input of the draw is the same with or without them.
  int n_features;
  int feat_kind[kMaxFeatures];
  float feat_sign[kMaxFeatures];
  float feat_lo[kMaxFeatures];
  float feat_mode[kMaxFeatures];
  float feat_hi[kMaxFeatures];

  // Regional split: state population and relative income only. The challenge data
  // have no state urban share, so the split has no state urbanisation term; the
  // national urban effect is already in the national total being split.
  int n_states;
  int state_region[kMaxStates];
  float state_pop_share[kMaxStates][kMaxYears];  // share of national population
  float state_ln_rel_income[kMaxStates];         // ln(state GDP pc / national GDP pc) at base
  float conv_mean, conv_sd;                      // convergence speed of relative income, per year
  float rel_income_lag;                          // years between the relative-income year and base_year

  // Histogram ranges per series and year
  float hist_lo[kSeries][kMaxYears];
  float hist_hi[kSeries][kMaxYears];
};

template <class Real>
E47_HD Real clamp01(Real v) {
  return v < Real(0) ? Real(0) : (v > Real(1) ? Real(1) : v);
}

// Copies the demand-feature distributions into the kernel inputs (n_features = 0 for none).
inline void pack_features(McInputs& in, const std::vector<FeatureDraw>& d) {
  if (d.size() > static_cast<size_t>(kMaxFeatures))
    throw std::invalid_argument("pack_features: " + std::to_string(d.size()) + " features, at most " +
                                std::to_string(kMaxFeatures));
  in.n_features = static_cast<int>(d.size());
  for (int j = 0; j < kMaxFeatures; ++j) {
    const bool on = j < in.n_features;
    in.feat_kind[j] = on ? d[j].kind : kFeatureTriangular;
    in.feat_sign[j] = on ? static_cast<float>(d[j].sign) : 0.0f;
    in.feat_lo[j] = on ? static_cast<float>(d[j].lo) : 0.0f;
    in.feat_mode[j] = on ? static_cast<float>(d[j].mode) : 0.0f;
    in.feat_hi[j] = on ? static_cast<float>(d[j].hi) : 0.0f;
  }
}

// Inverse distribution function of one demand-feature draw at u in (0, 1]. A distribution
// with hi <= lo is the constant lo.
template <class Real>
E47_HD Real feature_inverse_cdf(int kind, Real lo, Real mode, Real hi, Real u) {
  using std::sqrt;
  const Real w = hi - lo;
  if (!(w > Real(0))) return lo;
  if (kind == kFeatureUniform) return lo + w * u;
  const Real a = mode - lo;
  if (u * w <= a) return lo + sqrt(u * w * a);
  return hi - sqrt((Real(1) - u) * w * (hi - mode));
}

// Sum of the demand-feature draws of one Monte Carlo draw: sum of sign x value, TWh in the
// final year. Uniforms come from stream 6 of the draw, four per step.
template <class Real>
E47_HD Real feature_draw_total(const McInputs& in, const DrawRng& rng) {
  Real s = 0;
  Real u[4] = {0, 0, 0, 0};
  for (int j = 0; j < in.n_features; ++j) {
    if ((j & 3) == 0) rng.template uniform4<Real>(static_cast<uint32_t>(j >> 2), 6, u);
    s += Real(in.feat_sign[j]) *
         feature_inverse_cdf<Real>(in.feat_kind[j], Real(in.feat_lo[j]), Real(in.feat_mode[j]), Real(in.feat_hi[j]),
                                   u[j & 3]);
  }
  return s;
}

// Parameters of the quadratic-form model used by `model`: slot 0 reads p[0..3];
// slot 1 reads p[4..7] when quad_only is set.
E47_HD const float* quad_params(int model, const float* p, int quad_only) {
  return (model == kFeGompertz && quad_only) ? p + 4 : p;
}

// Per-state log index of generation per person from the state's income, up to terms
// common to all states (the urban term is common: there is no state urban share).
template <class Real>
E47_HD Real state_log_index(int model, const float* p, Real y_k, int quad_only) {
  using std::exp;
  using std::log;
  if (model == kFeGompertz && !quad_only) return -Real(p[4]) * exp(-Real(p[5]) * y_k);
  const float* q = quad_params(model, p, quad_only);
  const Real x = log(y_k * Real(0.1));  // ln(y / 10) with y in thousands (any currency; only changes matter)
  return Real(q[0]) * x + Real(q[1]) * x * x;
}

// Model choice is shared by each group of 32 consecutive draws (one GPU warp),
// so the threads of a warp follow the same branch. Groups are still random.
template <class Real>
E47_HD int pick_model(const McInputs& in, uint64_t seed, uint64_t draw) {
  Real g[4];
  DrawRng(seed ^ 0xA5A5A5A5u, draw >> 5).template uniform4<Real>(0, 5, g);
  if (g[0] < Real(in.w_model[0])) return kFeQuad;
  if (g[0] < Real(in.w_model[0] + in.w_model[1])) return kFeGompertz;
  return kDamped;
}

// Simulates one draw. emit(series, t, value) receives every output.
// keep (optional) receives kKeepFeatures draw-level inputs and the final national value.
E47_HD_TEMPLATE
template <class Real, class Emit>
E47_HD void simulate_draw(const McInputs& in, const float* boot, uint64_t seed, uint64_t draw, Emit&& emit,
                          float* keep) {
  using std::exp;
  using std::log;
  const DrawRng rng(seed, draw);

  Real z[4], uu[4];
  rng.template normal4<Real>(0, 1, z);    // population, urban, growth level, convergence
  rng.template uniform4<Real>(0, 2, uu);  // spare, bootstrap pick, add-on, spare

  const int model = pick_model<Real>(in, seed, draw);
  int k = static_cast<int>(uu[1] * Real(in.n_boot));
  k = k >= in.n_boot ? in.n_boot - 1 : k;
  const float* p = boot + static_cast<size_t>(k) * kBootStride;

  const Real lambda = Real(in.conv_mean) + Real(in.conv_sd) * z[3];
  Real addon = Real(in.addon_lo) + (Real(in.addon_hi) - Real(in.addon_lo)) * uu[2];
  if (in.n_features > 0) addon += feature_draw_total<Real>(in, rng);
  const Real g_level = Real(in.g_level_sd) * z[2];

  const bool quad = model == kFeQuad || (model == kFeGompertz && in.quad_only);
  const float* q = quad_params(model, p, in.quad_only);
  const Real y0 = Real(in.y0);
  const Real x0 = log(y0 * Real(0.1));
  const Real base_quad = Real(q[0]) * x0 + Real(q[1]) * x0 * x0 + Real(q[2]) * Real(in.u0);
  const Real base_gomp = -Real(p[4]) * exp(-Real(p[5]) * y0) + Real(p[6]) * Real(in.u0);

  Real ln_y = log(y0);
  Real shock = 0;
  Real resid = Real(in.resid0);
  Real dl = Real(in.dt_level), db = Real(in.dt_trend);
  Real gen = Real(in.gen0_twh);

  for (int t = 0; t < in.n_years; ++t) {
    Real nz[4];
    rng.template normal4<Real>(static_cast<uint32_t>(t + 1), 3, nz);
    shock = Real(in.g_rho) * shock + Real(in.g_shock_sd) * nz[0];
    ln_y += Real(in.g_mean[t]) + g_level + shock;
    const Real y = exp(ln_y);
    const Real u = clamp01(Real(in.urb_med[t]) + Real(in.urb_sd[t]) * z[1]);
    const Real pop = Real(in.pop_med[t]) + Real(in.pop_sd[t]) * z[0];
    const Real dtau = Real(t + 1) * Real(0.1);

    if (model == kDamped) {
      const Real f = dl + Real(in.dt_phi) * db;
      const Real eps = Real(in.dt_sigma) * nz[1];
      dl = f + Real(in.dt_alpha) * eps;
      db = Real(in.dt_phi) * db + Real(in.dt_alpha) * Real(in.dt_beta) * eps;
      gen = exp(f + eps);
    } else {
      Real d;
      if (quad) {
        const Real x = log(y * Real(0.1));
        d = Real(q[0]) * x + Real(q[1]) * x * x + Real(q[2]) * u - base_quad + Real(q[3]) * dtau;
        resid = Real(in.resid_rho) * resid + Real(in.resid_innov_sd) * nz[2];
        d += resid - Real(in.resid0);
      } else {
        d = -Real(p[4]) * exp(-Real(p[5]) * y) + Real(p[6]) * u - base_gomp + Real(p[7]) * dtau;
      }
      gen = pop * exp(Real(in.ln_e0) + d) * Real(0.001);  // millions x kWh -> TWh
    }
    gen += addon * Real(t + 1) / Real(in.n_years);
    emit(0, t, gen);

    if (in.n_states > 0) {
      // Regional requirement share: population x per-person index from the same income response.
      const int rmodel = model == kDamped ? kFeQuad : model;
      Real reg[kRegions] = {0, 0, 0, 0, 0};
      Real total = 0;
      const Real decay = exp(-lambda * (Real(t + 1) + Real(in.rel_income_lag)));
      for (int s = 0; s < in.n_states; ++s) {
        const Real ys = y * exp(Real(in.state_ln_rel_income[s]) * decay);
        const Real w = Real(in.state_pop_share[s][t]) * exp(state_log_index<Real>(rmodel, p, ys, in.quad_only));
        reg[in.state_region[s]] += w;
        total += w;
      }
      for (int r = 0; r < kRegions; ++r) emit(1 + r, t, gen * reg[r] / total);
    }
  }

  if (keep) {
    keep[0] = static_cast<float>(z[0]);
    keep[1] = static_cast<float>(z[1]);
    keep[2] = static_cast<float>(z[2]);
    keep[3] = static_cast<float>(lambda);
    keep[4] = static_cast<float>(model);
    keep[5] = static_cast<float>(k);
    keep[6] = static_cast<float>(addon);
    keep[7] = static_cast<float>(gen);
  }
}

template <class Real>
E47_HD int hist_bin(const McInputs& in, int series, int t, Real v) {
  const Real lo = Real(in.hist_lo[series][t]), hi = Real(in.hist_hi[series][t]);
  const Real pos = (v - lo) / (hi - lo) * Real(kBins);
  // Clamp before the integer conversion (out-of-range float-to-int is undefined).
  if (!(pos < Real(kBins))) return kBins - 1;  // also catches NaN
  if (pos < Real(1)) return 0;
  return static_cast<int>(pos);
}

struct McResult {
  uint64_t draws = 0;
  int n_years = 0;
  std::vector<uint64_t> hist;  // [kSeries][kMaxYears][kBins]
  std::vector<float> keep;     // [n_keep][kKeepFeatures]
  double ms = 0;
  std::string device;

  uint64_t* bins(int series, int t) { return hist.data() + (static_cast<size_t>(series) * kMaxYears + t) * kBins; }
  const uint64_t* bins(int series, int t) const {
    return hist.data() + (static_cast<size_t>(series) * kMaxYears + t) * kBins;
  }
  double quantile(const McInputs& in, int series, int t, double q) const;
  double mean(const McInputs& in, int series, int t) const;
};

// Fills hist_lo/hist_hi from a pilot run of `pilot` draws on the CPU.
void calibrate_hist_ranges(McInputs& in, const std::vector<float>& boot, uint64_t seed, int pilot = 8192);

McResult run_mc_cpu(const McInputs& in, const std::vector<float>& boot, uint64_t seed, uint64_t draws,
                    int n_keep = 0, bool single_precision = false);

bool cuda_available();
McResult run_mc_gpu(const McInputs& in, const std::vector<float>& boot, uint64_t seed, uint64_t draws,
                    int n_keep = 0);

}  // namespace e47
