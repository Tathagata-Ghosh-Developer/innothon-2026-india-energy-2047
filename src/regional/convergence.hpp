#pragma once
// Inter-state income convergence from the organiser dataset (state GSDP and
// population, 2006-2024). Only these two columns of that file are used; its
// installed-capacity columns are never read.
//
// r_s,t = ln(GSDP per capita of state s / national GSDP per capita) in year t.
// Nominal GSDP is fine here: a common price level cancels in the ratio.
// Beta-convergence (long difference): (r_s,T - r_s,0) / (T - 0) = c + slope * r_s,0
// implied speed lambda = -ln(1 + slope * (T - 0)) / (T - 0) per year.
#include <map>
#include <string>
#include <vector>

namespace e47 {

std::string normalize_state(const std::string& name);

struct StateIncome {
  std::string state;  // normalized name
  std::map<int, double> ln_rel;  // year -> r_s,t
  std::map<int, double> pop;     // year -> persons
};

struct ConvergenceResult {
  int year0 = 0, year1 = 0;
  int n_states = 0;
  double slope = 0;
  double lambda = 0;     // per year; positive = convergence
  double lambda_se = 0;  // bootstrap over states
  std::vector<StateIncome> states;
};

ConvergenceResult estimate_convergence(const std::string& organiser_csv, int year0, int year1, int replicates,
                                       unsigned long long seed);

}  // namespace e47
