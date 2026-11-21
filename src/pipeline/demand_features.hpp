#pragma once
// Demand-feature layer: every factor that can change India's FY2047-48 utility generation
// requirement (data/external/demand_features.csv, official sources, all secondary; see
// data/external/SOURCES.md section 11). The headline forecast stays the two-dataset
// estimate; each feature scenario is a labelled layer on top of it, never the headline.
//
// Rows with in_model = layer carry a signed TWh adjustment to the FY2047-48 requirement,
// already cut to the part beyond the historical trend and already on a generation basis.
// How a row enters the layer (ruling R22):
//   overlap_history partial or no: applied as given. Central uses twh_central, low and high
//     are the range ends; in the joint Monte Carlo the row is sign x triangular(low,
//     central, high).
//   overlap_history yes (only t_and_d_losses today): an upper bound that may already be
//     inside the trend. Central contributes 0; low counts as 0 too; the downside scenarios
//     use twh_high; in the joint Monte Carlo the row is sign x uniform(0, twh_high).
// Regions (ruling R23): every adjustment is split across the grid regions in proportion
// to each region's share of the requirement (the Monte Carlo kernel splits the add-on with
// the rest of the requirement). Regional siting of a feature is context only: no official
// regional split exists.
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "../core/csv.hpp"  // kNaN
#include "../forecast/mc.hpp"  // FeatureDraw

namespace e47 {

// The catalogue, relative to the repository root.
constexpr const char* kDemandFeaturesFile = "data/external/demand_features.csv";

struct DemandFeature {
  std::string feature, group;
  std::string overlap;   // overlap_history: yes, partial or no
  std::string in_model;  // core, driver, layer, excluded_rule or context
  int sign = 0;          // +1 raises utility generation, -1 lowers it
  // TWh in FY2047-48, generation basis, as positive numbers (the layer applies sign x value).
  // Finite for layer rows, NaN for every other row (not quantified).
  double low = kNaN, central = kNaN, high = kNaN;
  std::string source_name, notes;

  bool layer() const { return in_model == "layer"; }
  // Overlap "yes": the value is an upper bound that may already be inside the trend.
  bool upper_bound_only() const { return overlap == "yes"; }
  // The range the layer uses (R22): low and central are 0 for an upper-bound-only row.
  double layer_low() const { return upper_bound_only() ? 0.0 : low; }
  double layer_central() const { return upper_bound_only() ? 0.0 : central; }
  double layer_high() const { return high; }
};

// Reads and validates the catalogue. Throws (naming the file and the row) on: a missing
// column, an empty or repeated feature name, a sign other than +1 or -1, an in_model or
// overlap_history value outside the lists above, a layer row without finite TWh values
// with 0 <= low <= central <= high, and TWh values on a row that is not a layer row.
std::vector<DemandFeature> read_demand_features(const std::string& csv_path);

// Rows by in_model, and the rows that are not in the layer (context and the others) by group.
struct FeatureCounts {
  int total = 0;
  std::map<std::string, int> by_in_model;
  std::map<std::string, std::map<std::string, int>> not_layer_by_group;  // group -> in_model -> rows
};
FeatureCounts count_features(const std::vector<DemandFeature>& features);

// Display label of a catalogue group ("supply_side" -> "Supply side and system").
std::string feature_group_label(const std::string& group);
// Display label of a feature ("t_and_d_losses" -> "T&D losses").
std::string feature_label(const std::string& feature);

// Named feature scenarios (ruling R24). Each named scenario except kJoint is the core
// forecast plus a constant TWh adjustment, ramped linearly from the base year to the
// target year as the kernel ramps its add-on.
//   kCore                  no layer: the headline.
//   kFeaturesCentral       every layer row at central (upper-bound-only rows at 0).
//   kElectrificationHigh   transport, green hydrogen, data centres and space cooling at
//                          high; the rest at central.
//   kEfficiencyDistributed every -1 row at high (T&D included); the rest at central.
//   kClimateStress         space cooling and storage round-trip losses at high; the rest
//                          at central.
//   kAllUpside             every +1 row at high, every -1 row at low (upper-bound-only at 0).
//   kAllDownside           every +1 row at low, every -1 row at high (T&D at high).
//   kJoint                 Monte Carlo: core draws plus independent feature draws.
enum class Scenario : int {
  kCore = 0,
  kFeaturesCentral,
  kElectrificationHigh,
  kEfficiencyDistributed,
  kClimateStress,
  kAllUpside,
  kAllDownside,
  kJoint
};
constexpr int kScenarioCount = 8;
const char* scenario_key(Scenario s);          // "core", "features_central", ...
const char* scenario_name(Scenario s);         // "Core (headline)", "Features central", ...
const char* scenario_description(Scenario s);  // one sentence

enum class FeatureLevel : int { kLow = 0, kCentral = 1, kHigh = 2 };
const char* level_name(FeatureLevel l);  // "low", "central", "high"

// The layer value of a row at a level, unsigned (layer_low, layer_central or layer_high).
double layer_value(const DemandFeature& f, FeatureLevel l);
// The end of a layer row's range farthest from its layer central value (ties: high): the
// "extreme" of the one-at-a-time rows.
FeatureLevel extreme_level(const DemandFeature& f);

// Adjustment of a named scenario: one part per layer row (none for kCore), each the row's
// signed layer value at the scenario's level, and their total. Throws for kJoint (not a
// constant adjustment) and when a feature the scenario names is not a layer row.
struct ScenarioAdjustment {
  struct Part {
    std::string feature;
    FeatureLevel level = FeatureLevel::kCentral;
    double twh = 0;  // signed TWh in FY2047-48
  };
  Scenario scenario = Scenario::kCore;
  double total = 0;  // signed TWh in FY2047-48
  std::vector<Part> parts;
};
ScenarioAdjustment scenario_total(const std::vector<DemandFeature>& features, Scenario s);

// The joint distribution: one independent draw per layer row, in catalogue order; sign x
// triangular(low, central, high), or sign x uniform(0, high) for an upper-bound-only row.
std::vector<FeatureDraw> joint_distribution(const std::vector<DemandFeature>& features);
// Mean of the sum of the draws (triangular (lo + mode + hi) / 3, uniform (lo + hi) / 2).
double joint_mean(const std::vector<FeatureDraw>& d);
// n samples of the sum of the draws, TWh in FY2047-48: sample i is the feature total the
// Monte Carlo kernel adds in its draw i under `seed` (the same random stream and function).
std::vector<double> sample_joint_totals(const std::vector<FeatureDraw>& d, uint64_t seed, uint64_t n);

}  // namespace e47
