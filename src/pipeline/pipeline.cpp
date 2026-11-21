#include "pipeline.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <map>
#include <stdexcept>

#include "../core/csv.hpp"
#include "../core/philox.hpp"
#include "../core/stats.hpp"
#include "../regional/states.hpp"

namespace e47 {

namespace fs = std::filesystem;

namespace {

constexpr int kTargetYear = 2047;
constexpr int kRelIncomeYear = 2024;  // latest year of the organiser GSDP data
// State population paths: each state's growth relative to India's over the dataset's
// last five years (2019-2024), the window the organisers' consistency condition refers
// to. kPopLongFrom only adds the long 2011-2024 span to the consistency report, and
// kPopNear is the near-term check year of that report.
constexpr int kStateGrowthFrom = 2019;
constexpr int kPopLongFrom = 2011;
constexpr int kPopNear = 2030;

// India-only core: fiscal-year start years.
constexpr int kIndiaFirst = 2006;     // first year with annual NPP generation and organiser data
constexpr int kIndiaLast = 2023;      // FY2023-24: last year of the NPP all-India series
constexpr int kIndiaEstFirst = 2011;  // primary estimation window starts at the 2011-12 price base
constexpr int kIndiaBlock = 3;        // moving-block length for the residual bootstrap
constexpr int kBacktestOrigins[] = {2012, 2013, 2014, 2015, 2016, 2017, 2018};  // rolling origins

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

// One India-core parameter set in the Monte Carlo layout (stride kBootStride). Slot 0:
// beta with the excess-urbanisation term gamma; slot 1: gamma = 0. An elasticity
// decline delta keeps the base-year elasticity at beta:
//   elasticity(x) = beta - delta (x - x0)  =>  b2 = -delta / 2, b1 = beta + delta x0.
void pack_india(float* p, double beta, double gamma, double kappa, double delta, double x0) {
  p[0] = static_cast<float>(beta - gamma * kappa + delta * x0);
  p[1] = static_cast<float>(-0.5 * delta);
  p[2] = static_cast<float>(gamma);
  p[3] = 0.0f;
  p[4] = static_cast<float>(beta + delta * x0);
  p[5] = static_cast<float>(-0.5 * delta);
  p[6] = 0.0f;
  p[7] = 0.0f;
}

void growth_stats(const std::vector<double>& g, double& m, double& sd, double& rho) {
  m = mean(g);
  sd = stdev(g);
  double num = 0, den = 0;
  for (size_t i = 1; i < g.size(); ++i) {
    num += (g[i] - m) * (g[i - 1] - m);
    den += (g[i - 1] - m) * (g[i - 1] - m);
  }
  rho = den > 0 ? num / den : 0.0;
}

// Central path of the fitted core in every projection year: medium drivers, mean
// growth, full-sample parameters, no add-on. model 0 or 1 is the core's first or
// second model (India: with or without the excess-urbanisation term; cross-country:
// FE-Quad or FE-Gompertz). Pipeline::forecast and Pipeline::decompose_central both
// read the path from here, so the reported path and its decomposition cannot diverge.
//
// The path is scaled from the base-year population pop_base and generation
// twh_base = pop_base / 1e9 x exp(ln_e0), so in every year
//   ln(twh / twh_base) = ln(pop / pop_base) + income + urbanisation + model_error.
// The cross-country core leaves the last three parts NaN.
struct CentralPath {
  double pop_base = 0, twh_base = 0;
  std::vector<double> pop, twh;                           // [t] persons, TWh
  std::vector<double> income, urbanisation, model_error;  // [t] log points from the base year
};

CentralPath central_path(const Fitted& fit, const McInputs& in, const std::vector<int>& years,
                         const ScenarioControls& c, int model) {
  const HistoryPoint& h = fit.history.back();
  const size_t n = years.size();
  CentralPath p;
  p.pop_base = h.pop;
  p.pop.resize(n);
  p.twh.resize(n);
  p.income.assign(n, kNaN);
  p.urbanisation.assign(n, kNaN);
  p.model_error.assign(n, kNaN);
  for (size_t t = 0; t < n; ++t) p.pop[t] = in.pop_med[t] * 1e6;
  if (fit.core == CoreModel::kIndia) {
    const IndiaFit& fi = fit.india_fit;
    const double gamma = model == 0 ? fit.gamma_mean : 0.0;
    const double delta = std::max(0.0, c.elasticity_decline);
    const double e0 = std::exp(in.ln_e0);
    p.twh_base = p.pop_base / 1e9 * e0;
    const double x0 = std::log(h.gdp_pc / 10.0);
    double x = x0;
    for (size_t t = 0; t < n; ++t) {
      x += in.g_mean[t];
      const double dx = x - x0, du = in.urb_med[t] - h.urban;
      const double decline = -0.5 * delta * dx * dx;  // elasticity falling by delta per unit of x
      // The income and urban terms of india_delta_ln_e, one at a time; the decline is income.
      p.income[t] = india_delta_ln_e(fi.beta, 0.0, fi.kappa, dx, du) + decline;
      p.urbanisation[t] = india_delta_ln_e(0.0, gamma, fi.kappa, dx, du);
      // Expected path of the AR(1) residual relative to the base year.
      p.model_error[t] = fi.resid.back() * (std::pow(fi.resid_rho, static_cast<double>(t + 1)) - 1.0);
      p.twh[t] = p.pop[t] / 1e9 * e0 * std::exp(p.income[t] + p.urbanisation[t] + p.model_error[t]);
    }
    return p;
  }
  const DesignRow r0 = design_row(h.gen_twh * 1e9 / h.pop, h.gdp_pc, h.urban, h.year);
  p.twh_base = p.pop_base / 1e9 * std::exp(r0.ln_e);
  double ln_y = std::log(h.gdp_pc);
  for (size_t t = 0; t < n; ++t) {
    ln_y += in.g_mean[t];
    const DesignRow r1 = design_row(1.0, std::exp(ln_y), in.urb_med[t], years[t]);
    const double d = model == 0 ? delta_ln_e(fit.quad, r0, r1) : delta_ln_e(fit.gomp, r0, r1);
    p.twh[t] = p.pop[t] / 1e9 * std::exp(r0.ln_e + d);
  }
  return p;
}

}  // namespace

Pipeline::Pipeline(std::string root, CoreModel core, int replicates, PipelineOptions options)
    : root_(std::move(root)), options_(std::move(options)) {
  const fs::path r(root_);
  fit_.core = core;
  int last = 0;
  // Price indices (secondary) and the price basis of the challenge dataset's GSDP: states
  // whose GSDP is at current prices are deflated to constant 2011-12 prices wherever the
  // GSDP is read (an inflation correction of a mis-based series, not new estimation data).
  prices_ = PriceBase::load((r / "data/external/macro/price_indices.csv").string());
  gsdp_basis_ = GsdpPriceBasis::load((r / "data/external/macro/gsdp_price_basis.csv").string(), prices_);

  if (core == CoreModel::kCrossCountry) {
    // 1a. Cross-country panel models (secondary check).
    fit_.model_names = {"FE-Quad", "FE-Gompertz", "Damped trend"};
    const fs::path wb = r / "data/raw/worldbank/downloads";
    PanelSources src;
    src.ember_generation_csv = (r / "data/raw/generation/downloads/ember/release_generation_yearly_global.csv").string();
    src.wb_gdp_pc_csv = find_wdi(wb / "NY.GDP.PCAP.PP.KD");
    src.wb_urban_csv = find_wdi(wb / "SP.URB.TOTL.IN.ZS");
    src.wb_population_csv = find_wdi(wb / "SP.POP.TOTL");

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
    last = india->obs.back().year;
    for (int origin : {2005, 2010, 2015}) fit_.backtests.push_back(run_backtest(fit_.panel, "IND", origin, last));
    {
      const BacktestResult& b = fit_.backtests[1];  // 2010 origin: 15-year horizon
      double s = 0;
      for (int m = 0; m < 3; ++m) s += 1.0 / std::max(1e-3, b.mape[m]);
      for (int m = 0; m < 3; ++m) fit_.backtest_weights[m] = (1.0 / std::max(1e-3, b.mape[m])) / s;
    }
    // GDP per capita growth statistics, India, 2005 to base year (World Bank PPP series).
    drv_.g_hist_from = 2005;
    drv_.g_hist_to = last;
    std::vector<double> g;
    for (size_t i = 1; i < india->obs.size(); ++i)
      if (india->obs[i].year > drv_.g_hist_from)
        g.push_back(std::log(india->obs[i].gdp_pc / india->obs[i - 1].gdp_pc));
    growth_stats(g, drv_.g_hist_mean, drv_.g_hist_sd, drv_.g_hist_rho);
  } else {
    // 1b. India-only core (primary): NPP generation + organiser population and GSDP.
    fit_.model_names = {"India elasticity + urban prior", "India elasticity, no urban term",
                        "Damped trend (benchmark)"};
    GsdpOptions gsdp;
    gsdp.exclude_states = options_.income_exclude_states;
    fit_.india = load_india_series((r / "data/raw/npp/npp_national_generation_fy.csv").string(),
                                   (r / "data/raw/competition/renewable_energy_dataset.csv").string(),
                                   (r / "data/external/urbanization.csv").string(), kIndiaFirst, kIndiaLast,
                                   gsdp_basis_, gsdp);
    const IndiaSeries& s = fit_.india;
    last = s.obs.back().year;
    fit_.india_fit = fit_india(s, kIndiaEstFirst, last);
    fit_.india_fit_full = fit_india(s, s.obs.front().year, last);
    // The India bootstrap is tiny OLS solves: keep the full replicate count even in
    // the web build, so the browser and the command line give the same parameters.
    replicates = std::max(replicates, 2000);
    // Specification mixture, equal halves: S1 = FY2011-12 onward (one price base),
    // S3 = FY2006-07 onward (spliced price bases). Each half is a moving-block bootstrap.
    Timer tb;
    {
      const int h1 = replicates / 2, h3 = replicates - h1;
      const std::vector<double> b1 = bootstrap_india_beta(s, fit_.india_fit, h1, kIndiaBlock, 11);
      const std::vector<double> b3 = bootstrap_india_beta(s, fit_.india_fit_full, h3, kIndiaBlock, 13);
      fit_.india_beta_boot = b1;
      fit_.india_beta_boot.insert(fit_.india_beta_boot.end(), b3.begin(), b3.end());
      fit_.india_kappa_boot.assign(h1, fit_.india_fit.kappa);
      fit_.india_kappa_boot.insert(fit_.india_kappa_boot.end(), h3, fit_.india_fit_full.kappa);
    }
    fit_.bootstrap_ms = tb.ms();

    // Literature prior for the excess-urbanisation semi-elasticity gamma.
    {
      // File columns: parameter, distribution (uniform | normal), low, high, mean, sd, source.
      const fs::path pf = r / "data/external/literature_priors.csv";
      fit_.gamma_source = "no prior file: gamma = 0";
      bool uniform = false;
      double lo = 0, hi = 0;
      if (fs::exists(pf)) {
        const CsvTable t = CsvTable::read(pf.string());
        for (size_t i = 0; i < t.rows(); ++i) {
          if (t.str(i, t.col("parameter")) != "urban_excess_gamma") continue;
          uniform = t.str(i, t.col("distribution")) == "uniform";
          lo = t.num(i, t.col("low"));
          hi = t.num(i, t.col("high"));
          fit_.gamma_mean = uniform ? 0.5 * (lo + hi) : t.num(i, t.col("mean"));
          fit_.gamma_sd = uniform ? (hi - lo) / std::sqrt(12.0) : t.num(i, t.col("sd"));
          fit_.gamma_source = t.str(i, t.col("source"));
        }
      }
      fit_.india_gamma.resize(replicates);
      for (int k = 0; k < replicates; ++k) {
        const DrawRng rng(0x6A11u, static_cast<uint64_t>(k));
        if (uniform) {
          double v[4];
          rng.uniform4<double>(0, 1, v);
          fit_.india_gamma[k] = lo + (hi - lo) * v[0];
        } else {
          // Truncated normal: redraw until non-negative (not censored at zero).
          double g = -1.0;
          for (uint32_t step = 0; g < 0.0 && step < 64; ++step) {
            double z[4];
            rng.normal4<double>(step, 1, z);
            for (double zi : z)
              if (g < 0.0) g = fit_.gamma_mean + fit_.gamma_sd * zi;
          }
          fit_.india_gamma[k] = std::max(0.0, g);
        }
      }
    }

    std::vector<double> ln_gen;
    for (const auto& o : s.obs) {
      fit_.history.push_back({o.year, o.gen_twh, o.pop, o.y, o.urban});
      ln_gen.push_back(std::log(o.gen_twh));
    }
    fit_.damped = fit_damped_trend(ln_gen);
    for (int origin : kBacktestOrigins) {
      const IndiaBacktest ib = run_india_backtest(s, s.obs.front().year, origin, last, fit_.gamma_mean);
      BacktestResult b;
      b.origin = ib.origin;
      for (int m = 0; m < 3; ++m) {
        b.mape[m] = ib.mape[m];
        b.ape_last[m] = ib.ape_last[m];
      }
      for (const auto& p : ib.points) b.points.push_back({p.year, p.actual_twh, p.elasticity_twh, p.elasticity_no_urban_twh, p.damped_twh});
      fit_.backtests.push_back(b);
    }
    // The driver model carries the forecast; the no-urban variant is the ablation and
    // the damped trend is a benchmark without drivers (both weight 0 by default).
    fit_.backtest_weights = {1.0, 0.0, 0.0};

    // Income growth statistics from the same organiser series (2012..base, after the
    // splice), leaving out FY2020-21 and FY2021-22 (pandemic fall and rebound): one
    // pandemic in twelve years would otherwise double the annual shock variance.
    // Persistent uncertainty about the growth path is carried by g_level_sd.
    drv_.g_hist_from = kIndiaEstFirst;
    drv_.g_hist_to = last;
    std::vector<double> g_trend;
    double num = 0, den = 0, mean_tmp = 0;
    std::vector<std::pair<double, double>> pairs;  // consecutive retained years only
    for (size_t i = 0; i < s.gpc_growth.size(); ++i) {
      const int y = s.obs[i + 1].year;
      const bool keep = y > kIndiaEstFirst && y != 2020 && y != 2021;
      if (keep) g_trend.push_back(s.gpc_growth[i]);
      const bool keep_prev = y - 1 > kIndiaEstFirst && y - 1 != 2020 && y - 1 != 2021;
      if (keep && keep_prev && i > 0) pairs.push_back({s.gpc_growth[i - 1], s.gpc_growth[i]});
    }
    growth_stats(g_trend, drv_.g_hist_mean, drv_.g_hist_sd, mean_tmp);
    for (const auto& [a, b] : pairs) {
      num += (b - drv_.g_hist_mean) * (a - drv_.g_hist_mean);
      den += (a - drv_.g_hist_mean) * (a - drv_.g_hist_mean);
    }
    drv_.g_hist_rho = den > 0 ? num / den : 0.0;

    // Model error: the regression residual is persistent (Durbin-Watson about 1), so
    // the Monte Carlo carries an AR(1) residual that starts at the base year's
    // residual and reverts to the fitted relation at the estimated speed.
    fit_.anchor_shift = 0.0;

    // Scope is NPP utility generation. Captive generation is outside NPP's series and,
    // under the organisers' ruling of 16 November 2026 ("Use energy production ... from
    // the NPP dataset. Do not use any external data for estimation"), it is neither
    // estimated nor reported.
    fit_.boot = boot_for(ScenarioControls{});
  }
  fit_.convergence = estimate_convergence((r / "data/raw/competition/renewable_energy_dataset.csv").string(), 2006,
                                          2024, 4000, 99, gsdp_basis_);

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
    drv_.wpp_med_by_year = med;  // India-level reference growth for the state paths (section 3)
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
    // NEP years are fiscal years ("2031-32"): the India core labels them by start
    // year; the cross-country core places them at the fiscal-year midpoint.
    const double offset = core == CoreModel::kIndia ? 0.0 : 0.75;
    const CsvTable t = CsvTable::read((r / "data/external/techno_economic/policy_targets.csv").string());
    for (size_t i = 0; i < t.rows(); ++i) {
      if (t.str(i, t.col("target")) != "Projected gross generation, Total (CEA NEP 2022-32)") continue;
      const std::string fy = t.str(i, t.col("target_year"));  // "2031-32"
      if (fy.size() < 4) continue;
      drv_.nep_generation.push_back({std::stoi(fy.substr(0, 4)) + offset, t.num(i, t.col("value"))});
    }
  }

