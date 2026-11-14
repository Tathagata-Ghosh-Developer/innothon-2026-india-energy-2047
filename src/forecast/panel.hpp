#pragma once
// Cross-country panel of electricity generation per capita and its drivers.
//
// Generation comes from Ember's yearly global release ("Total generation", TWh).
// The Ember rows "Demand" and "Net imports" are never read: the competition
// allows energy generated only.
// Drivers come from the World Bank WDI: population, GDP per capita (PPP,
// constant 2021 international dollars) and urban population share.
#include <string>
#include <vector>

namespace e47 {

struct Obs {
  int year = 0;
  double gen_twh = 0;  // total electricity generation, TWh
  double pop = 0;      // persons
  double gdp_pc = 0;   // GDP per capita, PPP, constant 2021 international $
  double urban = 0;    // urban population share, fraction in [0, 1]

  double kwh_per_capita() const { return gen_twh * 1e9 / pop; }
};

struct CountrySeries {
  std::string iso3;
  std::string name;
  std::vector<Obs> obs;  // complete cases, sorted by year
};

struct PanelFilter {
  int first_year = 1990;
  int last_year = 2100;
  double min_population = 1e6;  // drop micro-states
  int min_years = 10;           // need within-country variation
};

struct Panel {
  std::vector<CountrySeries> countries;

  const CountrySeries* find(const std::string& iso3) const;
  size_t n_obs() const;
};

struct PanelSources {
  std::string ember_generation_csv;
  std::string wb_gdp_pc_csv;
  std::string wb_urban_csv;
  std::string wb_population_csv;
};

Panel load_panel(const PanelSources& src, const PanelFilter& filter);

// Copy of the panel keeping only years <= last_year (for backtests).
Panel truncate_panel(const Panel& p, int last_year, int min_years);

}  // namespace e47
