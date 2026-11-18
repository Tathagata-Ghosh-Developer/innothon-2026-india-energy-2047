#pragma once
// End-to-end pipeline: data -> fitted models -> Monte Carlo inputs -> ensemble
// -> 2047 regional requirement -> generation strategy. Used by the command-line
// tool and the dashboard.
#include <array>
#include <map>
#include <string>
#include <vector>

#include "../allocation/allocate.hpp"
#include "../allocation/storage.hpp"
#include "../core/csv.hpp"  // kNaN
#include "../forecast/backtest.hpp"
#include "../forecast/india.hpp"
#include "../forecast/mc.hpp"
#include "../forecast/models.hpp"
#include "../forecast/panel.hpp"
#include "../regional/convergence.hpp"

namespace e47 {

// Default ensemble size and bootstrap replicates. The WebAssembly build runs the
// Monte Carlo without a GPU, so it starts smaller (the slider still goes higher).
#ifdef E47_WEB
constexpr int kDefaultDrawsLog2 = 16;
constexpr int kDefaultBootstrapReplicates = 300;
#else
constexpr int kDefaultDrawsLog2 = 22;
constexpr int kDefaultBootstrapReplicates = 2000;
#endif

// Forecast core.
//   kIndia (primary): NPP generation and the organiser state data only; urban
//     share and population paths are external drivers.
//   kCrossCountry: 153-country Ember/World Bank panel, an independent check used
//     for justification, not for estimation (organisers' ruling of 16 November 2026).
enum class CoreModel : int { kIndia = 0, kCrossCountry = 1 };
inline const char* core_name(CoreModel c) {
  return c == CoreModel::kIndia ? "India-only (NPP + organiser data)"
                                : "Cross-country panel (independent check used for justification, not for estimation)";
}

// Scenario settings exposed in the dashboard.
struct ScenarioControls {
  double g_start = -1;          // ln GDP per capita growth to 2030; < 0 = historical mean
  double g_start_shift = 0;     // added to g_start (used by the growth presets)
  double g_slowdown = 0.015;    // fall in annual growth between 2030 and 2047
  double g_level_sd = 0.009;    // persistent uncertainty in annual growth (+-1.5 points at 90%)
  double urban_shift_2047 = 0;  // added to the projected 2047 urban share (fraction)
  double urban_sd_2047 = 0.02;  // 1-sigma uncertainty of the 2047 urban share
  double pop_band_scale = 1.0;  // multiplier on the UN low-high band
  double addon_lo = 0, addon_hi = 0;  // new loads in 2047, TWh
  std::array<double, 3> w_model{-1, -1, -1};  // < 0 = backtest-based weights
  uint64_t draws = 1ull << kDefaultDrawsLog2;
  uint64_t seed = 20470815;
  bool use_gpu = true;
  double requirement_quantile = 0.5;  // which quantile of the 2047 requirement to plan for
  // India-only core: fall in the income elasticity per unit increase of ln(income)
  // (0 = constant elasticity, the estimated model).
  double elasticity_decline = 0;
  // India-only core: added to the income elasticity beta of every bootstrap replicate.
  double beta_shift = 0;
  // India-only core: which estimation window the replicates come from.
  // 0 = specification mixture (estimated model), 1 = S1 only (FY2011-12 onward),
  // 2 = S3 only (FY2006-07 onward, spliced price bases).
  int beta_window = 0;
};

struct HistoryPoint {
  int year;
  double gen_twh, pop, gdp_pc, urban;
};

struct DriverData {
  int base_year = 0;
  std::vector<int> years;                       // projection years
  std::vector<double> wpp_med, wpp_low, wpp_high;  // persons
  std::map<int, double> wpp_med_by_year;         // UN WPP 2024 medium, India, every year in the file (persons)
  std::vector<double> wup;                      // urban share, fraction (UN WUP 2018, national definition)
  double wup_base = 0;
  double g_hist_mean = 0, g_hist_sd = 0, g_hist_rho = 0;
  int g_hist_from = 0, g_hist_to = 0;
  // States of the regional split, from the challenge data only (see the constructor).
  // The challenge data have no state urban share, so the split has no state
  // urbanisation term; the national urban effect stays in the national total.
  std::vector<std::string> states;
  std::vector<int> state_region;
  std::vector<std::string> state_region_from;         // "NPP" or "dataset zone"
  std::vector<std::map<int, double>> state_pop_data;  // challenge dataset: year -> persons
  int state_pop_year = 0;          // last dataset year (2024): where the state paths start
  int state_growth_from = 0;       // start of the recent-growth window (2019)
  double state_growth_india = 0;   // dataset ln growth of India's population per year over that window
  std::vector<double> state_growth_offset;           // state's ln growth over the window minus India's
  std::vector<std::vector<double>> state_pop;        // [state][t] projected persons
  std::vector<std::vector<double>> state_pop_share;  // [state][t], sums to 1 every year
  std::vector<double> state_ln_rel_income;
  std::vector<std::string> region_notes;  // how states were mapped to grid regions
  // Published generation projections for comparison (CEA NEP 2022-32, gross generation, TWh).
  // x = fiscal-year midpoint (e.g. 2026-27 -> 2026.75).
  std::vector<std::pair<double, double>> nep_generation;
};

struct Fitted {
  CoreModel core = CoreModel::kIndia;
  std::array<std::string, 3> model_names;
  // India-only core
  IndiaSeries india;
  IndiaFit india_fit;            // primary estimation window
  IndiaFit india_fit_full;       // sensitivity: full window including the 2011 splice
  std::vector<double> india_beta_boot;   // specification mixture: S1 half, then S3 half
  std::vector<double> india_kappa_boot;  // kappa of the replicate's window
  std::vector<double> india_gamma;  // prior draws, one per replicate
  double gamma_mean = 0, gamma_sd = 0;
  std::string gamma_source;
  double anchor_shift = 0;  // retired (always 0): the base-year residual now decays through the AR(1) model error
  // Cross-country core
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

// Growth of the central path (central[0] of forecast(), no add-on) from the base
// year to target_year, split by driver. All parts are log points:
//   total        = ln(gen_target_twh / gen_base_twh)
//                = population + income + urbanisation + model_error
//   population   = ln(population in target_year / base-year population)
//   income       = beta dx - delta/2 dx^2 (dx: change of ln income per person; the
//                  elasticity decline delta belongs to income)
//   urbanisation = gamma_mean (du - kappa dx) (du: change of the urban share)
//   model_error  = E[residual in target_year] - base-year residual (AR(1) decay)
// India core only: the cross-country core fills total and population and leaves
// income, urbanisation and model_error NaN.
struct GrowthDecomposition {
  int base_year = 0, target_year = 0;
  double total = kNaN, population = kNaN, income = kNaN, urbanisation = kNaN, model_error = kNaN;
  double gen_base_twh = kNaN, gen_target_twh = kNaN;
};

struct StrategyResult {
  std::array<double, kRegions> requirement{};
  AllocationResult least_cost;
  AllocationResult score;
  PolicySettings policy;
  std::vector<TornadoRow> tornado;
  double tornado_ms = 0;
  // Storage and land back-calculated from the least-cost plan (hourly Merit shapes).
  StorageLandResult storage;
};

// Population growth check (organisers' ruling of 16 November 2026: external
// projections must be "reasonably consistent with the state-level population growth
// provided in the GitHub dataset"). Compound annual growth rates as fractions per
// year; NaN where a span is not available.
struct PopulationGrowth {
  std::string area;         // "India" or the state (normalised name)
  std::string region;       // grid region; empty for India
  std::string region_from;  // "NPP" or "dataset zone"; empty for India
  double pop_base = 0;      // challenge dataset, year `base` (India: sum over the states)
  double share_base = 0, share_target = 0;  // share of the states' total in `base` and `target`
  double data_long = kNaN, data_recent = kNaN;   // dataset: long_from..base, recent_from..base
  double model_near = kNaN, model_target = kNaN;  // model path: base..near, base..target
  double tg_near = kNaN;  // Technical Group projection base..near: a reference column only, not used
};

struct PopulationConsistency {
  int long_from = 0, recent_from = 0, base = 0, near = 0, target = 0;
  double un_long = kNaN, un_recent = kNaN, un_near = kNaN, un_target = kNaN;  // UN WPP 2024 medium, India
  std::vector<PopulationGrowth> rows;  // India first, then the states
};

class Pipeline {
 public:
  explicit Pipeline(std::string repo_root, CoreModel core = CoreModel::kIndia,
                    int bootstrap_replicates = kDefaultBootstrapReplicates);

