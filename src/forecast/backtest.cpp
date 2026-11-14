#include "backtest.hpp"

#include <cmath>
#include <stdexcept>

#include "models.hpp"

namespace e47 {

BacktestResult run_backtest(const Panel& full, const std::string& iso3, int origin, int last_year) {
  const CountrySeries* target = full.find(iso3);
  if (!target) throw std::runtime_error("backtest: country not in panel: " + iso3);

  const Panel train = truncate_panel(full, origin, 8);
  const Design d = make_design(train);
  const FeQuad q = fit_fe_quad(d);
  const FeGompertz g = fit_fe_gompertz(d);

  const Obs* base = nullptr;
  std::vector<double> ln_hist;
  for (const auto& o : target->obs) {
    if (o.year <= origin) {
      base = &o;
      ln_hist.push_back(std::log(o.gen_twh));
    }
  }
  if (!base) throw std::runtime_error("backtest: no base year");
  const DampedTrend dt = fit_damped_trend(ln_hist);
  const DesignRow r0 = design_row(*base);

  BacktestResult res;
  res.origin = origin;
  int n = 0;
  for (const auto& o : target->obs) {
    if (o.year <= origin || o.year > last_year) continue;
    const DesignRow r1 = design_row(o);
    BacktestPoint p;
    p.year = o.year;
    p.actual_twh = o.gen_twh;
    const double pop_twh = o.pop / 1e9;  // TWh per (kWh per person)
    p.fe_quad_twh = pop_twh * std::exp(r0.ln_e + delta_ln_e(q, r0, r1));
    p.fe_gompertz_twh = pop_twh * std::exp(r0.ln_e + delta_ln_e(g, r0, r1));
    p.damped_twh = std::exp(damped_forecast(dt, o.year - base->year));
    const double pred[3] = {p.fe_quad_twh, p.fe_gompertz_twh, p.damped_twh};
    for (int m = 0; m < 3; ++m) {
      const double ape = std::abs(pred[m] - p.actual_twh) / p.actual_twh;
      res.mape[m] += ape;
      res.ape_last[m] = ape;
    }
    res.points.push_back(p);
    ++n;
  }
  if (n > 0)
    for (double& m : res.mape) m /= n;
  return res;
}

}  // namespace e47