  // 3. States of the regional split, from the challenge data only (organisers' ruling
  //    of 16 November 2026): population from the challenge dataset, grid region from
  //    NPP (the dataset Zone where NPP does not list the state), relative income from
  //    the organiser GSDP. The challenge data have no state urban share, so the split
  //    has no state urbanisation term.
  //
  //    Population path of state s: its own recent growth relative to the dataset's
  //    national growth, on top of UN WPP 2024 medium growth for India,
  //      g_s(y) = g_UN(y) + (g_s,2019-24 - g_India,2019-24),
  //    starting from the dataset's 2024 population, so
  //      P_s(y) = P_s(2024) x WPP(y) / WPP(2024) x exp(offset_s (y - 2024)).
  //    Years align as in the India core: dataset year t is fiscal year t, so the
  //    dataset's 2024 is FY2024-25, the first projection year after the FY2023-24
  //    base. Only the shares enter the Monte Carlo (renormalised every year); the
  //    national population path of the forecast is not changed.
  {
    const StatePopulation sp =
        load_state_population((r / "data/raw/competition/renewable_energy_dataset.csv").string(),
                              (r / "data/raw/npp/npp_state_generation_fy.csv").string());
    drv_.region_notes = sp.notes;
    drv_.state_pop_year = sp.last_year;
    drv_.state_growth_from = kStateGrowthFrom;
    drv_.state_growth_india = state_population_growth(sp, "", kStateGrowthFrom, sp.last_year);
    if (!std::isfinite(drv_.state_growth_india))
      throw std::runtime_error("challenge dataset: no national population growth " + std::to_string(kStateGrowthFrom) +
                               "-" + std::to_string(sp.last_year));
    std::map<std::string, double> rel;
    for (const auto& si : fit_.convergence.states) {
      auto it = si.ln_rel.find(kRelIncomeYear);
      if (it != si.ln_rel.end()) rel[si.state] = it->second;
    }
    for (const StateRecord& rec : sp.states) {
      drv_.states.push_back(rec.state);
      drv_.state_region.push_back(rec.region);
      drv_.state_region_from.push_back(rec.region_from);
      drv_.state_pop_data.push_back(rec.pop);
      double offset = state_population_growth(sp, rec.state, kStateGrowthFrom, sp.last_year) - drv_.state_growth_india;
      if (!std::isfinite(offset)) {
        drv_.region_notes.push_back(rec.state + ": no population for " + std::to_string(kStateGrowthFrom) +
                                    "; grows at the national rate");
        offset = 0;
      }
      drv_.state_growth_offset.push_back(offset);
      auto it = rel.find(rec.state);
      if (it == rel.end())
        drv_.region_notes.push_back(rec.state + ": no GSDP for " + std::to_string(kRelIncomeYear) +
                                    "; national income per person assumed");
      drv_.state_ln_rel_income.push_back(it == rel.end() ? 0.0 : it->second);
    }
    if (drv_.states.size() > static_cast<size_t>(kMaxStates)) throw std::runtime_error("too many states");
    const size_t ns = drv_.states.size();
    const double un_start = interp(drv_.wpp_med_by_year, sp.last_year);
    drv_.state_pop.assign(ns, std::vector<double>(drv_.years.size(), 0.0));
    drv_.state_pop_share.assign(ns, std::vector<double>(drv_.years.size(), 0.0));
    for (size_t t_i = 0; t_i < drv_.years.size(); ++t_i) {
      const int y = drv_.years[t_i];
      const double un_ratio = interp(drv_.wpp_med_by_year, y) / un_start;
      double total = 0;
      for (size_t k = 0; k < ns; ++k) {
        const double v = drv_.state_pop_data[k].at(sp.last_year) * un_ratio *
                         std::exp(drv_.state_growth_offset[k] * (y - sp.last_year));
        drv_.state_pop[k][t_i] = v;
        total += v;
      }
      for (size_t k = 0; k < ns; ++k) drv_.state_pop_share[k][t_i] = drv_.state_pop[k][t_i] / total;
    }
  }

