#include "demand_features.hpp"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <set>
#include <stdexcept>

namespace e47 {

namespace {

const char* const kInModel[] = {"core", "driver", "layer", "excluded_rule", "context"};
const char* const kOverlap[] = {"yes", "partial", "no"};

// Layer rows the named scenarios refer to by name (ruling R24).
const char* const kElectrification[] = {"transport_electrification", "green_hydrogen", "data_centres", "space_cooling"};
const char* const kClimate[] = {"space_cooling", "storage_round_trip_losses"};

template <size_t N>
bool one_of(const std::string& v, const char* const (&list)[N]) {
  for (const char* x : list)
    if (v == x) return true;
  return false;
}

template <size_t N>
std::string joined(const char* const (&list)[N]) {
  std::string s;
  for (const char* x : list) s += (s.empty() ? "" : ", ") + std::string(x);
  return s;
}

// Every name in the list must be a layer row: a scenario that names a feature the catalogue
// no longer quantifies fails instead of silently leaving it out.
template <size_t N>
void require_layer(const std::vector<DemandFeature>& features, const char* const (&list)[N], Scenario s) {
  for (const char* name : list) {
    bool found = false;
    for (const DemandFeature& f : features) found = found || (f.feature == name && f.layer());
    if (!found)
      throw std::invalid_argument(std::string("scenario ") + scenario_key(s) + ": '" + name +
                                  "' is not a layer row of the demand-feature catalogue");
  }
}

}  // namespace

std::vector<DemandFeature> read_demand_features(const std::string& csv_path) {
  const CsvTable t = CsvTable::read(csv_path);
  const size_t c_feature = t.col("feature"), c_group = t.col("group"), c_sign = t.col("sign"),
               c_low = t.col("twh_low"), c_mid = t.col("twh_central"), c_high = t.col("twh_high"),
               c_overlap = t.col("overlap_history"), c_in = t.col("in_model"), c_source = t.col("source_name"),
               c_notes = t.col("notes");
  std::vector<DemandFeature> out;
  std::set<std::string> seen;
  for (size_t i = 0; i < t.rows(); ++i) {
    DemandFeature f;
    f.feature = trim(t.str(i, c_feature));
    const std::string where = csv_path + ": " + (f.feature.empty() ? "row " + std::to_string(i + 2) : f.feature);
    if (f.feature.empty()) throw std::runtime_error(where + ": empty feature name");
    if (!seen.insert(f.feature).second) throw std::runtime_error(where + ": feature listed twice");
    f.group = trim(t.str(i, c_group));
    f.overlap = lower(trim(t.str(i, c_overlap)));
    f.in_model = lower(trim(t.str(i, c_in)));
    const std::string sign = trim(t.str(i, c_sign));
    if (sign == "+1" || sign == "1")
      f.sign = 1;
    else if (sign == "-1")
      f.sign = -1;
    else
      throw std::runtime_error(where + ": sign must be +1 or -1 (got '" + sign + "')");
    if (!one_of(f.in_model, kInModel))
      throw std::runtime_error(where + ": in_model must be one of " + joined(kInModel) + " (got '" + f.in_model + "')");
    if (!one_of(f.overlap, kOverlap))
      throw std::runtime_error(where + ": overlap_history must be one of " + joined(kOverlap) + " (got '" +
                               f.overlap + "')");
    // A TWh cell is blank (not quantified) or a number, all of the cell.
    const auto value = [&](size_t c, const char* name) {
      const std::string s = trim(t.str(i, c));
      if (s.empty()) return kNaN;
      char* end = nullptr;
      const double v = std::strtod(s.c_str(), &end);
      if (end != s.c_str() + s.size() || !std::isfinite(v))
        throw std::runtime_error(where + ": " + name + " is not a number ('" + s + "')");
      return v;
    };
    f.low = value(c_low, "twh_low");
    f.central = value(c_mid, "twh_central");
    f.high = value(c_high, "twh_high");
    if (f.layer()) {
      if (!(std::isfinite(f.low) && std::isfinite(f.central) && std::isfinite(f.high)))
        throw std::runtime_error(where + ": a layer row needs twh_low, twh_central and twh_high");
      if (!(0 <= f.low && f.low <= f.central && f.central <= f.high))
        throw std::runtime_error(where + ": TWh values must satisfy 0 <= low <= central <= high");
    } else if (!std::isnan(f.low) || !std::isnan(f.central) || !std::isnan(f.high)) {
      throw std::runtime_error(where + ": TWh values on a row with in_model '" + f.in_model +
                               "' (only layer rows are quantified)");
    }
    f.source_name = trim(t.str(i, c_source));
    f.notes = trim(t.str(i, c_notes));
    out.push_back(std::move(f));
  }
  if (out.empty()) throw std::runtime_error(csv_path + ": no rows");
  return out;
}

FeatureCounts count_features(const std::vector<DemandFeature>& features) {
  FeatureCounts n;
  for (const char* k : kInModel) n.by_in_model[k] = 0;
  for (const DemandFeature& f : features) {
    ++n.total;
    ++n.by_in_model[f.in_model];
    if (!f.layer()) ++n.not_layer_by_group[f.group][f.in_model];
  }
  return n;
}

std::string feature_group_label(const std::string& group) {
  static const std::pair<const char*, const char*> labels[] = {
      {"demographic", "Demographic"},
      {"economic", "Economic"},
      {"supply_side", "Supply side and system"},
      {"distributed", "Distributed and behind-the-meter generation"},
      {"efficiency", "Efficiency"},
      {"electrification", "Electrification and new loads"},
      {"climate", "Climate and weather"},
      {"shocks", "Shocks"},
      {"prices_policy", "Prices and policy"},
      {"history", "Historical trend effects"},
  };
  for (const auto& [k, v] : labels)
    if (group == k) return v;
  std::string s = group;
  for (char& c : s)
    if (c == '_') c = ' ';
  if (!s.empty()) s[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[0])));
  return s;
}

