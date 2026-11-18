#include "inputs.hpp"

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <map>
#include <set>
#include <stdexcept>

#include "../core/csv.hpp"
#include "../regional/convergence.hpp"  // normalize_state

namespace e47 {

namespace {

constexpr double kHoursTWhPerGW = 8.76;  // 1 GW for a year at 100% = 8.76 TWh

int region_index(const std::string& name) {
  const std::string n = normalize_state(name);
  if (n == "northern" || n == "nr") return 0;
  if (n == "western" || n == "wr") return 1;
  if (n == "southern" || n == "sr") return 2;
  if (n == "eastern" || n == "er") return 3;
  if (n == "north eastern" || n == "northeastern" || n == "ner") return 4;
  return -1;
}

std::map<std::string, int> load_state_regions(const std::string& path) {
  const CsvTable t = CsvTable::read(path);
  std::map<std::string, int> m;
  for (size_t r = 0; r < t.rows(); ++r) {
    const int reg = region_index(t.str(r, t.col("region")));
    if (reg >= 0) m[normalize_state(t.str(r, t.col("state")))] = reg;
  }
  return m;
}

int state_region(const std::map<std::string, int>& m, const std::string& state) {
  const std::string n = normalize_state(state);
  auto it = m.find(n);
  if (it != m.end()) return it->second;
  if (n.find("jammu") != std::string::npos || n.find("ladakh") != std::string::npos)
    return m.at("jammu and kashmir and ladakh");
  return -1;
}

double num_or(const CsvTable& t, size_t r, const char* col, double fallback) {
  if (!t.has(col)) return fallback;
  const double v = t.num(r, t.col(col));
  return std::isnan(v) ? fallback : v;
}

// Central value of a capacity-factor row: value, else midpoint of low and high (fraction).
double cuf_value(const CsvTable& t, size_t r) {
  const double v = num_or(t, r, "value_pct", NAN);
  if (!std::isnan(v)) return v / 100.0;
  const double lo = num_or(t, r, "low_pct", NAN), hi = num_or(t, r, "high_pct", NAN);
  if (!std::isnan(lo) && !std::isnan(hi)) return 0.5 * (lo + hi) / 100.0;
  return NAN;
}

double mean_of(const std::vector<double>& v) {
  if (v.empty()) throw std::runtime_error("mean of empty set");
  double s = 0;
  for (double x : v) s += x;
  return s / static_cast<double>(v.size());
}

std::string strf(const char* f, ...) {
  va_list a, b;
  va_start(a, f);
  va_copy(b, a);
  const int n = std::vsnprintf(nullptr, 0, f, a);
  va_end(a);
  std::string s(static_cast<size_t>(std::max(n, 0)), '\0');
  if (n > 0) std::vsnprintf(s.data(), s.size() + 1, f, b);
  va_end(b);
  return s;
}

// Fiscal year as written in the source files: start year 2023 -> "2023-24".
std::string fiscal_label(int start) { return strf("%d-%02d", start, (start + 1) % 100); }

}  // namespace

const char* tech_key(int s) {
  static const char* k[kTechs] = {"coal", "gas", "nuclear", "hydro", "solar",
                                  "wind", "offshore_wind", "bioenergy", "firmed_re"};
  return k[s];
}

const char* tech_label(int s) {
  static const char* k[kTechs] = {"Coal",          "Natural gas", "Nuclear",   "Hydro", "Solar", "Onshore wind",
                                  "Offshore wind", "Bioenergy",   "Firmed RE"};
  return k[s];
}

bool tech_is_firm(int s) { return s == kCoal || s == kGas || s == kNuclear || s == kHydro || s == kBio || s == kFirmRE; }
bool tech_is_fossil(int s) { return s == kCoal || s == kGas; }
bool tech_is_variable(int s) { return s == kSolar || s == kWind || s == kOffshore; }

AllocationInputs load_allocation_inputs(const std::string& root) {
  AllocationInputs in;
  const std::string gen_dir = root + "/data/raw/generation";
  const std::string ext = root + "/data/external";
  const std::string te = ext + "/techno_economic";
  const auto state_reg = load_state_regions(gen_dir + "/state/state_region_map.csv");

  // 1. Base-year generation by region and source, FY2023-24 (in.base_year is the start year).
  //    Conventional stations: NPP (primary), thermal split into coal and gas with NPP's own
  //    station sums by state. Renewables: CEA General Review by state and type (secondary),
  //    each state in the grid region NPP reports it under.
  {
    const std::string fy = fiscal_label(in.base_year);
    auto at = [&](const char* rel) { return root + "/" + rel; };
    auto& src_notes = in.base_source_notes;

    // 1a. NPP regional generation (CEA 18-column reports, Sub-report 2): thermal, nuclear,
    //     large hydro. The All-India and region "total" rows are subtotals; Bhutan import is
    //     electricity generated in Bhutan, so it is not generation in India.
    std::array<double, kRegions> thermal{};
    double bhutan = 0, conv_total = 0;
    {
      const CsvTable t = CsvTable::read(at("data/raw/npp/npp_region_generation_fy.csv"));
      const size_t cfy = t.col("fiscal_year"), cr = t.col("region"), cs = t.col("source"), cg = t.col("generation_gwh"),
                   csub = t.col("is_subtotal"), cpart = t.col("partial");
      std::set<std::string> seen;
      for (size_t r = 0; r < t.rows(); ++r) {
        if (t.str(r, cfy) != fy || t.str(r, cpart) == "True" || t.str(r, csub) == "True") continue;
        const std::string& name = t.str(r, cr);
        const std::string& src = t.str(r, cs);
        const double twh = t.num(r, cg) / 1000.0;
        if (std::isnan(twh)) continue;
        if (src == "bhutan_import") {
          bhutan += twh;
          continue;
        }
        const int reg = region_index(name);
        if (reg < 0) continue;
        if (!seen.insert(name + "|" + src).second)
          throw std::runtime_error("NPP region file: two " + fy + " rows for " + name + " " + src);
        if (src == "thermal") thermal[reg] += twh;
        else if (src == "nuclear") in.base_twh[reg][kNuclear] += twh;
        else if (src == "hydro") in.base_twh[reg][kHydro] += twh;
        else continue;
        conv_total += twh;
      }
      for (int r = 0; r < kRegions; ++r)
        if (thermal[r] <= 0)
          throw std::runtime_error("no NPP " + fy + " thermal generation for " + std::string(region_name(r)));
    }
    src_notes.push_back(strf(
        "Primary: NPP (CEA 18-column reports, Sub-report 2), FY%s thermal, nuclear and hydro (stations above 25 MW) "
        "generation by grid region, %.1f TWh (data/raw/npp/npp_region_generation_fy.csv). Bhutan import (%.1f TWh) is "
        "left out: it is generated in Bhutan. The All-India rows are subtotals and are not added.",
        fy.c_str(), conv_total, bhutan));

    // 1b. NPP state file (Sub-report 16, station sums by state): the grid region of each
    //     state (the map of the regional split, src/regional/states.cpp) and the coal /
    //     gas split of thermal by region.
    std::map<std::string, int> npp_region;
    std::array<double, kRegions> npp_coal{}, npp_thermal{};
    double npp_diesel = 0;
    {
      const CsvTable t = CsvTable::read(at("data/raw/npp/npp_state_generation_fy.csv"));
      const size_t cfy = t.col("fiscal_year"), cr = t.col("region"), cst = t.col("state"), cs = t.col("source"),
                   cg = t.col("generation_gwh"), cpart = t.col("partial");
      for (size_t r = 0; r < t.rows(); ++r) {
        const int reg = region_index(t.str(r, cr));
        const std::string st = normalize_state(t.str(r, cst));
        if (reg < 0 || st.empty()) continue;
        auto [it, added] = npp_region.emplace(st, reg);
        if (!added && it->second != reg) throw std::runtime_error("NPP state file puts " + st + " in two grid regions");
        if (t.str(r, cfy) != fy || t.str(r, cpart) == "True") continue;
        const std::string& src = t.str(r, cs);
        const double twh = t.num(r, cg) / 1000.0;
        if (std::isnan(twh)) continue;
        if (src == "coal_lignite") npp_coal[reg] += twh;
        if (src == "diesel") npp_diesel += twh;
        if (src == "coal_lignite" || src == "gas" || src == "diesel") npp_thermal[reg] += twh;
      }
    }

    // 1c. Coal and gas shares of thermal by region from NPP's station sums, applied to NPP
    //     regional thermal (the two NPP sub-reports agree to the GWh). Diesel goes with gas:
    //     both are non-coal fossil generation. A region without station rows would take
    //     NPP's national share. CEA's regional split (General Review Table 4.4) is only a
    //     cross-check.
    {
      double nat_coal = 0, nat_thermal = 0, coal_total = 0, gas_total = 0;
      for (int r = 0; r < kRegions; ++r) {
        nat_coal += npp_coal[r];
        nat_thermal += npp_thermal[r];
      }
      std::string fallback;
      for (int r = 0; r < kRegions; ++r) {
        double share;
        if (npp_thermal[r] > 0) {
          share = npp_coal[r] / npp_thermal[r];
        } else if (nat_thermal > 0) {
          share = nat_coal / nat_thermal;
          fallback += (fallback.empty() ? "" : ", ") + std::string(region_name(r));
        } else {
          throw std::runtime_error("no NPP coal and gas split of " + fy + " thermal generation");
        }
        in.base_twh[r][kCoal] = thermal[r] * share;
        in.base_twh[r][kGas] = thermal[r] * (1.0 - share);
        coal_total += in.base_twh[r][kCoal];
        gas_total += in.base_twh[r][kGas];
      }
      src_notes.push_back(strf(
          "Primary: NPP station sums by state (CEA 18-column reports, Sub-report 16, "
          "data/raw/npp/npp_state_generation_fy.csv): coal and gas shares of FY%s thermal generation by region, "
          "applied to NPP regional thermal: coal and lignite %.1f TWh, gas %.1f TWh. Gas includes combined-cycle "
          "stations run on liquid fuel; diesel stations (%.1f TWh) are counted with gas.",
          fy.c_str(), coal_total, gas_total, npp_diesel));
      if (!fallback.empty())
        src_notes.push_back(strf("Primary: no NPP station rows for %s; NPP's national coal share of thermal (%.3f) "
                                 "used there.",
                                 fallback.c_str(), nat_coal / nat_thermal));

      // Cross-check: the CEA General Review regional coal share applied to the same thermal.
      const CsvTable t = CsvTable::read(at("data/raw/generation/region/cea_region_generation_fy.csv"));
      const size_t cfy = t.col("fiscal_year"), cr = t.col("region"), ct = t.col("row_type"), cs = t.col("source"),
                   cg = t.col("generation_gwh");
      std::array<double, kRegions> coal{}, other{};
      for (size_t r = 0; r < t.rows(); ++r) {
        if (t.str(r, cfy) != fy || t.str(r, ct) != "region_total") continue;
        const int reg = region_index(t.str(r, cr));
        const std::string& src = t.str(r, cs);
        const double gwh = t.num(r, cg);
        if (reg < 0 || std::isnan(gwh)) continue;
        if (src == "coal") coal[reg] += gwh;
        if (src == "gas" || src == "oil_diesel") other[reg] += gwh;
      }
      std::string diffs;
      double worst = 0;
      bool any = false;
      for (int r = 0; r < kRegions; ++r) {
        if (coal[r] + other[r] <= 0) continue;
        const double d = thermal[r] * coal[r] / (coal[r] + other[r]) - in.base_twh[r][kCoal];
        diffs += strf("%s%s %+.2f", any ? ", " : "", region_name(r), d);
        worst = std::max(worst, std::abs(d));
        any = true;
      }
      if (any)
        src_notes.push_back(strf(
            "Secondary: cross-check only, not used: CEA General Review 2025, Table 4.4 "
            "(data/raw/generation/region/cea_region_generation_fy.csv) regional coal shares would change coal by "
            "%s TWh (largest %.2f TWh).",
            diffs.c_str(), worst));
    }

    // 1d. Renewables by state and type (CEA General Review, Table 4.7). Solar and wind;
    //     small hydro (25 MW and below) joins hydro; biomass, bagasse and waste to energy
    //     ("other") are bioenergy. A state NPP does not list takes the CEA General Review
    //     grouping; a state with neither is skipped and counted, never guessed.
    {
      const CsvTable t = CsvTable::read(at("data/raw/generation/state/cea_state_res_by_type_fy.csv"));
      const size_t cfy = t.col("fiscal_year"), ct = t.col("row_type"), cst = t.col("state"), cs = t.col("source"),
                   cg = t.col("generation_gwh"), csub = t.col("is_subtotal");
      const std::map<std::string, int> type = {
          {"solar", kSolar}, {"wind", kWind}, {"hydro_small", kHydro}, {"biomass", kBio}, {"other", kBio}};
      std::set<std::string> by_cea, unmatched;
      double cea_twh = 0, skipped_twh = 0, waste = 0;
      std::array<double, kTechs> by_type{};
      for (size_t r = 0; r < t.rows(); ++r) {
        if (t.str(r, cfy) != fy || t.str(r, ct) != "state" || t.str(r, csub) == "True") continue;
        auto k = type.find(t.str(r, cs));
        const double twh = t.num(r, cg) / 1000.0;
        if (k == type.end() || std::isnan(twh)) continue;
        const std::string& st = t.str(r, cst);
        int reg = -1;
        if (auto n = npp_region.find(normalize_state(st)); n != npp_region.end()) {
          reg = n->second;
        } else if ((reg = state_region(state_reg, st)) >= 0) {
          by_cea.insert(st);
          cea_twh += twh;
        }
        if (reg < 0) {
          unmatched.insert(st);
          skipped_twh += twh;
          continue;
        }
        in.base_res_twh[reg][k->second] += twh;
        by_type[k->second] += twh;
        if (k->first == "other") waste += twh;
      }
      in.base_unmatched_states = static_cast<int>(unmatched.size());
      double res_total = 0;
      for (int r = 0; r < kRegions; ++r)
        for (int s = 0; s < kTechs; ++s) {
          in.base_twh[r][s] += in.base_res_twh[r][s];
          res_total += in.base_res_twh[r][s];
        }
      if (res_total <= 0) throw std::runtime_error("no CEA renewable generation by state for " + fy);
      src_notes.push_back(strf(
          "Secondary: CEA General Review 2025, Table 4.7 (data/raw/generation/state/cea_state_res_by_type_fy.csv): "
          "FY%s renewable generation by state and type, %.1f TWh: solar %.1f, wind %.1f, small hydro %.1f (added to "
          "hydro), biomass and bagasse %.1f plus waste to energy %.1f (bioenergy). Central-sector plants count in "
          "their host state.",
          fy.c_str(), res_total, by_type[kSolar], by_type[kWind], by_type[kHydro], by_type[kBio] - waste, waste));
      {
        // NPP's own renewable figure: one all-India total (dashboard chart), no region or type split.
        const CsvTable n = CsvTable::read(at("data/raw/npp/npp_national_generation_fy.csv"));
        double npp_res = kNaN;
        for (size_t r = 0; r < n.rows(); ++r)
          if (n.str(r, n.col("fiscal_year")) == fy) npp_res = n.num(r, n.col("res_gwh")) / 1000.0;
        if (!std::isnan(npp_res))
          src_notes.push_back(strf("Primary: cross-check only: NPP's all-India renewable total for FY%s (dashboard "
                                   "chart, data/raw/npp/npp_national_generation_fy.csv) is %.1f TWh, %.1f TWh below "
                                   "CEA's; NPP gives no region or type split, so CEA's is used.",
                                   fy.c_str(), npp_res, res_total - npp_res));
      }
      src_notes.push_back(
          "Primary: grid region of each renewable-table state from the NPP state file "
          "(data/raw/npp/npp_state_generation_fy.csv), the map of the regional split.");
      std::string list;
      for (const std::string& s : by_cea) list += (list.empty() ? "" : ", ") + s;
      if (!by_cea.empty())
        src_notes.push_back(strf("Secondary: %s (no NPP stations; %.2f TWh) take the CEA General Review grouping "
                                 "(data/raw/generation/state/state_region_map.csv).",
                                 list.c_str(), cea_twh));
      list.clear();
      for (const std::string& s : unmatched) list += (list.empty() ? "" : ", ") + s;
      std::string skipped = strf("Secondary: renewable-table states with no grid region, skipped: %d", in.base_unmatched_states);
      if (!unmatched.empty()) skipped += strf(" (%s; %.2f TWh)", list.c_str(), skipped_twh);
      src_notes.push_back(skipped + ".");
    }

    // 1e. Cross-check only: Ember regional data (secondary), same fiscal year and the
    //     calendar year in which it ends.
    {
      double base_total = 0;
      std::array<double, kRegions> base_r{};
      for (int r = 0; r < kRegions; ++r)
        for (int s = 0; s < kTechs; ++s) {
          base_r[r] += in.base_twh[r][s];
          base_total += in.base_twh[r][s];
        }
      // Ember total generation (every source) by region for one period.
      auto ember = [&](const char* rel, const char* key, const std::string& value) {
        std::array<double, kRegions> reg{};
        const CsvTable t = CsvTable::read(at(rel));
        const size_t ck = t.col(key), cr = t.col("region"), cg = t.col("generation_gwh"), csub = t.col("is_subtotal");
        for (size_t r = 0; r < t.rows(); ++r) {
          if (t.str(r, ck) != value || t.str(r, csub) == "True") continue;
          const int k = region_index(t.str(r, cr));
          const double twh = t.num(r, cg) / 1000.0;
          if (k >= 0 && !std::isnan(twh)) reg[k] += twh;
        }
        return reg;
      };
      const auto fy_r = ember("data/raw/generation/region/ember_region_generation_fy.csv", "fiscal_year", fy);
      const auto cy_r = ember("data/raw/generation/region/ember_region_generation_cy.csv", "year",
                              std::to_string(in.base_year + 1));
      double ember_fy = 0, ember_cy = 0;
      for (int r = 0; r < kRegions; ++r) {
        ember_fy += fy_r[r];
        ember_cy += cy_r[r];
      }
      std::string by_region;
      for (int r = 0; r < kRegions; ++r)
        by_region += strf("%s%s %+.1f%%", r ? ", " : "", region_name(r),
                          fy_r[r] > 0 ? 100.0 * (base_r[r] / fy_r[r] - 1.0) : 0.0);
      src_notes.push_back(strf("Secondary: cross-check only, not used: Ember India state data aggregated to regions, "
                               "utilities. FY%s %.1f TWh against %.1f TWh here (%+.1f%%; by region %s); CY%d %.1f TWh.",
                               fy.c_str(), ember_fy, base_total,
                               ember_fy > 0 ? 100.0 * (base_total / ember_fy - 1.0) : 0.0, by_region.c_str(),
                               in.base_year + 1, ember_cy));
    }
    in.notes.push_back("Base generation FY" + fy +
                       ": NPP regional conventional generation, thermal split into coal and gas with NPP station "
                       "sums (primary), plus CEA renewable generation by state (secondary); see the base-mix "
                       "provenance lines.");
  }

  // 1b. National generation history: build-rate evidence for 2047 upper bounds.
  {
    const CsvTable t = CsvTable::read(gen_dir + "/national/ember_national_generation_cy.csv");
    const std::map<std::string, int> src = {{"coal", kCoal},       {"gas", kGas},     {"nuclear", kNuclear},
                                            {"hydro", kHydro},     {"solar", kSolar}, {"wind", kWind},
                                            {"biomass", kBio}};
    std::map<int, std::array<double, kTechs>> by_year;
    for (size_t r = 0; r < t.rows(); ++r) {
      if (t.str(r, t.col("is_subtotal")) == "True") continue;
      auto it = src.find(t.str(r, t.col("source")));
      if (it == src.end()) continue;
      by_year[static_cast<int>(t.num(r, t.col("year")))][it->second] += t.num(r, t.col("generation_gwh")) / 1000.0;
    }
    in.rate_last_year = by_year.rbegin()->first;
    in.national_last_twh = by_year.rbegin()->second;
    for (int y = 2010; y <= in.rate_last_year; ++y) {
      if (!by_year.count(y) || !by_year.count(y - 1)) continue;
      for (int s = 0; s < kTechs; ++s)
        in.max_annual_increase[s] = std::max(in.max_annual_increase[s], by_year[y][s] - by_year[y - 1][s]);
    }
    in.notes.push_back("Build-rate evidence: largest year-on-year generation increase by source, 2010-2025, Ember national.");
  }

  // 2. Capacity factors (fractions).
  std::array<double, kRegions> cuf_solar{}, cuf_wind{}, cuf_off{}, cuf_hydro{};
  double cuf_bio = 0.6;
  {
    const CsvTable t = CsvTable::read(te + "/capacity_factors.csv");
    for (size_t r = 0; r < t.rows(); ++r) {
      const std::string tech = t.str(r, t.col("source_tech")), lvl = t.str(r, t.col("geography_level"));
      const std::string metric = t.str(r, t.col("metric"));
      const int reg = region_index(t.str(r, t.col("region")));
      const double v = cuf_value(t, r);
      if (std::isnan(v)) continue;
      if (lvl == "region" && reg >= 0) {
        if (tech == "solar" && metric == "planning_cuf_new") cuf_solar[reg] = v;
        if (tech == "wind_onshore" && metric == "planning_cuf") cuf_wind[reg] = v;
        if (tech == "wind_offshore" && metric == "planning_cuf") cuf_off[reg] = v;
        if (tech == "hydro_all" && metric == "observed_cuf") cuf_hydro[reg] = v;
      }
      if (tech == "biomass" && metric == "availability_assumption") cuf_bio = v;
    }
    // Regions without a published planning CUF take the lowest published regional value.
    // No regional value at all means the selection matched nothing: fail loudly.
    auto fill = [](std::array<double, kRegions>& a, const char* what) {
      double mn = 2;
      for (double v : a)
        if (v > 0) mn = std::min(mn, v);
      if (mn > 1) throw std::runtime_error(std::string("no regional capacity factor read for ") + what);
      for (double& v : a)
        if (v <= 0) v = mn;
    };
    fill(cuf_solar, "solar");
    fill(cuf_wind, "onshore wind");
    fill(cuf_hydro, "hydro");
    if (cuf_off[1] <= 0 || cuf_off[2] <= 0) throw std::runtime_error("no offshore wind capacity factor read");
    in.notes.push_back("Solar/wind CUF: CEA NEP 2022-32 regional planning values; missing regions use the lowest published regional value.");
  }

  // 3. Resource potential converted to generation potential (TWh/yr).
  {
    const CsvTable t = CsvTable::read(ext + "/resource_potential.csv");
    const size_t cst = t.col("state"), cres = t.col("resource"), cmet = t.col("metric"), cval = t.col("value");
    std::set<std::string> has_ladakh;  // (resource|metric) reported with Ladakh separately
    for (size_t r = 0; r < t.rows(); ++r)
      if (normalize_state(t.str(r, cst)) == "ladakh") has_ladakh.insert(t.str(r, cres) + "|" + t.str(r, cmet));
    for (size_t r = 0; r < t.rows(); ++r) {
      const std::string st = t.str(r, cst), res = t.str(r, cres), met = t.str(r, cmet);
      if (st.find("incl. Ladakh") != std::string::npos && has_ladakh.count(res + "|" + met)) continue;
      const int reg = state_region(state_reg, st);
      const double v = t.num(r, cval);
      if (reg < 0 || std::isnan(v)) continue;
      if (res == "solar" && met == "ground_mounted_potential_nise_2025")
        in.potential_twh[reg][kSolar] += v * cuf_solar[reg] * kHoursTWhPerGW;
      else if (res == "wind_150m" && met == "potential")
        in.potential_twh[reg][kWind] += v * cuf_wind[reg] * kHoursTWhPerGW;
      else if (res == "large_hydro" && met == "identified_capacity_reassessment_2017_23_mw")
        in.potential_twh[reg][kHydro] += v / 1000.0 * cuf_hydro[reg] * kHoursTWhPerGW;
      else if (res == "small_hydro" && met == "potential")
        in.potential_twh[reg][kHydro] += v / 1000.0 * cuf_hydro[reg] * kHoursTWhPerGW;
      else if ((res == "biomass" && met == "biomass_power_potential") || (res == "bagasse_cogeneration" && met == "potential"))
        in.potential_twh[reg][kBio] += v / 1000.0 * cuf_bio * kHoursTWhPerGW;
    }
    in.notes.push_back("Solar potential: NISE 2025 ground-mounted (GW) x regional CUF x 8.76.");
    in.notes.push_back("Wind potential: NIWE 150 m (GW) x regional CUF x 8.76.");
    in.notes.push_back("Hydro potential: CEA reassessment 2017-23 identified large hydro plus MNRE small hydro (MW) x observed regional hydro CUF.");
    in.notes.push_back("Bioenergy potential: MNRE biomass and bagasse cogeneration (MW) x CEA biomass availability.");
  }

  // 4. Offshore wind (initial NIWE assessment, by state).
  {
    const CsvTable t = CsvTable::read(te + "/offshore_wind.csv");
    for (size_t r = 0; r < t.rows(); ++r) {
      const std::string item = t.str(r, t.col("item")), unit = t.str(r, t.col("unit"));
      if (item.rfind("potential (initial NIWE", 0) != 0 || unit != "GW") continue;
      const int reg = state_region(state_reg, t.str(r, t.col("state")));
      if (reg < 0) continue;
      in.potential_twh[reg][kOffshore] += t.num(r, t.col("value")) * cuf_off[reg] * kHoursTWhPerGW;
    }
    in.notes.push_back("Offshore wind potential: NIWE initial assessment (Gujarat, Tamil Nadu) x CEA offshore CUF.");
  }

  // 5. Costs, USD/MWh in 2047.
  {
    const CsvTable t = CsvTable::read(te + "/lcoe.csv");
    std::map<std::string, std::map<int, double>> iea;
    std::vector<double> coal_tariff, hydro_tariff, bio_tariff, rtc_tariff, solar_avg;
    for (size_t r = 0; r < t.rows(); ++r) {
      const std::string tech = t.str(r, t.col("technology")), metric = t.str(r, t.col("cost_metric"));
      const std::string proj = t.str(r, t.col("project_or_auction"));
      const double v = t.num(r, t.col("value_central"));
      const double yr = t.num(r, t.col("year"));
      if (std::isnan(v)) continue;
      if (metric == "lcoe_projection" && !std::isnan(yr)) iea[tech][static_cast<int>(yr)] = v;
      if (tech == "coal" && metric == "discovered_tariff") coal_tariff.push_back(v);
      if (tech == "large_hydro" && metric == "levelised_tariff") hydro_tariff.push_back(v);
      if (tech == "biomass" && metric == "regulated_tariff") bio_tariff.push_back(v);
      if (tech == "fdre_rtc" && metric == "discovered_tariff" && proj.find("RTC") != std::string::npos)
        rtc_tariff.push_back(v);
      if (tech == "solar" && metric == "discovered_tariff" && proj.rfind("Average", 0) == 0) solar_avg.push_back(v);
    }
    auto at2047 = [&](const std::string& tech) {
      const auto& m = iea.at(tech);
      const double a = m.at(2035), b = m.at(2050);
      return a + (b - a) * (2047 - 2035) / (2050 - 2035.0);
    };
    const double coal = at2047("coal");
    const double inr_coal = mean_of(coal_tariff);
    const double c[kTechs] = {
        coal,
        at2047("gas"),
        at2047("nuclear"),
        mean_of(hydro_tariff) / inr_coal * coal,
        at2047("solar"),
        at2047("wind_onshore"),
        at2047("wind_offshore"),
        mean_of(bio_tariff) / inr_coal * coal,
        at2047("solar") + (mean_of(rtc_tariff) - mean_of(solar_avg)) / inr_coal * coal,
    };
    double mean_solar = 0, mean_wind = 0;
    for (int r = 0; r < kRegions; ++r) {
      mean_solar += cuf_solar[r] / kRegions;
      mean_wind += cuf_wind[r] / kRegions;
    }
    for (int r = 0; r < kRegions; ++r)
      for (int s = 0; s < kTechs; ++s) {
        double v = c[s];
        if (s == kSolar) v *= mean_solar / cuf_solar[r];
        if (s == kWind) v *= mean_wind / cuf_wind[r];
        in.cost[r][s] = v;
      }
    in.notes.push_back("Costs: IEA WEO 2025 India LCOE projections interpolated to 2047 (coal, gas, nuclear, solar, wind).");
    in.notes.push_back("Hydro and bioenergy: Indian tariff ratio to new-coal tariffs (2025 bids) applied to the 2047 coal cost.");
    in.notes.push_back("Firmed RE: 2047 solar cost plus the current round-the-clock RE premium over plain solar, held constant in real terms.");
    in.notes.push_back("Solar and wind cost scaled by regional CUF relative to the mean regional CUF.");
  }

  // 6. Emission factors (tCO2/MWh), CEA CO2 Baseline Database.
  {
    const CsvTable t = CsvTable::read(te + "/emission_factors.csv");
    for (size_t r = 0; r < t.rows(); ++r) {
      if (t.str(r, t.col("period")) != "2024-25") continue;
      if (t.str(r, t.col("metric")).rfind("station_weighted_avg", 0) != 0) continue;
      const std::string f = t.str(r, t.col("fuel"));
      const double v = t.num(r, t.col("value"));
      if (f == "coal") in.emission_t_per_mwh[kCoal] = v;
      if (f == "gas") in.emission_t_per_mwh[kGas] = v;
    }
  }

  // 7. Nuclear reactor units by region (built, under construction, sanctioned, planned).
  {
    const CsvTable t = CsvTable::read(te + "/nuclear_pipeline.csv");
    for (size_t r = 0; r < t.rows(); ++r) {
      const int reg = region_index(t.str(r, t.col("region")));
      const double u = t.num(r, t.col("units"));
      if (reg >= 0 && !std::isnan(u)) in.nuclear_units[reg] += u;
    }
    for (int r = 0; r < kRegions; ++r)
      if (in.base_twh[r][kNuclear] > 0) in.nuclear_units[r] += 1.0;  // existing stations
    in.notes.push_back("Nuclear siting key: reactor units in the NPCIL/DAE pipeline by region, plus existing stations.");
  }

  // 8. Inter-regional corridors: transfer capability planned for March 2032 and regional losses.
  {
    const CsvTable t = CsvTable::read(te + "/transmission.csv");
    std::array<double, kRegions> loss{};
    for (size_t r = 0; r < t.rows(); ++r) {
      const std::string metric = t.str(r, t.col("metric")), asof = t.str(r, t.col("as_of"));
      const int a = region_index(t.str(r, t.col("region_a"))), b = region_index(t.str(r, t.col("region_b")));
      if (metric == "regional_transmission_loss" && a >= 0) loss[a] = t.num(r, t.col("value")) / 100.0;
      if (metric.rfind("inter_regional_transfer_capa", 0) == 0 && asof == "2032-03-31" && a >= 0 && b >= 0)
        in.corridors.push_back(Corridor{a, b, t.num(r, t.col("value")), 0, t.str(r, t.col("corridor"))});
    }
    for (auto& c : in.corridors) c.loss = 0.5 * (loss[c.a] + loss[c.b]);
    in.notes.push_back("Corridors: CEA/NEP Vol II inter-regional transfer capability planned for 31.03.2032; loss = mean of the two regional loss rates.");
  }
  return in;
}

}  // namespace e47
