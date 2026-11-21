// Regional split checks (organisers' ruling of 16 November 2026: the regional split
// uses only the challenge data).
//  1. Every state with a population in the challenge dataset is in the split, mapped
//     to the grid region the NPP state file reports for it, or (for a state NPP does
//     not list) to the region of its dataset Zone.
//  2. State population shares sum to 1 in every projection year.
//  3. In the first projection year the shares equal the dataset's last-year shares.
//  4. The national forecast does not depend on the regional split.
//  5. The state panel check (a diagnostic, never used by the forecast): the within
//     estimator recovers a known slope, and names it cannot match are skipped and counted.
//  6. The state table: one row per state, shares sum to 1, and its regional sums agree
//     with the Monte Carlo's regional medians.
//  7. The new-loads scenario (outside the headline): only loads absent from the
//     historical series (overlaps_history "no") enter the add-on, scaled from their
//     published year to 2047, and the headline with no add-on is unchanged.
// Usage: test_regions <repo_root>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "core/csv.hpp"
#include "forecast/state_panel.hpp"
#include "pipeline/pipeline.hpp"
#include "regional/convergence.hpp"

using namespace e47;
namespace fs = std::filesystem;

namespace {

int fails = 0;

void check(bool ok, const std::string& what) {
  std::printf("  %-74s %s\n", what.c_str(), ok ? "ok" : "FAIL");
  if (!ok) ++fails;
}

int region_index(const std::string& name) {
  for (int r = 0; r < kRegions; ++r)
    if (name == region_name(r)) return r;
  return -1;
}

// The challenge dataset read directly (not through the pipeline).
struct Dataset {
  int last_year = 0;
  std::map<std::string, std::map<int, double>> pop;  // normalised state -> year -> persons
  std::map<std::string, std::string> zone;           // normalised state -> Zone column
};

Dataset read_dataset(const fs::path& root) {
  Dataset d;
  const CsvTable t = CsvTable::read((root / "data/raw/competition/renewable_energy_dataset.csv").string());
  for (size_t i = 0; i < t.rows(); ++i) {
    const double y = t.num(i, t.col("Year")), p = t.num(i, t.col("Population"));
    if (std::isnan(y) || std::isnan(p) || p <= 0) continue;
    const std::string s = normalize_state(t.str(i, t.col("State")));
    d.pop[s][static_cast<int>(y)] = p;
    d.zone[s] = t.str(i, t.col("Zone"));
    d.last_year = std::max(d.last_year, static_cast<int>(y));
  }
  return d;
}

// Expected grid region: NPP's own region for the state, else the dataset Zone.
std::map<std::string, int> expected_regions(const fs::path& root, const Dataset& d) {
  std::map<std::string, int> npp;
  const CsvTable t = CsvTable::read((root / "data/raw/npp/npp_state_generation_fy.csv").string());
  for (size_t i = 0; i < t.rows(); ++i) {
    const int r = region_index(t.str(i, t.col("region")));
    if (r >= 0) npp[normalize_state(t.str(i, t.col("state")))] = r;
  }
  const std::map<std::string, std::string> zone_region = {{"North", "Northern"}, {"South", "Southern"},
                                                          {"East", "Eastern"},   {"West", "Western"},
                                                          {"North-East", "North-Eastern"},
                                                          {"Central", "Western"}};
  std::map<std::string, int> out;
  for (const auto& [s, z] : d.zone) {
    if (!d.pop.at(s).count(d.last_year)) continue;  // populated = has a population in the dataset's last year
    auto it = npp.find(s);
    if (it != npp.end()) {
      out[s] = it->second;
    } else {
      auto zr = zone_region.find(z);
      out[s] = zr == zone_region.end() ? -1 : region_index(zr->second);
    }
  }
  return out;
}

void states_and_regions(const Pipeline& pl, const Dataset& d, const std::map<std::string, int>& want) {
  std::printf("every populated state mapped to its grid region\n");
  const DriverData& drv = pl.drivers();
  std::set<std::string> in_split(drv.states.begin(), drv.states.end());
  for (const auto& [s, r] : want) {
    auto it = std::find(drv.states.begin(), drv.states.end(), s);
    const bool found = it != drv.states.end();
    const int got = found ? drv.state_region[static_cast<size_t>(it - drv.states.begin())] : -1;
    check(found && r >= 0 && got == r,
          s + " -> " + (r >= 0 ? region_name(r) : "?") + (found ? std::string(" (got ") + (got >= 0 && got < kRegions ? region_name(got) : "?") + ")" : " (missing)"));
  }
  for (const std::string& s : drv.states)
    if (!d.pop.count(s)) check(false, s + " is in the split but has no population in the challenge dataset");
}

void shares(const Pipeline& pl, const Dataset& d) {
  std::printf("state population shares\n");
  const DriverData& drv = pl.drivers();
  double worst = 0;
  for (size_t t = 0; t < drv.years.size(); ++t) {
    double sum = 0;
    for (const auto& v : drv.state_pop_share) sum += v[t];
    worst = std::max(worst, std::abs(sum - 1.0));
  }
  check(!drv.state_pop_share.empty() && worst < 1e-9, "shares sum to 1 in every projection year");

  check(!drv.years.empty() && drv.years.front() == d.last_year,
        "first projection year is the dataset's last year (" + std::to_string(d.last_year) + ")");
  double total = 0;
  for (const auto& [s, p] : d.pop)
    if (p.count(d.last_year)) total += p.at(d.last_year);
  double err = 0;
  for (size_t k = 0; k < drv.states.size(); ++k) {
    auto it = d.pop.find(drv.states[k]);
    const double want = (it != d.pop.end() && it->second.count(d.last_year)) ? it->second.at(d.last_year) / total : 0.0;
    err = std::max(err, std::abs(drv.state_pop_share[k][0] - want));
  }
  char what[96];
  std::snprintf(what, sizeof what, "first-year shares equal the dataset's %d shares (max error %.1e)", d.last_year, err);
  check(!drv.states.empty() && err < 1e-9, what);
}

// The regional rework must leave the national forecast as it was. Reference values:
// the 2047 national quantiles of the model (CPU, 2^18 draws, default seed), updated on
// 2026-11-21 for the Jammu and Kashmir GSDP price-basis correction (before it: P5 3214.085,
// P50 4699.020, P95 7002.236); the tolerance allows for Monte Carlo noise only.
void national_unchanged(const Pipeline& pl) {
  std::printf("national 2047 forecast unchanged by the regional split\n");
  ScenarioControls c;
  c.draws = 1u << 18;
  c.use_gpu = false;
  const ForecastResult f = pl.forecast(c);
  const auto& q = f.quant.q[0][f.years.size() - 1];
  std::printf("    %d: P5 %.3f  P50 %.3f  P95 %.3f TWh\n", f.years.back(), q[0], q[3], q[6]);
  const double before[3] = {3187.139, 4684.099, 7021.520};  // P5, P50, P95
  const int k[3] = {0, 3, 6};
  const char* name[3] = {"P5", "P50", "P95"};
  check(f.years.back() == 2047, "last projection year is 2047");
  for (int i = 0; i < 3; ++i) {
    char what[96];
    std::snprintf(what, sizeof what, "%s %.1f TWh within 0.5%% of %.1f TWh", name[i], q[k[i]], before[i]);
    check(std::abs(q[k[i]] / before[i] - 1) < 0.005, what);
  }
}

// The model's national population path must stay close to the challenge dataset's
// own recent growth (the organisers' consistency condition), and the reported
// path must be the one the forecast uses.
void population_consistent(const Pipeline& pl) {
  std::printf("population consistency\n");
  const PopulationConsistency pc = pl.population_consistency();
  check(!pc.rows.empty() && pc.rows.front().area == "India", "first row is India");
  if (pc.rows.empty()) return;
  const PopulationGrowth& india = pc.rows.front();
  std::printf("    India: dataset %d-%d %.3f%%/yr, model %d-%d %.3f%%/yr, UN WPP 2024 %.3f%%/yr\n", pc.recent_from, pc.base,
              100 * india.data_recent, pc.base, pc.near, 100 * india.model_near, 100 * pc.un_near);
  check(pc.recent_from == 2019 && pc.base == 2024 && pc.near == 2030, "spans 2019-2024 (dataset) and 2024-2030 (model)");
  check(std::isfinite(india.data_recent) && std::isfinite(india.model_near) &&
            std::abs(india.model_near - india.data_recent) < 0.005,
        "national path 2024-2030 within 0.5 points of the dataset's 2019-2024 growth");

  // Same path as the forecast's population input (millions, index 0 = first projection year).
  const McInputs in = pl.build_inputs(ScenarioControls{});
  const auto& years = pl.drivers().years;
  const auto at = [&](int y) { return static_cast<double>(in.pop_med[y - years.front()]); };
  const double forecast_near = std::pow(at(pc.near) / at(pc.base), 1.0 / (pc.near - pc.base)) - 1;
  check(std::abs(forecast_near - india.model_near) < 1e-5, "reported national path is the forecast's population input");

  // Every state in the split has a row with finite model growth.
  size_t n_states = 0;
  for (const auto& r : pc.rows)
    if (r.area != "India" && std::isfinite(r.model_near) && std::isfinite(r.model_target) && std::isfinite(r.data_recent))
      ++n_states;
  check(n_states == pl.drivers().states.size(), "a finite growth row for every state in the split");
}

// Synthetic panel: 5 states x 6 years, ln generation per person = a_s + 0.8 ln income
// per person. The state effects a_s fall as income rises across states, so a pooled
// regression without state effects is far from 0.8; the within estimator is exact.
constexpr double kTrueSlope = 0.8;
double synth_x(int s, int y) { return 3.0 + 0.4 * s + 0.06 * (y - 2018) + 0.02 * ((3 * s + 2 * (y - 2018)) % 5); }
double synth_a(int s) { return 1.0 - 1.2 * s; }

std::vector<PanelObs> synthetic_panel() {
  std::vector<PanelObs> v;
  for (int s = 0; s < 5; ++s)
    for (int y = 2018; y <= 2023; ++y)
      v.push_back({"state " + std::to_string(s), y, synth_a(s) + kTrueSlope * synth_x(s, y), synth_x(s, y)});
  return v;
}

void fe_slope_recovers_known() {
  std::printf("fe_slope_recovers_known\n");
  const std::vector<PanelObs> v = synthetic_panel();
  const PanelFit f = fe_slope(v);
  double mx = 0, my = 0, sxy = 0, sxx = 0;  // pooled OLS, no state effects
  for (const PanelObs& o : v) mx += o.ln_income_pc / v.size(), my += o.ln_gen_pc / v.size();
  for (const PanelObs& o : v) sxy += (o.ln_income_pc - mx) * (o.ln_gen_pc - my), sxx += (o.ln_income_pc - mx) * (o.ln_income_pc - mx);
  std::printf("    within slope %.12f (s.e. %.2e), pooled slope without state effects %.3f\n", f.slope, f.se, sxy / sxx);
  check(std::abs(f.slope - kTrueSlope) < 1e-9, "within slope recovers 0.8 (1e-9)");
  check(f.n_obs == 30 && f.n_states == 5 && f.n_skipped == 0, "30 observations, 5 states, none skipped");
  check(std::abs(sxy / sxx - kTrueSlope) > 0.5, "pooled slope without state effects is far from 0.8");
}

void write_text(const fs::path& p, const std::string& text) {
  fs::create_directories(p.parent_path());
  std::ofstream(p, std::ios::binary) << text;
}

std::string num17(double v) {
  char b[40];
  std::snprintf(b, sizeof b, "%.17g", v);
  return b;
}

void fe_slope_skips_unmatched(const fs::path& tmp) {
  std::printf("fe_slope_skips_unmatched\n");
  // In memory: a name the caller could not match carries NaN values.
  {
    std::vector<PanelObs> v = synthetic_panel();
    for (int y = 2018; y <= 2023; ++y) v.push_back({"atlantis", y, kNaN, kNaN});
    const PanelFit f = fe_slope(v);
    check(f.n_skipped == 1, "one unknown name: n_skipped == 1");
    check(f.n_obs == 30 && f.n_states == 5 && std::abs(f.slope - kTrueSlope) < 1e-9,
          "the unknown name changes neither the sample nor the slope");
  }
  // Through the file reader: a synthetic repository. The NPP state file spells names
  // differently from the dataset (case, '&', spaces), splits generation by source, and
  // has rows that must be left out: a partial year, a subtotal, an All India row, years
  // outside FY2018-19..FY2023-24, a state with no generation (Zeta), and one name the
  // dataset does not have (Atlantis). The dataset repeats Epsilon's 2021 GSDP (stale
  // year, dropped) and has a state with no generation rows (Chandi).
  fs::remove_all(tmp);
  const char* data_name[] = {"Alpha Pradesh", "Beta Nadu", "Gamma Bengal", "Delta And Islands", "Epsilon"};
  const char* npp_name[] = {"Alpha Pradesh", "BETA NADU", "Gamma  Bengal", "Delta & Islands", "Epsilon"};
  const char* npp_region[] = {"Northern", "Southern", "Eastern", "Eastern", "Western"};
  const auto pop = [](int s, int y) { return 1e6 * (s + 1) * (1.0 + 0.012 * (y - 2016)); };
  std::string data = "State,Year,Zone,Population,GSDP (crore INR)\n";
  for (int s = 0; s < 5; ++s)
    for (int y = 2016; y <= 2024; ++y) {
      const int gy = (s == 4 && y == 2021) ? 2020 : y;  // Epsilon 2021 repeats its 2020 GSDP
      const double gsdp = std::exp(synth_x(s, gy)) * 1e3 * pop(s, gy) / 1e7;  // thousand INR per person -> crore INR
      data += std::string(data_name[s]) + "," + std::to_string(y) + ",North," + num17(pop(s, y)) + "," + num17(gsdp) + "\n";
    }
  for (int y = 2016; y <= 2024; ++y) {
    data += "Zeta," + std::to_string(y) + ",West,2000000,5000\n";
    data += "Chandi," + std::to_string(y) + ",North,1000000,4000\n";
  }
  write_text(tmp / "data/raw/competition/renewable_energy_dataset.csv", data);

  std::string npp = "fiscal_year,fy_start_year,region,state,source,generation_gwh,is_subtotal,partial,period,origin,source_id\n";
  const auto row = [&](int y, const std::string& region, const std::string& state, const std::string& source, double gwh,
                       bool subtotal, bool partial) {
    npp += std::to_string(y) + "-" + std::to_string((y + 1) % 100) + "," + std::to_string(y) + "," + region + "," + state +
           "," + source + "," + num17(gwh) + "," + (subtotal ? "True" : "False") + "," + (partial ? "True" : "False") +
           ",Apr-Mar,cumulative_actual,synthetic\n";
  };
  for (int s = 0; s < 5; ++s)
    for (int y = 2017; y <= 2024; ++y) {
      const bool in = y >= 2018 && y <= 2023;
      // GWh from ln(kWh per person) = a_s + 0.8 x; outside the window a value that would break the fit.
      const double gwh = in ? std::exp(synth_a(s) + kTrueSlope * synth_x(s, y)) * pop(s, y) / 1e6 : 1e6;
      row(y, npp_region[s], npp_name[s], "coal_lignite", 0.3 * gwh, false, false);
      row(y, npp_region[s], npp_name[s], "hydro", 0.7 * gwh, false, false);
    }
  row(2023, "Northern", "Alpha Pradesh", "coal_lignite", 99999, false, true);  // partial: left out
  row(2020, "Southern", "Beta Nadu", "total", 88888, true, false);             // subtotal: left out
  row(2021, "All India", "All India", "hydro", 77777, false, false);            // not a state
  for (int y = 2018; y <= 2023; ++y) {
    row(y, "Western", "Zeta", "gas", 0.0, false, false);         // matched, no generation
    row(y, "Northern", "Atlantis", "hydro", 500.0, false, false);  // not in the dataset
  }
  write_text(tmp / "data/raw/npp/npp_state_generation_fy.csv", npp);

  const StatePanelData d = state_panel_data(tmp.string());
  const PanelFit g = state_panel_check(tmp.string());
  std::string names;
  for (const std::string& s : d.skipped) names += (names.empty() ? "" : ", ") + s;
  std::printf("    files: slope %.12f, n_obs %d, n_states %d, n_skipped %d (%s), stale %d, zero generation %d\n", g.slope,
              g.n_obs, g.n_states, g.n_skipped, names.c_str(), d.n_stale, d.n_zero_generation);
  check(g.n_skipped == 1 && d.skipped == std::vector<std::string>{"atlantis"}, "files: n_skipped == 1 (atlantis)");
  check(g.n_states == 5 && g.n_obs == 29, "files: 5 states, 29 state-years (Epsilon's stale 2021 dropped)");
  check(std::abs(g.slope - kTrueSlope) < 1e-9, "files: within slope recovers 0.8 (1e-9)");
  check(d.n_stale == 1 && d.n_zero_generation == 6, "files: 1 stale GSDP year and 6 zero-generation years dropped");
  const auto zeta = std::find_if(d.spread.begin(), d.spread.end(), [](const StateSpread& r) { return r.state == "zeta"; });
  check(zeta != d.spread.end() && zeta->gen_pc_kwh == 0.0, "files: Zeta is matched, with zero generation per person");
}

// State table: the kernel's regional split made visible, one row per state.
void state_table_shares_sum(const Pipeline& pl) {
  std::printf("state_table_shares_sum\n");
  ScenarioControls c;
  c.draws = 1u << 16;
  c.use_gpu = false;
  const std::vector<StateRow> rows = pl.state_table(c, 2047);
  double sum = 0;
  std::map<std::string, double> by_region;
  for (const StateRow& r : rows) {
    sum += r.requirement_share_target;
    by_region[r.region] += r.requirement_share_target;
  }
  check(rows.size() == 34, "34 rows, one per state (" + std::to_string(rows.size()) + ")");
  check(std::abs(sum - 1.0) < 1e-9, "2047 requirement shares sum to 1 (1e-9)");

  // Regional medians of the Monte Carlo, as shares of their sum (as Pipeline::strategy).
  const ForecastResult f = pl.forecast(c);
  const size_t t = f.years.size() - 1;
  double med[kRegions], med_sum = 0;
  for (int r = 0; r < kRegions; ++r) med_sum += med[r] = f.quant.q[1 + r][t][3];
  double worst = 0;
  for (int r = 0; r < kRegions; ++r) {
    const double table = by_region[region_name(r)], mc = med[r] / med_sum;
    std::printf("    %-14s state table %.4f | Monte Carlo medians %.4f\n", region_name(r), table, mc);
    worst = std::max(worst, std::abs(table - mc));
  }
  char what[96];
  std::snprintf(what, sizeof what, "regional sums match the regional medians' shares within 0.02 (worst %.4f)", worst);
  check(worst < 0.02, what);
}

// New loads: a temporary file with one load of each overlap class. Only the "no" load
// enters the add-on, scaled from 2050 to 2047 by (2047 - base) / (2050 - base); the
// totals with overlapping loads are reported but never added. The headline (default
// controls, no add-on) is unchanged.
void new_loads_excludes_overlap(const Pipeline& pl, const fs::path& tmp) {
  std::printf("new_loads_excludes_overlap\n");
  fs::remove_all(tmp);
  const fs::path file = tmp / "new_loads.csv";
  write_text(file,
             "load,year,twh_low,twh_central,twh_high,basis,overlaps_history,source_name\n"
             "cooling,2050,500,700,900,\"2050 as published, end use\",yes,synthetic\n"
             "vehicles,2050,300,400,600,2050 as published,partial,synthetic\n"
             "hydrogen,2050,270,540,810,2050 as published,no,synthetic\n");
  const int base = pl.drivers().base_year;
  const double s = (2047.0 - base) / (2050.0 - base);
  std::printf("    base year %d, scale to 2047 = %.6f\n", base, s);
  check(base == 2023 && std::abs(s - 24.0 / 27.0) < 1e-15,
        "base year FY2023-24, scale (2047-2023)/(2050-2023) = 24/27");
  const auto near = [](double a, double b) { return std::abs(a - b) < 1e-9 * std::max(1.0, std::abs(b)); };

  const std::vector<NewLoad> v = read_new_loads(file.string(), base, 2047);
  check(v.size() == 3 && v[0].overlaps == "yes" && v[1].overlaps == "partial" && v[2].overlaps == "no",
        "three loads read with their overlap class");
  if (v.size() != 3) return;
  check(v[2].load == "hydrogen" && v[2].year == 2050 && near(v[2].scale, s) && near(v[2].low_published, 270) &&
            near(v[2].low, 270 * s) && near(v[2].central, 540 * s) && near(v[2].high, 810 * s),
        "values scaled to 2047, published values kept");
  check(v[0].basis == "2050 as published, end use", "basis read (quoted field)");

  const NewLoadTotals added = new_loads_total(v);
  const NewLoadTotals all = new_loads_total(v, true);
  std::printf("    add-on %.3f-%.3f TWh (n %d) | all loads %.3f-%.3f TWh (n %d)\n", added.low, added.high, added.n,
              all.low, all.high, all.n);
  check(added.n == 1 && near(added.low, 270 * s) && near(added.central, 540 * s) && near(added.high, 810 * s),
        "add-on totals: the 'no' load only, scaled");
  check(all.n == 3 && near(all.low, 1070 * s) && near(all.high, 2310 * s), "totals with overlapping loads: all three");

  ScenarioControls c;
  apply_new_loads(c, v);
  check(c.addon_lo == added.low && c.addon_hi == added.high, "apply_new_loads sets addon_lo / addon_hi to the add-on");
  const std::string desc = new_loads_description(v);
  std::printf("    description: %s\n", desc.c_str());
  const auto has = [&](const char* s) { return desc.find(s) != std::string::npos; };
  check(has("Outside the headline") && has("(hydrogen): 240-720 TWh in 2047") && has("if supplied from the grid") &&
            has("not grossed up for losses") && has("Not added") && has(": cooling, vehicles."),
        "description: outside the headline, grid label, no loss gross-up, which loads are added");

  // Headline: default controls carry no add-on; overlapping loads alone add nothing,
  // so the P50 is the same number; the "no" load moves the P50 by an amount inside its range.
  ScenarioControls h;
  h.draws = 1u << 18;
  h.use_gpu = false;
  check(h.addon_lo == 0 && h.addon_hi == 0, "default controls carry no add-on");
  ScenarioControls z = h;
  apply_new_loads(z, {v[0], v[1]});
  ScenarioControls n = h;
  apply_new_loads(n, v);
  const auto p50 = [&](const ScenarioControls& sc) {
    const ForecastResult f = pl.forecast(sc);
    return f.quant.q[0][f.years.size() - 1][3];
  };
  const double head = p50(h), zero = p50(z), with = p50(n);
  std::printf("    2047 P50: headline %.3f | overlapping loads only %.3f | with the add-on %.3f TWh (+%.1f)\n", head,
              zero, with, with - head);
  check(z.addon_lo == 0 && z.addon_hi == 0 && zero == head,
        "overlapping loads only: zero add-on, headline P50 unchanged");
  check(std::abs(head / 4684.099 - 1) < 0.005, "headline P50 within 0.5% of 4684.1 TWh");
  check(with - head >= added.low && with - head <= added.high,
        "the add-on moves the P50 by an amount inside its range");

  // The repository file: every load scaled from its published year to 2047.
  const std::vector<NewLoad> repo = pl.new_loads();
  bool scaled = !repo.empty();
  for (const NewLoad& l : repo)
    scaled = scaled && near(l.scale, (2047.0 - base) / (l.year - base)) && near(l.low, l.low_published * l.scale) &&
             near(l.high, l.high_published * l.scale);
  check(scaled, "data/external/new_loads.csv: " + std::to_string(repo.size()) + " loads, each scaled to 2047");
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const fs::path root = argc > 1 ? fs::path(argv[1]) : fs::current_path();
    fe_slope_recovers_known();
    fe_slope_skips_unmatched(fs::temp_directory_path() / "e47_test_state_panel");
    const Dataset d = read_dataset(root);
    const std::map<std::string, int> want = expected_regions(root, d);
    const Pipeline pl(root.string(), CoreModel::kIndia);
    states_and_regions(pl, d, want);
    shares(pl, d);
    national_unchanged(pl);
    population_consistent(pl);
    state_table_shares_sum(pl);
    new_loads_excludes_overlap(pl, fs::temp_directory_path() / "e47_test_new_loads");
    std::printf(fails ? "FAILED (%d)\n" : "ok\n", fails);
    return fails ? 1 : 0;
  } catch (const std::exception& e) {
    std::printf("error: %s\n", e.what());
    return 2;
  }
}
