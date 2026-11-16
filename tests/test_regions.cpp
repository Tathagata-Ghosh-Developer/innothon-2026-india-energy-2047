// Regional split checks (organisers' ruling of 16 November 2026: the regional split
// uses only the challenge data).
//  1. Every state with a population in the challenge dataset is in the split, mapped
//     to the grid region the NPP state file reports for it, or (for a state NPP does
//     not list) to the region of its dataset Zone.
//  2. State population shares sum to 1 in every projection year.
//  3. In the first projection year the shares equal the dataset's last-year shares.
//  4. The national forecast does not depend on the regional split.
// Usage: test_regions <repo_root>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <map>
#include <set>
#include <string>

#include "core/csv.hpp"
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
// the 2047 national quantiles of the model before the rework (CPU, 2^18 draws,
// default seed); the tolerance allows for Monte Carlo noise only.
void national_unchanged(const Pipeline& pl) {
  std::printf("national 2047 forecast unchanged by the regional split\n");
  ScenarioControls c;
  c.draws = 1u << 18;
  c.use_gpu = false;
  const ForecastResult f = pl.forecast(c);
  const auto& q = f.quant.q[0][f.years.size() - 1];
  std::printf("    %d: P5 %.3f  P50 %.3f  P95 %.3f TWh\n", f.years.back(), q[0], q[3], q[6]);
  const double before[3] = {3214.085, 4699.020, 7002.236};  // P5, P50, P95
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

}  // namespace

int main(int argc, char** argv) {
  try {
    const fs::path root = argc > 1 ? fs::path(argv[1]) : fs::current_path();
    const Dataset d = read_dataset(root);
    const std::map<std::string, int> want = expected_regions(root, d);
    const Pipeline pl(root.string(), CoreModel::kIndia);
    states_and_regions(pl, d, want);
    shares(pl, d);
    national_unchanged(pl);
    population_consistent(pl);
    std::printf(fails ? "FAILED (%d)\n" : "ok\n", fails);
    return fails ? 1 : 0;
  } catch (const std::exception& e) {
    std::printf("error: %s\n", e.what());
    return 2;
  }
}