  alloc_in_ = load_allocation_inputs(root_);
  storage_in_ = load_storage_inputs(root_);  // never throws; storage is skipped if its inputs are missing
}

std::vector<float> Pipeline::boot_for(const ScenarioControls& c) const {
  if (fit_.core == CoreModel::kCrossCountry) return fit_.boot;
  // India-only core, one replicate per slot of the array (pack_india: slot 0 with the
  // excess-urbanisation prior, slot 1 with gamma = 0, optional elasticity decline).
  // beta_shift is added to beta in every replicate. beta_window 1 or 2 fills every slot
  // with that window's half of the mixture (S1 first, then S3), each replicate of the
  // half repeated equally, so the array keeps its size and the draw count is unchanged.
  const size_t n = fit_.india_beta_boot.size();
  const size_t h1 = n / 2;  // S1 half; matches the split in the constructor
  size_t from = 0, len = n;
  if (c.beta_window == 1) len = h1;
  if (c.beta_window == 2) from = h1, len = n - h1;
  const double x0 = std::log(fit_.history.back().gdp_pc / 10.0);
  const double delta = std::max(0.0, c.elasticity_decline);
  std::vector<float> out(n * kBootStride);
  for (size_t k = 0; k < n; ++k) {
    const size_t j = from + k * len / n;
    pack_india(out.data() + k * kBootStride, fit_.india_beta_boot[j] + c.beta_shift, fit_.india_gamma[j],
               fit_.india_kappa_boot[j], delta, x0);
  }
  return out;
}

