#pragma once
// Out-of-sample backtest: fit every model on data up to an origin year, then
// predict India's total generation for later years using the observed
// population, GDP per capita and urban share (perfect-foresight drivers), so
// the test isolates model error from driver-projection error.
#include <string>
#include <vector>

#include "panel.hpp"

namespace e47 {

struct BacktestPoint {
  int year = 0;
  double actual_twh = 0;
  double fe_quad_twh = 0;
  double fe_gompertz_twh = 0;
  double damped_twh = 0;
};

struct BacktestResult {
  int origin = 0;
  std::vector<BacktestPoint> points;
  double mape[3] = {0, 0, 0};      // FE-Quad, FE-Gompertz, damped trend
  double ape_last[3] = {0, 0, 0};  // absolute percentage error in the last year
};

BacktestResult run_backtest(const Panel& full, const std::string& iso3, int origin_year, int last_year);

}  // namespace e47
