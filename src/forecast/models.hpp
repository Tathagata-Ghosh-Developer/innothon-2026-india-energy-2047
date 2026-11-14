#pragma once
// Forecast models for electricity generation per capita.
//
// Model 1 (FE-Quad): ln e_it = alpha_i + b1 x + b2 x^2 + g u + d tau
// Model 2 (FE-Gompertz): ln e_it = alpha_i - a exp(-b y) + c u + d tau
// Model 3 (Damped trend): additive damped trend on India's ln(total generation)
//
// e = kWh generated per person, x = ln(GDP per capita / 10,000),
// y = GDP per capita in thousand $, u = urban share, tau = (year - 2000) / 10.
// alpha_i is a country fixed effect, removed by within-country demeaning.
// Every projection is anchored on India's last observed generation per capita,
// so the fixed effect never needs to be estimated for the forecast.
#include <cstdint>
#include <string>
#include <vector>

#include "panel.hpp"

namespace e47 {

struct DesignRow {
  double ln_e = 0;
  double x = 0;
  double y = 0;
  double u = 0;
  double tau = 0;
};

DesignRow design_row(double kwh_pc, double gdp_pc, double urban, int year);
inline DesignRow design_row(const Obs& o) { return design_row(o.kwh_per_capita(), o.gdp_pc, o.urban, o.year); }

struct Design {
  std::vector<DesignRow> rows;
  std::vector<size_t> start;  // country c owns rows [start[c], start[c + 1])
  std::vector<std::string> iso3;
  size_t n_countries() const { return iso3.size(); }
};

Design make_design(const Panel& p);

struct FeQuad {
  double b1 = 0, b2 = 0, g = 0, d = 0;
  // Income elasticity of generation per capita at GDP per capita y_dollars.
  double elasticity(double y_dollars) const;
};

struct FeGompertz {
  double a = 0, b = 0, c = 0, d = 0;
  double elasticity(double y_dollars) const;
};

struct FitStats {
  size_t n = 0;
  size_t countries = 0;
  double rss = 0;
  double r2_within = 0;
};

// `sample` lists country indices for a cluster bootstrap (repeats allowed).
// An empty sample means every country once.
FeQuad fit_fe_quad(const Design& d, const std::vector<int>& sample = {});
FeGompertz fit_fe_gompertz(const Design& d, const std::vector<int>& sample = {});

FitStats fit_stats(const Design& d, const FeQuad& m);
FitStats fit_stats(const Design& d, const FeGompertz& m);

// Cluster (country) bootstrap, parallel over replicates with OpenMP.
std::vector<FeQuad> bootstrap_fe_quad(const Design& d, int replicates, uint64_t seed);
std::vector<FeGompertz> bootstrap_fe_gompertz(const Design& d, int replicates, uint64_t seed);

// Change in ln(kWh per capita) between two covariate states.
double delta_ln_e(const FeQuad& m, const DesignRow& from, const DesignRow& to);
double delta_ln_e(const FeGompertz& m, const DesignRow& from, const DesignRow& to);

struct DampedTrend {
  double alpha = 0, beta = 0, phi = 0;  // smoothing and damping parameters
  double level = 0, trend = 0;          // state after the last observation
  double sigma = 0;                     // one-step error standard deviation
  int n = 0;
};

// Grid search over (alpha, beta, phi) minimizing one-step squared error; OpenMP parallel.
DampedTrend fit_damped_trend(const std::vector<double>& series);
double damped_forecast(const DampedTrend& m, int h);

}  // namespace e47