McInputs Pipeline::build_inputs(const ScenarioControls& c) const {
  McInputs in{};
  const bool india = fit_.core == CoreModel::kIndia;
  const HistoryPoint& h = fit_.history.back();
  const int n = static_cast<int>(drv_.years.size());
  if (n > kMaxYears) throw std::runtime_error("horizon too long");
  in.base_year = drv_.base_year;
  in.n_years = n;
  in.ln_e0 = static_cast<float>(std::log(h.gen_twh * 1e9 / h.pop));
  if (india) {
    const IndiaFit& fi = fit_.india_fit;
    in.resid0 = static_cast<float>(fi.resid.back());
    in.resid_rho = static_cast<float>(fi.resid_rho);
    in.resid_innov_sd = static_cast<float>(fi.resid_sd * std::sqrt(1.0 - fi.resid_rho * fi.resid_rho));
  }
  // Income per person in thousands: INR (India core) or PPP $ (cross-country core).
  in.y0 = static_cast<float>(india ? h.gdp_pc : h.gdp_pc / 1000.0);
  in.u0 = static_cast<float>(h.urban);
  in.gen0_twh = static_cast<float>(h.gen_twh);
  in.quad_only = india ? 1 : 0;

  const double g_start = (c.g_start >= 0 ? c.g_start : drv_.g_hist_mean) + c.g_start_shift;
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
  in.n_boot = static_cast<int>(fit_.boot.size() / kBootStride);

  const DampedTrend& d = fit_.damped;
  in.dt_level = static_cast<float>(d.level);
  in.dt_trend = static_cast<float>(d.trend);
  in.dt_phi = static_cast<float>(d.phi);
  in.dt_alpha = static_cast<float>(d.alpha);
  in.dt_beta = static_cast<float>(d.beta);
  in.dt_sigma = static_cast<float>(d.sigma);
  in.addon_lo = static_cast<float>(c.addon_lo);
  in.addon_hi = static_cast<float>(std::max(c.addon_lo, c.addon_hi));
  pack_features(in, c.feature_draws);

  in.n_states = static_cast<int>(drv_.states.size());
  for (int s = 0; s < in.n_states; ++s) {
    in.state_region[s] = drv_.state_region[s];
    in.state_ln_rel_income[s] = static_cast<float>(drv_.state_ln_rel_income[s]);
    for (int t = 0; t < n; ++t) in.state_pop_share[s][t] = static_cast<float>(drv_.state_pop_share[s][t]);
  }
  in.conv_mean = static_cast<float>(fit_.convergence.lambda);
  in.conv_sd = static_cast<float>(fit_.convergence.lambda_se);
  in.rel_income_lag = static_cast<float>(drv_.base_year - kRelIncomeYear);
  return in;
}