  CoreModel core() const { return fit_.core; }
  const Fitted& fitted() const { return fit_; }
  // Bootstrap parameter array for the Monte Carlo (stride kBootStride) under scenario c.
  std::vector<float> boot_for(const ScenarioControls& c) const;
  const DriverData& drivers() const { return drv_; }
  const AllocationInputs& allocation_inputs() const { return alloc_in_; }
  const StorageInputs& storage_inputs() const { return storage_in_; }

  McInputs build_inputs(const ScenarioControls& c) const;
  ForecastResult forecast(const ScenarioControls& c) const;
  // Splits the central path's growth from the base year to target_year (a projection
  // year) by driver; uses the same driver paths and parameters as forecast(c).central[0].
  GrowthDecomposition decompose_central(const ScenarioControls& c, int target_year) const;
  StrategyResult strategy(const ForecastResult& f, const ScenarioControls& c, const PolicySettings& p) const;

  // Growth of the challenge dataset's populations against the model's population
  // paths and UN WPP 2024 (outputs/population_consistency.csv). India's model path is
  // the forecast's population input; a state's path is its projected population.
  // Reads the Technical Group projections for the reference column only.
  PopulationConsistency population_consistency() const;

 private:
  std::string root_;
  Fitted fit_;
  DriverData drv_;
  AllocationInputs alloc_in_;
  StorageInputs storage_in_;
};

}  // namespace e47
