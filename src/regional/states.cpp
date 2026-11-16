#include "states.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "../core/csv.hpp"
#include "../forecast/mc.hpp"  // kRegions, region_name
#include "convergence.hpp"     // normalize_state

namespace e47 {

namespace {

int region_index(const std::string& name) {
  for (int r = 0; r < kRegions; ++r)
    if (name == region_name(r)) return r;
  return -1;
}

// Dataset Zone -> grid region, used only for states the NPP state file does not list.
// Central (Madhya Pradesh, Chhattisgarh) belongs to the Western grid.
int zone_region(const std::string& zone) {
  static const std::map<std::string, std::string> m = {{"North", "Northern"},          {"South", "Southern"},
                                                       {"East", "Eastern"},            {"West", "Western"},
                                                       {"North-East", "North-Eastern"}, {"Central", "Western"}};
  auto it = m.find(zone);
  return it == m.end() ? -1 : region_index(it->second);
}

}  // namespace

StatePopulation load_state_population(const std::string& organiser_csv, const std::string& npp_state_csv) {
  // 1. The grid region of every state NPP reports generation for.
  std::map<std::string, int> npp;
  {
    const CsvTable t = CsvTable::read(npp_state_csv);
    const size_t cr = t.col("region"), cs = t.col("state");
    for (size_t i = 0; i < t.rows(); ++i) {
      const int r = region_index(trim(t.str(i, cr)));
      const std::string s = normalize_state(t.str(i, cs));
      if (r < 0 || s.empty()) continue;  // "All India" and subtotal rows
      auto [it, added] = npp.emplace(s, r);
      if (!added && it->second != r) throw std::runtime_error("NPP state file puts " + s + " in two grid regions");
    }
  }

  // 2. Population and Zone by state (challenge dataset).
  StatePopulation sp;
  std::map<std::string, StateRecord> by_state;
  {
    const CsvTable t = CsvTable::read(organiser_csv);
    const size_t cs = t.col("State"), cy = t.col("Year"), cp = t.col("Population"), cz = t.col("Zone");
    for (size_t i = 0; i < t.rows(); ++i) {
      const double y = t.num(i, cy), p = t.num(i, cp);
      if (std::isnan(y) || std::isnan(p) || p <= 0) continue;
      const std::string s = normalize_state(t.str(i, cs));
      StateRecord& rec = by_state[s];
      rec.state = s;
      const std::string z = trim(t.str(i, cz));
      if (!z.empty()) rec.zone = z;
      rec.pop[static_cast<int>(y)] = p;
      sp.last_year = std::max(sp.last_year, static_cast<int>(y));
    }
  }
  if (by_state.empty()) throw std::runtime_error("challenge dataset has no state population");

  // 3. Grid region: NPP's own, else the dataset Zone. A state with a population in the
  //    last year but no region would leave people out of the split, so it stops the run.
  std::vector<std::string> unmapped;
  for (auto& [s, rec] : by_state) {
    const int zr = zone_region(rec.zone);
    auto it = npp.find(s);
    if (it != npp.end()) {
      rec.region = it->second;
      rec.region_from = "NPP";
      if (zr >= 0 && zr != rec.region)
        sp.notes.push_back(s + ": NPP region " + region_name(rec.region) + " used (dataset zone " + rec.zone +
                           " would give " + region_name(zr) + ")");
    } else if (zr >= 0) {
      rec.region = zr;
      rec.region_from = "dataset zone";
      sp.notes.push_back(s + ": not in the NPP state file; dataset zone " + rec.zone + " -> " + region_name(zr));
    } else {
      sp.notes.push_back(s + ": unmapped (not in the NPP state file; dataset zone '" + rec.zone + "' unknown)");
    }
    if (!rec.pop.count(sp.last_year)) {
      sp.notes.push_back(s + ": no population for " + std::to_string(sp.last_year) + "; left out of the split");
      continue;
    }
    if (rec.region < 0) {
      unmapped.push_back(s);
      continue;
    }
    sp.states.push_back(rec);
  }
  if (!unmapped.empty()) {
    std::string list;
    for (const std::string& s : unmapped) list += (list.empty() ? "" : ", ") + s;
    throw std::runtime_error("states with a population but no grid region: " + list);
  }
  for (const auto& [s, r] : npp)
    if (!by_state.count(s))
      sp.notes.push_back(s + ": in the NPP state file but has no population in the challenge dataset; not in the split");
  return sp;
}

double state_population_growth(const StatePopulation& sp, const std::string& state, int from, int to) {
  double p0 = 0, p1 = 0;
  for (const StateRecord& rec : sp.states) {
    if (!state.empty() && rec.state != state) continue;
    auto a = rec.pop.find(from), b = rec.pop.find(to);
    if (a == rec.pop.end() || b == rec.pop.end()) continue;  // India: states reported in both years only
    p0 += a->second;
    p1 += b->second;
  }
  if (to == from || p0 <= 0 || p1 <= 0) return kNaN;
  return std::log(p1 / p0) / (to - from);
}

}  // namespace e47
