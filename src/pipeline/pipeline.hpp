#pragma once
// End-to-end pipeline: data -> fitted models -> Monte Carlo inputs -> ensemble
// -> 2047 regional requirement -> generation strategy. Used by the command-line
// tool and the dashboard.
#include <array>
#include <string>
#include <vector>

#include "../allocation/allocate.hpp"
#include "../forecast/backtest.hpp"
#include "../forecast/mc.hpp"
#include "../forecast/models.hpp"
#include "../forecast/panel.hpp"
#include "../regional/convergence.hpp"

namespace e47 {

// Scenario settings exposed in the dashboard.
struct ScenarioControls {
  double g_start = -1;          // ln GDP per capita growth in 2026-2030; < 0 = historical mean
  double g_slowdown = 0.015;    // fall in annual growth between 2030 and 2047
  double g_level_sd = 0.0075;   // persistent uncertainty in annual growth
  double urban_shift_2047 = 0;  // added to the projected 2047 urban share (fraction)
  double urban_sd_2047 = 0.02;  // 1-sigma uncertainty of the 2047 urban share
  double pop_band_scale = 1.0;  // multiplier on the UN low-high band
  double addon_lo = 0, addon_hi = 0;  // new loads in 2047, TWh
  std::array<double, 3> w_model{-1, -1, -1};  // < 0 = backtest-based weights
  uint64_t draws = 1u << 22;
  uint64_t seed = 20470815;
  bool use_gpu = true;
  double requirement_quantile = 0.5;  // which quantile of the 2047 requirement to plan for
};

struct HistoryPoint {
  int year;
  double gen_twh, pop, gdp_pc, urban;
};

struct DriverData {
  int base_year = 0;
  std::vector<int> years;                       // projection years
  std::vector<double> wpp_med, wpp_low, wpp_high;  // persons
  std::vector<double> wup;                      // urban share, fraction (UN WUP 2018, national definition)
  double wup_base = 0;
  double g_hist_mean = 0, g_hist_sd = 0, g_hist_rho = 0;
  int g_hist_from = 0, g_hist_to = 0;
  // States
  std::vector<std::string> states;
  std::vector<int> state_region;
  std::vector<std::vector<double>> state_pop_share;  // [state][t]
  std::vector<double> state_ln_rel_income;
  std::vector<std::vector<double>> state_urb_delta;  // [state][t], change since base year
  // Published generation projections for comparison (CEA NEP 2022-32, gross generation, TWh).
  // x = fiscal-year midpoint (e.g. 2026-27 -> 2026.75).
  std::vector<std::pair<double, double>> nep_generation;
};

struct Fitted {
  Panel panel;
  Design design;
  FeQuad quad;
  FeGompertz gomp;
  DampedTrend damped;
  FitStats quad_stats, gomp_stats;
  std::vector<FeQuad> quad_boot;
  std::vector<FeGompertz> gomp_boot;
  std::vector<float> boot;  // packed for the Monte Carlo
  std::vector<BacktestResult> backtests;
  ConvergenceResult convergence;
  std::vector<HistoryPoint> history;  // India
  std::array<double, 3> backtest_weights{};
  double bootstrap_ms = 0;
};

struct Quantiles {
  // [series][t] for p05 p10 p25 p50 p75 p90 p95 and mean
  std::vector<std::array<double, 8>> q[kSeries];
};

struct ForecastResult {
  McInputs inputs{};
  McResult mc;
  Quantiles quant;
  std::vector<int> years;
  // Central paths (medium drivers, mean growth, full-sample parameters), national TWh
  std::vector<double> central[3];
};

struct StrategyResult {
  std::array<double, kRegions> requirement{};
  AllocationResult least_cost;
  AllocationResult score;
  PolicySettings policy;
  std::vector<TornadoRow> tornado;
  double tornado_ms = 0;
};

class Pipeline {
 public:
  explicit Pipeline(std::string repo_root, int bootstrap_replicates = 2000);

  const Fitted& fitted() const { return fit_; }
  const DriverData& drivers() const { return drv_; }
  const AllocationInputs& allocation_inputs() const { return alloc_in_; }

  McInputs build_inputs(const ScenarioControls& c) const;
  ForecastResult forecast(const ScenarioControls& c) const;
  StrategyResult strategy(const ForecastResult& f, const ScenarioControls& c, const PolicySettings& p) const;

 private:
  std::string root_;
  Fitted fit_;
  DriverData drv_;
  AllocationInputs alloc_in_;
};

}  // namespace e47