ForecastResult Pipeline::forecast(const ScenarioControls& c) const {
  ForecastResult f;
  f.inputs = build_inputs(c);
  const std::vector<float> boot = boot_for(c);
  calibrate_hist_ranges(f.inputs, boot, c.seed);
  if (c.use_gpu && cuda_available())
    f.mc = run_mc_gpu(f.inputs, boot, c.seed, c.draws, 65536);
  else
    f.mc = run_mc_cpu(f.inputs, boot, c.seed, c.draws, 65536, true);
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
  for (int m = 0; m < 2; ++m) f.central[m] = central_path(fit_, f.inputs, f.years, c, m).twh;
  f.central[2].resize(f.years.size());
  for (size_t t = 0; t < f.years.size(); ++t)
    f.central[2][t] = std::exp(damped_forecast(fit_.damped, static_cast<int>(t) + 1));
  return f;
}

GrowthDecomposition Pipeline::decompose_central(const ScenarioControls& c, int target_year) const {
  const auto it = std::find(drv_.years.begin(), drv_.years.end(), target_year);
  if (it == drv_.years.end())
    throw std::invalid_argument("decompose_central: " + std::to_string(target_year) + " is not a projection year");
  const size_t t = static_cast<size_t>(it - drv_.years.begin());
  // build_inputs gives the same driver paths as forecast(c); the Monte Carlo is not needed.
  const CentralPath p = central_path(fit_, build_inputs(c), drv_.years, c, 0);
  GrowthDecomposition d;
  d.base_year = drv_.base_year;
  d.target_year = target_year;
  d.gen_base_twh = p.twh_base;
  d.gen_target_twh = p.twh[t];
  d.total = std::log(p.twh[t] / p.twh_base);
  d.population = std::log(p.pop[t] / p.pop_base);
  d.income = p.income[t];
  d.urbanisation = p.urbanisation[t];
  d.model_error = p.model_error[t];
  return d;
}

