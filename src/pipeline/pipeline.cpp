#include "pipeline.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <map>
#include <stdexcept>

#include "../core/csv.hpp"
#include "../core/stats.hpp"

namespace e47 {

namespace fs = std::filesystem;

namespace {

constexpr int kTargetYear = 2047;

std::string find_wdi(const fs::path& dir) {
  for (const auto& e : fs::directory_iterator(dir)) {
    const std::string n = e.path().filename().string();
    if (n.rfind("API_", 0) == 0) return e.path().string();
  }
  throw std::runtime_error("no API_*.csv in " + dir.string());
}

// Linear interpolation over a sparse year -> value map.
double interp(const std::map<int, double>& m, int year) {
  if (m.empty()) throw std::runtime_error("interp on empty series");
  auto hi = m.lower_bound(year);
  if (hi != m.end() && hi->first == year) return hi->second;
  if (hi == m.begin()) return hi->second;
  if (hi == m.end()) return std::prev(hi)->second;
  auto lo = std::prev(hi);
  const double w = static_cast<double>(year - lo->first) / (hi->first - lo->first);
  return lo->second + w * (hi->second - lo->second);
}

}  // namespace

Pipeline::Pipeline(std::string root, int replicates) : root_(std::move(root)) {
  const fs::path r(root_);
  const fs::path wb = r / "data/raw/worldbank/downloads";
  PanelSources src;
  src.ember_generation_csv = (r / "data/raw/generation/downloads/ember/release_generation_yearly_global.csv").string();
  src.wb_gdp_pc_csv = find_wdi(wb / "NY.GDP.PCAP.PP.KD");
  src.wb_urban_csv = find_wdi(wb / "SP.URB.TOTL.IN.ZS");
  src.wb_population_csv = find_wdi(wb / "SP.POP.TOTL");

  // 1. Models
  fit_.panel = load_panel(src, PanelFilter{});
  fit_.design = make_design(fit_.panel);
  fit_.quad = fit_fe_quad(fit_.design);
  fit_.gomp = fit_fe_gompertz(fit_.design);
  fit_.quad_stats = fit_stats(fit_.design, fit_.quad);
  fit_.gomp_stats = fit_stats(fit_.design, fit_.gomp);
  Timer tb;
  fit_.quad_boot = bootstrap_fe_quad(fit_.design, replicates, 11);
  fit_.gomp_boot = bootstrap_fe_gompertz(fit_.design, replicates, 7);
  fit_.bootstrap_ms = tb.ms();
  fit_.boot.resize(static_cast<size_t>(replicates) * kBootStride);
  for (int k = 0; k < replicates; ++k) {
    float* p = fit_.boot.data() + static_cast<size_t>(k) * kBootStride;
    const FeQuad& q = fit_.quad_boot[k];
    const FeGompertz& g = fit_.gomp_boot[k];
    p[0] = static_cast<float>(q.b1);
    p[1] = static_cast<float>(q.b2);
    p[2] = static_cast<float>(q.g);
    p[3] = static_cast<float>(q.d);
    p[4] = static_cast<float>(g.a);
    p[5] = static_cast<float>(g.b);
    p[6] = static_cast<float>(g.c);
    p[7] = static_cast<float>(g.d);
  }

  const CountrySeries* india = fit_.panel.find("IND");
  if (!india) throw std::runtime_error("India missing from panel");
  std::vector<double> ln_gen;
  for (const auto& o : india->obs) {
    fit_.history.push_back({o.year, o.gen_twh, o.pop, o.gdp_pc, o.urban});
    ln_gen.push_back(std::log(o.gen_twh));
  }
  fit_.damped = fit_damped_trend(ln_gen);
  const int last = india->obs.back().year;
  for (int origin : {2005, 2010, 2015}) fit_.backtests.push_back(run_backtest(fit_.panel, "IND", origin, last));
  {
    const BacktestResult& b = fit_.backtests[1];  // 2010 origin: 15-year horizon
    double s = 0;
    for (int m = 0; m < 3; ++m) s += 1.0 / std::max(1e-3, b.mape[m]);
    for (int m = 0; m < 3; ++m) fit_.backtest_weights[m] = (1.0 / std::max(1e-3, b.mape[m])) / s;
  }
  fit_.convergence = estimate_convergence((r / "data/raw/competition/renewable_energy_dataset.csv").string(), 2006,
                                          2024, 4000, 99);

  // 2. National drivers
  drv_.base_year = last;
  for (int y = last + 1; y <= kTargetYear; ++y) drv_.years.push_back(y);
  {
    const CsvTable t = CsvTable::read((r / "data/external/population_projection.csv").string());
    std::map<int, double> med, low, high;
    for (size_t i = 0; i < t.rows(); ++i) {
      if (t.str(i, t.col("state")) != "India") continue;
      const std::string v = t.str(i, t.col("variant"));
      const int y = static_cast<int>(t.num(i, t.col("year")));
      const double p = t.num(i, t.col("population"));
      if (v == "UN WPP 2024 Medium") med[y] = p;
      if (v == "UN WPP 2024 Low") low[y] = p;
      if (v == "UN WPP 2024 High") high[y] = p;
    }
    for (int y : drv_.years) {
      drv_.wpp_med.push_back(interp(med, y));
      drv_.wpp_low.push_back(interp(low, y));
      drv_.wpp_high.push_back(interp(high, y));
    }
    drv_.wpp_med.insert(drv_.wpp_med.begin(), interp(med, last));  // index 0 = base year
  }
  {
    const CsvTable t = CsvTable::read((r / "data/external/urbanization.csv").string());
    std::map<int, double> wup;
    for (size_t i = 0; i < t.rows(); ++i) {
      if (t.str(i, t.col("state")) != "India") continue;
      if (t.str(i, t.col("source")).find("World Urbanization Prospects 2018") == std::string::npos) continue;
      wup[static_cast<int>(t.num(i, t.col("year")))] = t.num(i, t.col("value")) / 100.0;
    }
    drv_.wup_base = interp(wup, last);
    for (int y : drv_.years) drv_.wup.push_back(interp(wup, y));
  }
  {
    // GDP per capita growth statistics, India, 2005 to base year (World Bank PPP series).
    drv_.g_hist_from = 2005;
    drv_.g_hist_to = last;
    std::vector<double> g;
    for (size_t i = 1; i < india->obs.size(); ++i)
      if (india->obs[i].year > drv_.g_hist_from)
        g.push_back(std::log(india->obs[i].gdp_pc / india->obs[i - 1].gdp_pc));
    drv_.g_hist_mean = mean(g);
    drv_.g_hist_sd = stdev(g);
    double num = 0, den = 0;
    for (size_t i = 1; i < g.size(); ++i) {
      num += (g[i] - drv_.g_hist_mean) * (g[i - 1] - drv_.g_hist_mean);
      den += (g[i - 1] - drv_.g_hist_mean) * (g[i - 1] - drv_.g_hist_mean);
    }
    drv_.g_hist_rho = den > 0 ? num / den : 0.0;
  }

  // 3. States: Technical Group projections (1 July), grid regions, urban gap, relative income.
  {
    std::map<std::string, int> region_of;
    {
      const CsvTable t = CsvTable::read((r / "data/external/state_region_map.csv").string());
      const std::map<std::string, int> idx = {{"Northern", 0}, {"Western", 1}, {"Southern", 2},
                                              {"Eastern", 3},  {"North-Eastern", 4}};
      for (size_t i = 0; i < t.rows(); ++i) {
        auto it = idx.find(t.str(i, t.col("grid_region")));
        if (it != idx.end() && t.str(i, t.col("row_type")) != "composite")
          region_of[normalize_state(t.str(i, t.col("state")))] = it->second;
      }
    }
    const CsvTable t = CsvTable::read((r / "data/external/population_projection.csv").string());
    std::map<std::string, std::map<int, double>> pop, ush;
    for (size_t i = 0; i < t.rows(); ++i) {
      if (t.str(i, t.col("variant")) != "Technical Group projection") continue;
      const std::string date = t.str(i, t.col("reference_date"));
      if (date.size() < 10 || date.substr(5) != "07-01") continue;
      const std::string s = normalize_state(t.str(i, t.col("state")));
      const int y = static_cast<int>(t.num(i, t.col("year")));
      pop[s][y] = t.num(i, t.col("population"));
      ush[s][y] = t.num(i, t.col("urban_share"));
    }
    const double nat_urban_base = interp(ush.at("india"), last);
    std::map<std::string, double> rel;
    for (const auto& si : fit_.convergence.states) {
      auto it = si.ln_rel.find(2024);
      if (it != si.ln_rel.end()) rel[si.state] = it->second;
    }
    for (const auto& [s, series] : pop) {
      if (s == "india") continue;
      auto rg = region_of.find(s);
      if (rg == region_of.end()) continue;
      drv_.states.push_back(s);
      drv_.state_region.push_back(rg->second);
      drv_.state_urb_gap.push_back(interp(ush.at(s), last) - nat_urban_base);
      double lr = 0;
      if (rel.count(s))
        lr = rel[s];
      else if (s == "ladakh" && rel.count("jammu and kashmir"))
        lr = rel["jammu and kashmir"];
      drv_.state_ln_rel_income.push_back(lr);
    }
    if (drv_.states.size() > static_cast<size_t>(kMaxStates)) throw std::runtime_error("too many states");
    // Shares by year; beyond the last projected year, extend each state's 5-year log-share trend.
    const int tg_last = pop.at("india").rbegin()->first;
    const size_t ns = drv_.states.size();
    drv_.state_pop_share.assign(ns, std::vector<double>(drv_.years.size(), 0.0));
    for (size_t t_i = 0; t_i < drv_.years.size(); ++t_i) {
      const int y = drv_.years[t_i];
      double total = 0;
      std::vector<double> v(ns);
      for (size_t k = 0; k < ns; ++k) {
        const auto& ser = pop.at(drv_.states[k]);
        if (y <= tg_last) {
          v[k] = interp(ser, y);
        } else {
          const double a = interp(ser, tg_last - 5), b = interp(ser, tg_last);
          v[k] = b * std::pow(b / a, (y - tg_last) / 5.0);
        }
        total += v[k];
      }
      for (size_t k = 0; k < ns; ++k) drv_.state_pop_share[k][t_i] = v[k] / total;
    }
  }

  alloc_in_ = load_allocation_inputs(root_);
}

McInputs Pipeline::build_inputs(const ScenarioControls& c) const {
  McInputs in{};
  const HistoryPoint& h = fit_.history.back();
  const int n = static_cast<int>(drv_.years.size());
  if (n > kMaxYears) throw std::runtime_error("horizon too long");
  in.base_year = drv_.base_year;
  in.n_years = n;
  in.ln_e0 = static_cast<float>(std::log(h.gen_twh * 1e9 / h.pop));
  in.y0 = static_cast<float>(h.gdp_pc / 1000.0);
  in.u0 = static_cast<float>(h.urban);
  in.gen0_twh = static_cast<float>(h.gen_twh);

  const double g_start = c.g_start >= 0 ? c.g_start : drv_.g_hist_mean;
  for (int t = 0; t < n; ++t) {
    const int y = drv_.years[t];
    // Population anchored on the observed base-year level, growing as UN WPP 2024 medium.
    const double ratio = drv_.wpp_med[t + 1] / drv_.wpp_med[0];
    const double pop_med = h.pop * ratio / 1e6;
    const double band = (drv_.wpp_high[t] - drv_.wpp_low[t]) / drv_.wpp_med[t + 1];
    in.pop_med[t] = static_cast<float>(pop_med);
    in.pop_sd[t] = static_cast<float>(pop_med * band * c.pop_band_scale / (2.0 * 1.645));
    // Urban share anchored on the observed base year, moving as UN WUP 2018.
    const double frac = static_cast<double>(t + 1) / n;
    in.urb_med[t] = static_cast<float>(h.urban + (drv_.wup[t] - drv_.wup_base) + c.urban_shift_2047 * frac);
    in.urb_sd[t] = static_cast<float>(c.urban_sd_2047 * frac);
    // Growth: historical mean to 2030, then a linear slowdown to 2047.
    const double slow = y <= 2030 ? 0.0 : c.g_slowdown * (y - 2030) / (kTargetYear - 2030.0);
    in.g_mean[t] = static_cast<float>(g_start - slow);
  }
  in.g_level_sd = static_cast<float>(c.g_level_sd);
  in.g_shock_sd = static_cast<float>(drv_.g_hist_sd);
  in.g_rho = static_cast<float>(std::clamp(drv_.g_hist_rho, -0.9, 0.9));

  std::array<double, 3> w = fit_.backtest_weights;
  if (c.w_model[0] >= 0) w = c.w_model;
  const double ws = w[0] + w[1] + w[2];
  for (int m = 0; m < 3; ++m) in.w_model[m] = static_cast<float>(w[m] / ws);
  in.n_boot = static_cast<int>(fit_.quad_boot.size());

  const DampedTrend& d = fit_.damped;
  in.dt_level = static_cast<float>(d.level);
  in.dt_trend = static_cast<float>(d.trend);
  in.dt_phi = static_cast<float>(d.phi);
  in.dt_alpha = static_cast<float>(d.alpha);
  in.dt_beta = static_cast<float>(d.beta);
  in.dt_sigma = static_cast<float>(d.sigma);
  in.addon_lo = static_cast<float>(c.addon_lo);
  in.addon_hi = static_cast<float>(std::max(c.addon_lo, c.addon_hi));

  in.n_states = static_cast<int>(drv_.states.size());
  for (int s = 0; s < in.n_states; ++s) {
    in.state_region[s] = drv_.state_region[s];
    in.state_ln_rel_income[s] = static_cast<float>(drv_.state_ln_rel_income[s]);
    in.state_urb_gap[s] = static_cast<float>(drv_.state_urb_gap[s]);
    for (int t = 0; t < n; ++t) in.state_pop_share[s][t] = static_cast<float>(drv_.state_pop_share[s][t]);
  }
  in.conv_mean = static_cast<float>(fit_.convergence.lambda);
  in.conv_sd = static_cast<float>(fit_.convergence.lambda_se);
  return in;
}

ForecastResult Pipeline::forecast(const ScenarioControls& c) const {
  ForecastResult f;
  f.inputs = build_inputs(c);
  calibrate_hist_ranges(f.inputs, fit_.boot, c.seed);
  if (c.use_gpu && cuda_available())
    f.mc = run_mc_gpu(f.inputs, fit_.boot, c.seed, c.draws, 65536);
  else
    f.mc = run_mc_cpu(f.inputs, fit_.boot, c.seed, c.draws, 65536, true);
  f.years = drv_.years;
  const double qs[7] = {0.05, 0.10, 0.25, 0.50, 0.75, 0.90, 0.95};
  for (int s = 0; s < kSeries; ++s) {
    f.quant.q[s].resize(f.years.size());
    for (size_t t = 0; t < f.years.size(); ++t) {
      for (int k = 0; k < 7; ++k) f.quant.q[s][t][k] = f.mc.quantile(f.inputs, s, static_cast<int>(t), qs[k]);
      f.quant.q[s][t][7] = f.mc.mean(f.inputs, s, static_cast<int>(t));
    }
  }
  // Central paths: medium drivers, mean growth, full-sample parameters.
  const HistoryPoint& h = fit_.history.back();
  const DesignRow r0 = design_row(h.gen_twh * 1e9 / h.pop, h.gdp_pc, h.urban, h.year);
  double ln_y = std::log(h.gdp_pc);
  for (int m = 0; m < 3; ++m) f.central[m].resize(f.years.size());
  for (size_t t = 0; t < f.years.size(); ++t) {
    ln_y += f.inputs.g_mean[t];
    const double pop = f.inputs.pop_med[t] * 1e6;
    const DesignRow r1 = design_row(1.0, std::exp(ln_y), f.inputs.urb_med[t], f.years[t]);
    f.central[0][t] = pop / 1e9 * std::exp(r0.ln_e + delta_ln_e(fit_.quad, r0, r1));
    f.central[1][t] = pop / 1e9 * std::exp(r0.ln_e + delta_ln_e(fit_.gomp, r0, r1));
    f.central[2][t] = std::exp(damped_forecast(fit_.damped, static_cast<int>(t) + 1));
  }
  return f;
}

StrategyResult Pipeline::strategy(const ForecastResult& f, const ScenarioControls& c, const PolicySettings& p) const {
  StrategyResult s;
  s.policy = p;
  const int t = static_cast<int>(f.years.size()) - 1;
  // Regional quantiles do not add up to the national quantile, so plan for the
  // national quantile and split it by the regional medians.
  const double national = f.mc.quantile(f.inputs, 0, t, c.requirement_quantile);
  double sum = 0;
  for (int r = 0; r < kRegions; ++r) {
    s.requirement[r] = f.mc.quantile(f.inputs, 1 + r, t, 0.5);
    sum += s.requirement[r];
  }
  for (int r = 0; r < kRegions; ++r) s.requirement[r] *= national / sum;
  s.least_cost = solve_least_cost(alloc_in_, p, s.requirement);
  s.score = allocate_by_score(alloc_in_, p, s.requirement, s.least_cost.national);
  return s;
}

}  // namespace e47