const char* scenario_key(Scenario s) {
  switch (s) {
    case Scenario::kCore: return "core";
    case Scenario::kFeaturesCentral: return "features_central";
    case Scenario::kElectrificationHigh: return "electrification_high";
    case Scenario::kEfficiencyDistributed: return "efficiency_distributed";
    case Scenario::kClimateStress: return "climate_stress";
    case Scenario::kAllUpside: return "all_upside";
    case Scenario::kAllDownside: return "all_downside";
    case Scenario::kJoint: return "joint";
  }
  return "";
}

const char* scenario_name(Scenario s) {
  switch (s) {
    case Scenario::kCore: return "Core (headline)";
    case Scenario::kFeaturesCentral: return "Features central";
    case Scenario::kElectrificationHigh: return "Electrification high";
    case Scenario::kEfficiencyDistributed: return "Efficiency and distributed";
    case Scenario::kClimateStress: return "Climate stress";
    case Scenario::kAllUpside: return "All upside";
    case Scenario::kAllDownside: return "All downside";
    case Scenario::kJoint: return "Joint (Monte Carlo)";
  }
  return "";
}

const char* scenario_description(Scenario s) {
  switch (s) {
    case Scenario::kCore:
      return "The two-dataset forecast with no feature layer: the headline.";
    case Scenario::kFeaturesCentral:
      return "Every quantified feature at its central value; T&D losses, an upper bound that may already be inside "
             "the trend, at 0.";
    case Scenario::kElectrificationHigh:
      return "Transport, green hydrogen, data centres and space cooling at their high values; the rest at central.";
    case Scenario::kEfficiencyDistributed:
      return "Every feature that lowers generation at its high value (T&D losses included, at their upper bound); "
             "the rest at central.";
    case Scenario::kClimateStress:
      return "Space cooling and storage round-trip losses at their high values; the rest at central.";
    case Scenario::kAllUpside:
      return "Every feature that raises generation at its high value and every one that lowers it at its low value "
             "(T&D losses at 0).";
    case Scenario::kAllDownside:
      return "Every feature that raises generation at its low value and every one that lowers it at its high value "
             "(T&D losses at their upper bound).";
    case Scenario::kJoint:
      return "Core draws plus independent feature draws: each feature triangular (low, central, high) times its "
             "sign; T&D losses uniform from 0 to their upper bound.";
  }
  return "";
}

