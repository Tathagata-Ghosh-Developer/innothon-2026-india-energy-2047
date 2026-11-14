#include "inputs.hpp"

#include <cmath>
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

  // 1. Base-year generation by region and source (Ember, utilities).
  {
    const CsvTable t = CsvTable::read(gen_dir + "/region/ember_region_generation_cy.csv");
    const size_t cy = t.col("year"), cr = t.col("region"), cs = t.col("source"), cg = t.col("generation_gwh"),
                 csub = t.col("is_subtotal");
    const std::map<std::string, int> src = {{"coal", kCoal},   {"lignite", kCoal}, {"gas", kGas},
                                            {"nuclear", kNuclear}, {"hydro", kHydro}, {"solar", kSolar},
                                            {"wind", kWind},   {"biomass", kBio}};
    for (size_t r = 0; r < t.rows(); ++r) {
      if (static_cast<int>(t.num(r, cy)) != in.base_year || t.str(r, csub) == "True") continue;
      const int reg = region_index(t.str(r, cr));
      auto it = src.find(t.str(r, cs));
      if (reg < 0 || it == src.end()) continue;
      in.base_twh[reg][it->second] += t.num(r, cg) / 1000.0;
    }
    in.notes.push_back("Base generation: Ember India state data aggregated to regions, CY2024, utilities only.");
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
        if (tech == "hydro" && metric == "observed_cuf") cuf_hydro[reg] = v;
      }
      if (tech == "biomass" && metric == "availability_assumption") cuf_bio = v;
    }
    // Regions without a published planning CUF take the lowest published regional value.
    auto fill = [](std::array<double, kRegions>& a) {
      double mn = 1;
      for (double v : a)
        if (v > 0) mn = std::min(mn, v);
      for (double& v : a)
        if (v <= 0) v = mn;
    };
    fill(cuf_solar);
    fill(cuf_wind);
    fill(cuf_hydro);
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
