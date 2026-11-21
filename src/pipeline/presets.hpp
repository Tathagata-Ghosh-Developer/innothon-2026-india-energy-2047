#pragma once
// Named scenario and policy presets shared by the dashboard and the command-line tool.
#include <string>
#include <vector>

#include "pipeline.hpp"

namespace e47 {

struct ScenarioPreset {
  std::string name, description;
  void (*apply)(ScenarioControls&);
};

struct PolicyPreset {
  std::string name, description;
  void (*apply)(PolicySettings&);
};

inline const std::vector<ScenarioPreset>& scenario_presets() {
  static const std::vector<ScenarioPreset> v = {
      {"Base", "India's historical income growth per person to 2030, slowing by 1.5 points by 2047; no new loads.",
       [](ScenarioControls&) {}},
      {"High growth",
       "Income growth per person 1.5 points above history to 2030, slowing by 1 point; urban share 3 points above the "
       "UN path by 2047; 100-300 TWh of new loads (green hydrogen, data centres).",
       [](ScenarioControls& c) {
         c.g_start_shift = 0.015;
         c.g_slowdown = 0.010;
         c.urban_shift_2047 = 0.03;
         c.addon_lo = 100;
         c.addon_hi = 300;
       }},
      {"Low growth",
       "Income growth per person 1.5 points below history to 2030, slowing by 2 points; urban share 2 points below the "
       "UN path by 2047.",
       [](ScenarioControls& c) {
         c.g_start_shift = -0.015;
         c.g_slowdown = 0.020;
         c.urban_shift_2047 = -0.02;
       }},
  };
  return v;
}

// Scenario selector built from data rather than fixed values: the named demand-feature
// scenarios (Pipeline::demand_features, apply_feature_scenario), each on top of the other
// settings. It needs a loaded pipeline, so it is not in scenario_presets(); it stays
// outside the headline forecast.
inline constexpr const char* kFeaturesPresetName = "Demand features (official, outside headline)";

inline const std::vector<PolicyPreset>& policy_presets() {
  static const std::vector<PolicyPreset> v = {
      {"Base", "Default settings (docs/assumptions_and_uncertainty.md).", [](PolicySettings&) {}},
      {"Low carbon",
       "80% non-fossil, coal floor 35% of FY2023-24 coal, carbon price 50 USD/t, 30% usable solar and wind land, corridors 3x the "
       "2032 plan, nuclear up to 15%.",
       [](PolicySettings& p) {
         p.nonfossil_min = 0.80;
         p.coal_floor = 0.35;
         p.carbon_price = 50;
         p.solar_land = 0.30;
         p.wind_land = 0.30;
         p.transfer_scale = 3.0;
         p.nuclear_max = 0.15;
       }},
      {"Robust",
       "Meets the median requirement with margin: 75% non-fossil, 35% usable solar and 30% wind land (incl. rooftop "
       "and agri-PV), corridors 3x the 2032 plan, nuclear up to 15%, build rates at the published high-case limits, "
       "coal floor 50%, and the existing gas fleet kept at its best NPP year's output (FY2018-19 to FY2025-26) for "
       "peaking and as a bridge for variable renewables, each region within its FY2023-24 share of gas. Meeting the "
       "P90 also needs 45% of the solar potential usable; the published high-case build rates are then enough "
       "(outputs/p90_levers.csv).",
       [](PolicySettings& p) {
         p.nonfossil_min = 0.75;
         p.solar_land = 0.35;
         p.wind_land = 0.30;
         p.transfer_scale = 3.0;
         p.nuclear_max = 0.15;
         p.build_case = kBuildHigh;
         p.coal_floor = 0.50;
         p.gas_min_twh_share = 1.0;
       }},
      {"Energy security",
       "Each region 40% self-firm, national firm 55%, corridors only at the 2032 plan, gas at most 2%, coal floor 70%.",
       [](PolicySettings& p) {
         p.firm_regional = 0.40;
         p.firm_national = 0.55;
         p.transfer_scale = 1.0;
         p.gas_max = 0.02;
         p.coal_floor = 0.70;
       }},
  };
  return v;
}

}  // namespace e47
