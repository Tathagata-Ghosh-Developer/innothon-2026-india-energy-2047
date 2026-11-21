#pragma once
// India-only forecast core, built from the two organiser datasets:
//   * National Power Portal (NPP): all-India generation by fiscal year (GWh),
//   * Renewable India dataset: state population and real GSDP.
// Urban share (UN WUP 2018) and the population projections are external
// drivers, used the same way as the growth-rate assumptions the rules allow.
//
// Years are fiscal-year start years: 2023 means FY2023-24 (April 2023 to
// March 2024), and 2047 means FY2047-48.
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace e47 {

class GsdpPriceBasis;  // core/price_base.hpp

struct IndiaObs {
  int year = 0;          // fiscal-year start
  double gen_twh = 0;    // NPP total generation (utilities), TWh
  double thermal_twh = 0, hydro_twh = 0, nuclear_twh = 0, res_twh = 0;
  double pop = 0;        // persons (organiser data, chain-linked over matched states)
  double y = 0;          // real GSDP per person, thousand INR at 2011-12 prices (chain-linked)
  double urban = 0;      // urban share, fraction (UN WUP 2018, national definition)
  double ln_e() const { return std::log(gen_twh * 1e9 / pop); }  // ln kWh per person
};

struct IndiaSeries {
  std::vector<IndiaObs> obs;           // consecutive years
  double splice_growth = 0;            // ln growth of GSDP per person imputed for the 2011 base-year change
  std::vector<double> gpc_growth;      // ln growth of GSDP per person, year t over t-1 (obs[1..])
  int organiser_first = 0, organiser_last = 0;
  int stale_pairs_skipped = 0;         // state-year pairs whose GSDP was copied unchanged (not used for income growth)
  int gsdp_current_corrected = 0;      // dataset state-years at current prices deflated to 2011-12 prices (GsdpOptions::basis)
  int income_excluded_pairs = 0;       // state-year pairs left out of income growth (GsdpOptions::exclude_states)
};

// How load_india_series reads the challenge dataset's GSDP. The defaults read it as is.
//   basis: states whose GSDP is at current prices from a given year (gsdp_price_basis.csv);
//     their values are deflated to constant 2011-12 prices before any growth is taken. An
//     inflation correction of a mis-based series, not new estimation data.
//   exclude_states: states (any spelling normalize_state accepts) left out of national income
//     growth; their population still counts in population growth. A sensitivity only.
struct GsdpOptions {
  const GsdpPriceBasis* basis = nullptr;
  std::vector<std::string> exclude_states;
};

// npp_csv: data/raw/npp/npp_national_generation_fy.csv
// organiser_csv: data/raw/competition/renewable_energy_dataset.csv
// urban_csv: data/external/urbanization.csv
// Uses fiscal years first..last that have NPP generation and organiser data.
IndiaSeries load_india_series(const std::string& npp_csv, const std::string& organiser_csv,
                              const std::string& urban_csv, int first, int last, const GsdpOptions& gsdp = {});

// Income elasticity of generation per person, India only:
//   ln e_t = a + beta x_t + c D2020_t + eps_t,  x = ln(y / 10)
// D2020 marks FY2020-21 (pandemic). The urban share enters the projection only
// through its excess over the income-implied path,
//   ln e_t - ln e_0 = beta dx + gamma (du - kappa dx),
// where kappa is the slope of the urban share on x over the same window. In the
// sample u - u_hat(x) is orthogonal to x, so beta does not depend on gamma.
struct IndiaFit {
  int first = 0, last = 0, n = 0;
  double alpha = 0, beta = 0, beta_se = 0, covid = 0;
  double kappa = 0;     // d u / d x over the window (same regressors, including the dummy)
  double resid_sd = 0;  // regression residual standard deviation
  double dw = 0;        // Durbin-Watson statistic of the residuals
  double resid_rho = 0; // AR(1) persistence of the residuals, 1 - DW / 2
  double fd_beta = 0;   // first-difference estimate, FY2020-21 and FY2021-22 changes excluded
  std::vector<double> resid;
};

IndiaFit fit_india(const IndiaSeries& s, int first, int last);

// Moving-block bootstrap of the residuals (circular blocks of length `block`),
// returning `replicates` values of beta. OpenMP parallel over replicates.
std::vector<double> bootstrap_india_beta(const IndiaSeries& s, const IndiaFit& f, int replicates, int block,
                                         uint64_t seed);

// Projection of ln(kWh per person) change from the base observation, used by the
// backtest and the central paths.
inline double india_delta_ln_e(double beta, double gamma, double kappa, double dx, double du) {
  return beta * dx + gamma * (du - kappa * dx);
}

struct IndiaBacktestPoint {
  int year = 0;
  double actual_twh = 0, elasticity_twh = 0, elasticity_no_urban_twh = 0, damped_twh = 0;
};

struct IndiaBacktest {
  int origin = 0, fit_first = 0;
  double beta = 0;
  std::vector<IndiaBacktestPoint> points;
  double mape[3] = {0, 0, 0};
  double ape_last[3] = {0, 0, 0};
};

// Fits on fit_first..origin and predicts origin+1..last with the observed
// drivers (population, income, urban share), so only model error is tested.
IndiaBacktest run_india_backtest(const IndiaSeries& s, int fit_first, int origin, int last, double gamma);

}  // namespace e47
