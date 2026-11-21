#include "state_panel.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <map>
#include <set>

#include "../core/csv.hpp"
#include "../core/price_base.hpp"
#include "../core/stats.hpp"
#include "../regional/convergence.hpp"  // normalize_state
#include "mc.hpp"                       // kRegions, region_name

namespace e47 {

namespace {

namespace fs = std::filesystem;

constexpr int kFirst = 2018;  // FY2018-19: first year of the NPP state file
constexpr int kLast = 2023;   // FY2023-24: base year of the India core (the dataset's 2024 is not read)

bool is_grid_region(const std::string& name) {
  for (int r = 0; r < kRegions; ++r)
    if (name == region_name(r)) return true;
  return false;
}

bool is_true(const std::string& s) { return lower(trim(s)) == "true"; }

bool usable(const PanelObs& o) { return std::isfinite(o.ln_gen_pc) && std::isfinite(o.ln_income_pc); }

// Share of the variance of one variable that lies between states: 1 - within SS / total SS.
double between_share(const std::vector<PanelObs>& obs, double PanelObs::*v) {
  std::map<std::string, std::pair<double, int>> by_state;  // sum, count
  double sum = 0;
  int n = 0;
  for (const PanelObs& o : obs) {
    if (!usable(o)) continue;
    auto& s = by_state[o.state];
    s.first += o.*v;
    ++s.second;
    sum += o.*v;
    ++n;
  }
  if (n < 2) return kNaN;
  const double grand = sum / n;
  double total = 0, within = 0;
  for (const PanelObs& o : obs) {
    if (!usable(o)) continue;
    const auto& s = by_state.at(o.state);
    total += (o.*v - grand) * (o.*v - grand);
    within += (o.*v - s.first / s.second) * (o.*v - s.first / s.second);
  }
  return total > 0 ? 1.0 - within / total : kNaN;
}

}  // namespace

PanelFit fe_slope(const std::vector<PanelObs>& obs) {
  PanelFit f;
  std::set<std::string> skipped;
  std::map<std::string, std::pair<double, double>> sums;  // state -> sum of x, sum of y
  std::map<std::string, int> count;
  for (const PanelObs& o : obs) {
    if (!usable(o)) {
      skipped.insert(o.state);
      continue;
    }
    sums[o.state].first += o.ln_income_pc;
    sums[o.state].second += o.ln_gen_pc;
    ++count[o.state];
    ++f.n_obs;
  }
  f.n_skipped = static_cast<int>(skipped.size());
  f.n_states = static_cast<int>(count.size());

  // Deviations from each state's own mean remove the state effects.
  std::vector<std::pair<double, double>> dev;  // (x, y) within the state
  dev.reserve(static_cast<size_t>(f.n_obs));
  for (const PanelObs& o : obs) {
    if (!usable(o)) continue;
    const double n = count.at(o.state);
    const auto& s = sums.at(o.state);
    dev.push_back({o.ln_income_pc - s.first / n, o.ln_gen_pc - s.second / n});
  }
  double sxx = 0, sxy = 0;
  for (const auto& [x, y] : dev) {
    sxx += x * x;
    sxy += x * y;
  }
  if (!(sxx > 0)) return f;  // no within variation: slope not identified
  f.slope = sxy / sxx;
  const int dof = f.n_obs - f.n_states - 1;
  if (dof > 0) {
    double ssr = 0;
    for (const auto& [x, y] : dev) ssr += (y - f.slope * x) * (y - f.slope * x);
    f.se = std::sqrt(ssr / dof / sxx);
  }
  return f;
}

StatePanelData state_panel_data(const std::string& repo_root, const GsdpPriceBasis* basis) {
  const fs::path root(repo_root);
  StatePanelData d;
  d.first_year = kFirst;
  d.last_year = kLast;

  // 1. NPP state file: GWh by state and year, summed over the sources.
  struct Generation {
    std::string region;
    std::map<int, double> gwh;
  };
  std::map<std::string, Generation> gen;
  std::map<int, double> all_gwh;  // every state row, matched or not (national figure)
  std::set<std::string> two_regions;
  int blank = 0;
  {
    const CsvTable t = CsvTable::read((root / "data/raw/npp/npp_state_generation_fy.csv").string());
    const size_t cy = t.col("fy_start_year"), cr = t.col("region"), cs = t.col("state"), cg = t.col("generation_gwh"),
                 csub = t.col("is_subtotal"), cp = t.col("partial");
    for (size_t i = 0; i < t.rows(); ++i) {
      if (is_true(t.str(i, csub)) || is_true(t.str(i, cp))) continue;  // subtotals; part of a year
      const double y = t.num(i, cy);
      if (std::isnan(y) || y < kFirst || y > kLast) continue;
      const std::string region = trim(t.str(i, cr));
      if (!is_grid_region(region)) continue;  // All India and other aggregates
      const std::string s = normalize_state(t.str(i, cs));
      if (s.empty()) continue;
      const double g = t.num(i, cg);
      if (std::isnan(g)) {
        ++blank;
        continue;
      }
      Generation& e = gen[s];
      if (e.region.empty()) e.region = region;
      if (e.region != region && two_regions.insert(s).second)
        d.notes.push_back(s + ": NPP lists it under " + e.region + " and " + region + "; " + e.region + " shown");
      e.gwh[static_cast<int>(y)] += g;
      all_gwh[static_cast<int>(y)] += g;
    }
  }
  if (blank > 0) d.notes.push_back(std::to_string(blank) + " NPP rows with a blank generation cell left out");

  // 2. Challenge dataset: population and GSDP (crore INR) by state and year. The year
  //    before the window is read for the repeated-GSDP rule only.
  struct StateYear {
    double pop = kNaN, gsdp = kNaN;  // gsdp at constant prices (after the price-basis correction)
    double gsdp_read = kNaN;         // as read: the repeated-value rule compares these
  };
  std::set<std::string> names;  // every state name in the dataset, any year
  std::map<std::string, std::map<int, StateYear>> org;
  std::map<int, double> all_pop;  // every dataset state (national figure)
  {
    const CsvTable t = CsvTable::read((root / "data/raw/competition/renewable_energy_dataset.csv").string());
    const size_t cs = t.col("State"), cy = t.col("Year"), cp = t.col("Population"), cg = t.col("GSDP (crore INR)");
    for (size_t i = 0; i < t.rows(); ++i) {
      const std::string s = normalize_state(t.str(i, cs));
      if (s.empty()) continue;
      names.insert(s);
      const double y = t.num(i, cy);
      if (std::isnan(y) || y < kFirst - 1 || y > kLast) continue;
      const double p = t.num(i, cp), g = t.num(i, cg);
      StateYear& v = org[s][static_cast<int>(y)];
      v.pop = p > 0 ? p : kNaN;
      v.gsdp_read = g > 0 ? g : kNaN;
      v.gsdp = g > 0 && basis ? basis->constant_2011_12(t.str(i, cs), static_cast<int>(y), g) : v.gsdp_read;
      if (y >= kFirst && p > 0) all_pop[static_cast<int>(y)] += p;
    }
  }

  // 3. Panel and spread. A name the dataset does not have is skipped: its years enter
  //    as NaN observations so fe_slope counts the name, and nothing is assigned to it.
  for (const auto& [s, e] : gen) {
    if (!names.count(s)) {
      d.skipped.push_back(s);
      for (const auto& [year, g] : e.gwh) d.obs.push_back({s, year, kNaN, kNaN});
      d.notes.push_back(s + ": in the NPP state file, not in the challenge dataset; skipped (its generation is not "
                            "assigned to any other state)");
      continue;
    }
    const auto it = org.find(s);
    const auto at = [&](int year) -> const StateYear* {
      if (it == org.end()) return nullptr;
      const auto y = it->second.find(year);
      return y == it->second.end() ? nullptr : &y->second;
    };
    StateSpread sp;
    sp.state = s;
    sp.region = e.region;
    double sum_gwh = 0, sum_pop = 0;
    int zero = 0;
    for (const auto& [year, g] : e.gwh) {
      const StateYear* cur = at(year);
      if (cur && std::isfinite(cur->pop)) {
        sum_gwh += std::max(0.0, g);
        sum_pop += cur->pop;
        ++sp.n_years;
      }
      if (!(g > 0)) {
        ++zero;
        continue;
      }
      if (!cur || !std::isfinite(cur->pop) || !std::isfinite(cur->gsdp)) {
        ++d.n_no_data;
        continue;
      }
      const StateYear* prev = at(year - 1);
      if (prev && prev->gsdp_read == cur->gsdp_read) {  // placeholder copied from the year before
        ++d.n_stale;
        continue;
      }
      // kWh per person; thousand INR per person.
      d.obs.push_back({s, year, std::log(g * 1e6 / cur->pop), std::log(cur->gsdp * 1e7 / cur->pop / 1e3)});
    }
    d.n_zero_generation += zero;
    if (zero > 0)
      d.notes.push_back(s + ": no generation in " + std::to_string(zero) + " of " + std::to_string(e.gwh.size()) +
                        " years in the NPP state file; those years are not in the regression");
    if (sum_pop > 0) sp.gen_pc_kwh = sum_gwh * 1e6 / sum_pop;
    d.spread.push_back(sp);
  }
  if (d.n_stale > 0)
    d.notes.push_back(std::to_string(d.n_stale) + " state-years with GSDP equal to the year before left out");
  if (d.n_no_data > 0)
    d.notes.push_back(std::to_string(d.n_no_data) + " state-years with no dataset population or GSDP left out");
  {
    std::string none;
    for (const auto& [s, years] : org)
      if (!gen.count(s)) none += (none.empty() ? "" : ", ") + s;
    if (!none.empty()) d.notes.push_back("in the challenge dataset with no NPP generation rows: " + none);
  }

  // National figure on the same basis, then each state's ratio to it.
  double nat_gwh = 0, nat_pop = 0;
  for (const auto& [year, p] : all_pop)
    if (all_gwh.count(year)) nat_gwh += all_gwh.at(year), nat_pop += p;
  if (nat_pop > 0) d.national_gen_pc_kwh = nat_gwh * 1e6 / nat_pop;
  std::vector<double> ln_ratio;
  for (StateSpread& sp : d.spread) {
    sp.ratio_to_national = sp.gen_pc_kwh / d.national_gen_pc_kwh;
    if (sp.ratio_to_national > 0) ln_ratio.push_back(std::log(sp.ratio_to_national));
  }
  if (ln_ratio.size() >= 2) d.sd_ln_ratio = stdev(ln_ratio);
  d.between_share_ln_gen = between_share(d.obs, &PanelObs::ln_gen_pc);
  d.between_share_ln_income = between_share(d.obs, &PanelObs::ln_income_pc);
  return d;
}

PanelFit state_panel_check(const std::string& repo_root) { return fe_slope(state_panel_data(repo_root).obs); }

}  // namespace e47