std::vector<StateRow> Pipeline::state_table(const ScenarioControls& c, int target_year) const {
  const auto it = std::find(drv_.years.begin(), drv_.years.end(), target_year);
  if (it == drv_.years.end())
    throw std::invalid_argument("state_table: " + std::to_string(target_year) + " is not a projection year");
  const int t = static_cast<int>(it - drv_.years.begin());
  const McInputs in = build_inputs(c);  // same driver paths, weights and state inputs as forecast(c)

  // Model with the largest weight; the damped trend has no income response, so the
  // kernel splits its draws with slot 0, and so does the table.
  int model = kFeQuad;
  for (int m = 1; m < 3; ++m)
    if (in.w_model[m] > in.w_model[model]) model = m;
  if (model == kDamped) model = kFeQuad;

  // Point estimates in the Monte Carlo parameter layout.
  float p[kBootStride] = {};
  if (fit_.core == CoreModel::kIndia) {
    const IndiaFit& fi = c.beta_window == 2 ? fit_.india_fit_full : fit_.india_fit;
    pack_india(p, fi.beta + c.beta_shift, fit_.gamma_mean, fi.kappa, std::max(0.0, c.elasticity_decline),
               std::log(fit_.history.back().gdp_pc / 10.0));
  } else {
    const double v[kBootStride] = {fit_.quad.b1, fit_.quad.b2, fit_.quad.g, fit_.quad.d,
                                   fit_.gomp.a,  fit_.gomp.b,  fit_.gomp.c, fit_.gomp.d};
    for (int k = 0; k < kBootStride; ++k) p[k] = static_cast<float>(v[k]);
  }

  // Central income path (mean growth, no shocks) and the kernel's convergence decay;
  // t + 1 + rel_income_lag = target_year - kRelIncomeYear.
  double ln_y = std::log(static_cast<double>(in.y0));
  for (int k = 0; k <= t; ++k) ln_y += in.g_mean[k];
  const double y = std::exp(ln_y);
  const double decay = std::exp(-fit_.convergence.lambda * (t + 1 + static_cast<double>(in.rel_income_lag)));
  const double index_national = state_log_index<double>(model, p, y, in.quad_only);

  const size_t ns = drv_.states.size();
  double pop_total = 0;
  for (size_t k = 0; k < ns; ++k) pop_total += drv_.state_pop_data[k].at(drv_.state_pop_year);
  std::vector<StateRow> rows(ns);
  double total = 0;
  for (size_t k = 0; k < ns; ++k) {
    StateRow& r = rows[k];
    r.state = drv_.states[k];
    r.region = region_name(drv_.state_region[k]);
    r.region_from = drv_.state_region_from[k];
    r.pop_share_base = drv_.state_pop_data[k].at(drv_.state_pop_year) / pop_total;
    r.ln_rel_income_base = drv_.state_ln_rel_income[k];
    r.ln_rel_income_target = r.ln_rel_income_base * decay;
    const double ys = y * std::exp(r.ln_rel_income_target);
    r.per_person_index_target = std::exp(state_log_index<double>(model, p, ys, in.quad_only) - index_national);
    r.requirement_share_target = drv_.state_pop_share[k][static_cast<size_t>(t)] * r.per_person_index_target;
    total += r.requirement_share_target;
  }
  for (StateRow& r : rows) r.requirement_share_target /= total;
  return rows;
}

