#pragma once
// Inter-state income convergence from the organiser dataset (state GSDP and
// population, 2006-2024). Only these two columns of that file are used; its
// installed-capacity columns are never read.
//
// r_s,t = ln(GSDP per capita of state s / national GSDP per capita) in year t.
// Within a year every state must be on one price basis, which then cancels in the ratio
// (the dataset moves from 2004-05 to 2011-12 prices in 2011 for all states at once). A state
// listed in `basis` (gsdp_price_basis.csv: Jammu and Kashmir, current prices from 2012) is
// deflated to constant 2011-12 prices first; without it, its ratio would carry inflation.
//
// Base year b_s of each state: its first year from year0 (and before T = year1) whose GSDP, as
// read, equals neither the year before nor the year after. A run of equal values is one
// placeholder copied across years, the rule load_india_series applies to income growth
// (Jammu and Kashmir, Telangana, Andaman and Nicobar Islands and Lakshadweep repeat one figure
// for 2006-2010; Nagaland for 2006-2007), so its relative income is not a measurement. A state
// with no value in year0 starts at its first year (Delhi 2009, Chandigarh 2013).
// Beta-convergence (long difference over each state's own span T - b_s):
//   (r_s,T - r_s,b_s) / (T - b_s) = c + slope * r_s,b_s
// implied speed lambda = -ln(1 + slope * S) / S per year, with S the mean span over the states
// in the regression (the exact relation when every span is equal).
#include <map>
#include <string>
#include <vector>

namespace e47 {

class GsdpPriceBasis;  // core/price_base.hpp

std::string normalize_state(const std::string& name);

struct StateIncome {
  std::string state;  // normalized name
  std::map<int, double> ln_rel;  // year -> r_s,t
  std::map<int, double> pop;     // year -> persons
  int base_year = 0;             // b_s; 0 = not in the regression (no base year before year1, or no year1 value)
};

struct ConvergenceResult {
  int year0 = 0, year1 = 0;
  int n_states = 0;       // states in the regression
  int n_late_base = 0;    // of which with a base year after year0 (placeholder run, or data from a later year)
  double mean_span = 0;   // S: mean of year1 - b_s over the states in the regression
  double slope = 0;
  double lambda = 0;     // per year; positive = convergence
  double lambda_se = 0;  // bootstrap over states
  std::vector<StateIncome> states;
};

// basis (required): Pipeline::gsdp_basis(); GsdpPriceBasis::none() reads the GSDP as is.
ConvergenceResult estimate_convergence(const std::string& organiser_csv, int year0, int year1, int replicates,
                                       unsigned long long seed, const GsdpPriceBasis& basis);

}  // namespace e47