const char* level_name(FeatureLevel l) {
  switch (l) {
    case FeatureLevel::kLow: return "low";
    case FeatureLevel::kCentral: return "central";
    case FeatureLevel::kHigh: return "high";
  }
  return "";
}

double layer_value(const DemandFeature& f, FeatureLevel l) {
  if (!f.layer()) throw std::invalid_argument("layer_value: '" + f.feature + "' is not a layer row");
  switch (l) {
    case FeatureLevel::kLow: return f.layer_low();
    case FeatureLevel::kCentral: return f.layer_central();
    case FeatureLevel::kHigh: return f.layer_high();
  }
  return kNaN;
}

FeatureLevel extreme_level(const DemandFeature& f) {
  const double c = f.layer_central();
  return c - f.layer_low() > f.layer_high() - c ? FeatureLevel::kLow : FeatureLevel::kHigh;
}

ScenarioAdjustment scenario_total(const std::vector<DemandFeature>& features, Scenario s) {
  if (s == Scenario::kJoint)
    throw std::invalid_argument(
        "scenario_total: the joint scenario is a distribution, not a constant adjustment (joint_distribution)");
  ScenarioAdjustment a;
  a.scenario = s;
  if (s == Scenario::kCore) return a;
  if (s == Scenario::kElectrificationHigh) require_layer(features, kElectrification, s);
  if (s == Scenario::kClimateStress) require_layer(features, kClimate, s);
  for (const DemandFeature& f : features) {
    if (!f.layer()) continue;
    FeatureLevel l = FeatureLevel::kCentral;
    switch (s) {
      case Scenario::kElectrificationHigh:
        l = one_of(f.feature, kElectrification) ? FeatureLevel::kHigh : FeatureLevel::kCentral;
        break;
      case Scenario::kEfficiencyDistributed: l = f.sign < 0 ? FeatureLevel::kHigh : FeatureLevel::kCentral; break;
      case Scenario::kClimateStress:
        l = one_of(f.feature, kClimate) ? FeatureLevel::kHigh : FeatureLevel::kCentral;
        break;
      case Scenario::kAllUpside: l = f.sign > 0 ? FeatureLevel::kHigh : FeatureLevel::kLow; break;
      case Scenario::kAllDownside: l = f.sign > 0 ? FeatureLevel::kLow : FeatureLevel::kHigh; break;
      default: break;  // features central
    }
    const double v = layer_value(f, l);
    const double twh = v == 0 ? 0.0 : f.sign * v;  // no -0
    a.parts.push_back({f.feature, l, twh});
    a.total += twh;
  }
  return a;
}

std::vector<FeatureDraw> joint_distribution(const std::vector<DemandFeature>& features) {
  std::vector<FeatureDraw> d;
  for (const DemandFeature& f : features) {
    if (!f.layer()) continue;
    FeatureDraw x;
    x.sign = f.sign;
    if (f.upper_bound_only()) {
      x.kind = kFeatureUniform;
      x.lo = 0;
      x.mode = 0;
      x.hi = f.high;
    } else {
      x.kind = kFeatureTriangular;
      x.lo = f.low;
      x.mode = f.central;
      x.hi = f.high;
    }
    d.push_back(x);
  }
  if (d.size() > static_cast<size_t>(kMaxFeatures))
    throw std::runtime_error("joint_distribution: " + std::to_string(d.size()) + " layer rows, the kernel takes at most " +
                             std::to_string(kMaxFeatures));
  return d;
}

double joint_mean(const std::vector<FeatureDraw>& d) {
  double m = 0;
  for (const FeatureDraw& x : d)
    m += x.sign * (x.kind == kFeatureUniform ? (x.lo + x.hi) / 2 : (x.lo + x.mode + x.hi) / 3);
  return m;
}

std::vector<double> sample_joint_totals(const std::vector<FeatureDraw>& d, uint64_t seed, uint64_t n) {
  const auto in = std::make_unique<McInputs>();  // value-initialised: no add-on, no other inputs used
  pack_features(*in, d);
  std::vector<double> out(static_cast<size_t>(n));
  for (uint64_t i = 0; i < n; ++i) out[static_cast<size_t>(i)] = feature_draw_total<double>(*in, DrawRng(seed, i));
  return out;
}

}  // namespace e47
