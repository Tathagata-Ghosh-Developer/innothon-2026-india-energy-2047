#pragma once
// State panel check: a diagnostic, never used by the forecast.
//
// The elasticity is estimated for India as a whole (india.hpp). This file asks what a
// state-level regression would give instead:
//   ln(NPP generation per person)_s,t = a_s + slope ln(GSDP per person)_s,t + e_s,t
// with state fixed effects a_s, over FY2018-19 to FY2023-24. Generation comes from the
// NPP state file (data/raw/npp/npp_state_generation_fy.csv); population and GSDP come
// from the challenge dataset (data/raw/competition/renewable_energy_dataset.csv).
//
// NPP records generation in the state where the plant stands, not where the power is
// used, and the state file covers conventional stations only (CEA 18-column reports).
// A state's generation per person therefore measures where plants are, not its demand:
// the spread of state generation per person against the national figure shows how far
// apart the two are. That is why the forecast estimates the elasticity nationally and
// applies it state by state (mc.hpp), and does not use this regression.
//
// Years are fiscal-year start years: 2018 means FY2018-19. Dataset year t is fiscal
// year t, as in the India core.
#include <string>
#include <vector>

#include "../core/csv.hpp"  // kNaN

namespace e47 {

class GsdpPriceBasis;  // core/price_base.hpp

// One state-year. A state the caller could not match carries NaN values.
struct PanelObs {
  std::string state;  // normalised name (normalize_state)
  int year = 0;
  double ln_gen_pc = kNaN;     // ln(kWh generated per person)
  double ln_income_pc = kNaN;  // ln(GSDP per person, thousand INR)
};

struct PanelFit {
  double slope = kNaN, se = kNaN;  // within slope and its conventional OLS s.e. (not clustered)
  int n_obs = 0;                   // state-years used
  int n_states = 0;                // states with at least one state-year used
  int n_skipped = 0;               // distinct state names left out because their values are not finite
};

// Within (state fixed effects) OLS of ln_gen_pc on ln_income_pc. An observation with a
// non-finite value is left out, and its state name counts once in n_skipped. The s.e.
// uses n_obs - n_states - 1 degrees of freedom. slope and se are NaN when there is no
// within variation or no degree of freedom left.
PanelFit fe_slope(const std::vector<PanelObs>& obs);

// Generation per person of one state over the whole window (sum of generation over sum
// of population, years with both), against the national figure on the same basis.
struct StateSpread {
  std::string state, region;  // normalised name; grid region as the NPP file gives it
  int n_years = 0;
  double gen_pc_kwh = kNaN;         // kWh per person
  double ratio_to_national = kNaN;  // gen_pc_kwh / StatePanelData::national_gen_pc_kwh
};

// Everything behind state_panel_check, for outputs/state_panel_check.csv.
struct StatePanelData {
  int first_year = 0, last_year = 0;
  // Usable state-years, plus one NaN observation per year for each NPP name the dataset
  // does not have (so fe_slope counts it in n_skipped).
  std::vector<PanelObs> obs;
  std::vector<std::string> skipped;  // NPP names with no match in the dataset (normalised), sorted
  int n_stale = 0;            // matched state-years dropped: GSDP equal to the year before (placeholder)
  int n_zero_generation = 0;  // matched state-years dropped: no generation in the NPP file
  int n_no_data = 0;          // matched state-years dropped: no dataset population or GSDP that year
  std::vector<StateSpread> spread;  // every matched state with NPP rows in the window, sorted by name
  // National figure: all NPP state rows (matched or not) over the dataset's whole
  // population, window totals; same conventional-only basis as the state figures.
  double national_gen_pc_kwh = kNaN;
  double sd_ln_ratio = kNaN;  // s.d. across states of ln(ratio_to_national), states with generation
  // Share of the variance of the usable observations that lies between states (the part
  // the state effects absorb): ln generation per person and ln income per person.
  double between_share_ln_gen = kNaN, between_share_ln_income = kNaN;
  std::vector<std::string> notes;  // what was left out and why
};

// Reads both files under repo_root. Rows left out: partial years, subtotals, rows whose
// region is not one of the five grid regions, years outside the window. Names are
// matched with normalize_state only (no fuzzy matching), so a name the dataset does not
// have is skipped and reported, never assigned to another state. The dataset's 2024
// GSDP is never read (the window ends at FY2023-24).
// basis (required): the GSDP price-basis corrections (Pipeline::gsdp_basis());
// GsdpPriceBasis::none() reads the GSDP as is.
StatePanelData state_panel_data(const std::string& repo_root, const GsdpPriceBasis& basis);

// fe_slope over state_panel_data(repo_root, basis).obs.
PanelFit state_panel_check(const std::string& repo_root, const GsdpPriceBasis& basis);

}  // namespace e47