PopulationConsistency Pipeline::population_consistency() const {
  PopulationConsistency pc;
  pc.long_from = kPopLongFrom;
  pc.recent_from = drv_.state_growth_from;
  pc.base = drv_.state_pop_year;
  pc.near = kPopNear;
  pc.target = kTargetYear;
  // Compound annual growth rate between two levels; NaN when either is missing.
  const auto cagr = [](double a, double b, int years) {
    return (a > 0 && b > 0 && years > 0) ? std::pow(b / a, 1.0 / years) - 1.0 : kNaN;
  };
  const auto& un = drv_.wpp_med_by_year;
  const auto un_cagr = [&](int from, int to) { return cagr(interp(un, from), interp(un, to), to - from); };
  pc.un_long = un_cagr(pc.long_from, pc.base);
  pc.un_recent = un_cagr(pc.recent_from, pc.base);
  pc.un_near = un_cagr(pc.base, pc.near);
  pc.un_target = un_cagr(pc.base, pc.target);

  // Technical Group projections (1 July), reference column only: never used by the model.
  std::map<std::string, std::map<int, double>> tg;
  {
    const CsvTable t = CsvTable::read((fs::path(root_) / "data/external/population_projection.csv").string());
    for (size_t i = 0; i < t.rows(); ++i) {
      if (t.str(i, t.col("variant")) != "Technical Group projection") continue;
      const std::string date = t.str(i, t.col("reference_date"));
      if (date.size() < 10 || date.substr(5) != "07-01") continue;
      tg[normalize_state(t.str(i, t.col("state")))][static_cast<int>(t.num(i, t.col("year")))] =
          t.num(i, t.col("population"));
    }
  }
  const auto tg_cagr = [&](const std::string& s) {
    auto it = tg.find(s);
    if (it == tg.end() || !it->second.count(pc.base) || !it->second.count(pc.near)) return kNaN;
    return cagr(it->second.at(pc.base), it->second.at(pc.near), pc.near - pc.base);
  };
  // Index of a projection year in drv_.years; -1 if it is not projected.
  const auto year_index = [&](int y) {
    auto it = std::find(drv_.years.begin(), drv_.years.end(), y);
    return it == drv_.years.end() ? -1 : static_cast<int>(it - drv_.years.begin());
  };
  const int i_base = year_index(pc.base), i_near = year_index(pc.near), i_target = year_index(pc.target);
  const auto level = [](const std::vector<double>& path, int i) { return i >= 0 ? path[i] : kNaN; };
  const auto data_cagr = [&](const std::map<int, double>& p, int from, int to) {
    auto a = p.find(from), b = p.find(to);
    return (a == p.end() || b == p.end()) ? kNaN : cagr(a->second, b->second, to - from);
  };

  // India: the dataset over the states reported in both years; the model path is the
  // forecast's population input (base-year dataset level x UN WPP 2024 medium ratio).
  {
    PopulationGrowth g;
    g.area = "India";
    double long0 = 0, long1 = 0, recent0 = 0, recent1 = 0;  // sums over the states reported in both years
    for (const auto& p : drv_.state_pop_data) {
      g.pop_base += p.at(pc.base);
      if (p.count(pc.long_from)) long0 += p.at(pc.long_from), long1 += p.at(pc.base);
      if (p.count(pc.recent_from)) recent0 += p.at(pc.recent_from), recent1 += p.at(pc.base);
    }
    g.share_base = g.share_target = 1.0;
    g.data_long = cagr(long0, long1, pc.base - pc.long_from);
    g.data_recent = cagr(recent0, recent1, pc.base - pc.recent_from);
    const double h_pop = fit_.history.back().pop;
    std::vector<double> national(drv_.years.size());
    for (size_t t = 0; t < drv_.years.size(); ++t) national[t] = h_pop * drv_.wpp_med[t + 1] / drv_.wpp_med[0];
    g.model_near = cagr(level(national, i_base), level(national, i_near), pc.near - pc.base);
    g.model_target = cagr(level(national, i_base), level(national, i_target), pc.target - pc.base);
    g.tg_near = tg_cagr("india");
    pc.rows.push_back(g);
  }
  // States: the dataset's own series and the projected paths of the regional split.
  const double total_base = pc.rows.front().pop_base;
  for (size_t k = 0; k < drv_.states.size(); ++k) {
    PopulationGrowth g;
    g.area = drv_.states[k];
    g.region = region_name(drv_.state_region[k]);
    g.region_from = drv_.state_region_from[k];
    g.pop_base = drv_.state_pop_data[k].at(pc.base);
    g.share_base = g.pop_base / total_base;
    g.share_target = level(drv_.state_pop_share[k], i_target);
    g.data_long = data_cagr(drv_.state_pop_data[k], pc.long_from, pc.base);
    g.data_recent = data_cagr(drv_.state_pop_data[k], pc.recent_from, pc.base);
    g.model_near = cagr(level(drv_.state_pop[k], i_base), level(drv_.state_pop[k], i_near), pc.near - pc.base);
    g.model_target = cagr(level(drv_.state_pop[k], i_base), level(drv_.state_pop[k], i_target), pc.target - pc.base);
    g.tg_near = tg_cagr(drv_.states[k]);
    pc.rows.push_back(g);
  }
  return pc;
}

void apply_feature_scenario(ScenarioControls& c, const std::vector<DemandFeature>& features, Scenario s) {
  // Computed first, so a scenario that throws leaves c as it was.
  std::vector<FeatureDraw> draws;
  double fixed = 0;
  if (s == Scenario::kJoint)
    draws = joint_distribution(features);
  else if (s != Scenario::kCore)
    fixed = scenario_total(features, s).total;
  c.addon_lo = c.addon_hi = fixed;
  c.feature_draws = std::move(draws);
}

std::vector<DemandFeature> Pipeline::demand_features() const {
  return read_demand_features((fs::path(root_) / kDemandFeaturesFile).string());
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
  if (s.least_cost.ok) {
    s.score = allocate_by_score(alloc_in_, p, s.requirement, s.least_cost.national);
  } else {
    s.score.ok = false;
    s.score.status = "not run: the least-cost plan is infeasible, so there are no national totals to share";
  }
  s.storage = storage_and_land(storage_in_, alloc_in_, p, s.least_cost);
  Timer tt;
  const double p10 = f.mc.quantile(f.inputs, 0, t, 0.10), p90 = f.mc.quantile(f.inputs, 0, t, 0.90);
  s.tornado = policy_tornado(alloc_in_, p, s.requirement, p10 / national, p90 / national);
  s.tornado_ms = tt.ms();
  return s;
}

}  // namespace e47
