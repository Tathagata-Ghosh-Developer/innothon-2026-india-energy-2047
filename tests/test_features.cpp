// Demand-feature layer checks (data/external/demand_features.csv; rulings R22-R24). The
// headline stays the two-dataset forecast; every feature scenario is a labelled layer.
//  1. features_load_and_validate: the catalogue has 44 rows, 11 of them layer rows, with
//     the counts by in_model; a bad row (sign, order, missing or stray TWh, unknown class,
//     repeated name) throws.
//  2. scenario_totals_known: Features central = the sum of sign x central over the layer
//     rows that do not overlap the trend; every named scenario's total follows its rule.
//  3. all_downside_includes_overlap_rows: All downside puts the T&D row (overlap yes) at
//     -twh_high; the central and upside scenarios put it at 0.
//  4. joint_quantiles_bracket: the joint P50 lies between All downside and All upside, and
//     P5 < P50 < P95; the joint feature draws stay inside that envelope and match their
//     mean; a constant adjustment is split across the regions by requirement share.
//  5. headline_unchanged: Core equals the base forecast exactly.
// Usage: test_features <repo_root>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "core/csv.hpp"
#include "pipeline/demand_features.hpp"
#include "pipeline/pipeline.hpp"

using namespace e47;
namespace fs = std::filesystem;

namespace {

int fails = 0;

void check(bool ok, const std::string& what) {
  std::printf("  %-78s %s\n", what.c_str(), ok ? "ok" : "FAIL");
  if (!ok) ++fails;
}

bool near(double a, double b, double tol = 1e-9) { return std::abs(a - b) <= tol * std::max(1.0, std::abs(b)); }

void write_text(const fs::path& file, const std::string& text) {
  fs::create_directories(file.parent_path());
  std::ofstream(file, std::ios::binary) << text;
}

// A layer row of the catalogue read straight from the CSV (not through the module).
struct RawRow {
  std::string feature, overlap;
  int sign = 0;
  double low = 0, central = 0, high = 0;
};

std::vector<RawRow> raw_layer_rows(const fs::path& root) {
  const CsvTable t = CsvTable::read((root / kDemandFeaturesFile).string());
  std::vector<RawRow> v;
  for (size_t i = 0; i < t.rows(); ++i) {
    if (trim(t.str(i, t.col("in_model"))) != "layer") continue;
    RawRow r;
    r.feature = trim(t.str(i, t.col("feature")));
    r.overlap = trim(t.str(i, t.col("overlap_history")));
    r.sign = std::stoi(trim(t.str(i, t.col("sign"))));
    r.low = t.num(i, t.col("twh_low"));
    r.central = t.num(i, t.col("twh_central"));
    r.high = t.num(i, t.col("twh_high"));
    v.push_back(r);
  }
  return v;
}

// A row's unsigned value at level 'l', 'c' or 'h' under R22: a row that overlaps the trend
// is 0 except at its high value.
double raw_value(const RawRow& r, char level) {
  if (r.overlap == "yes") return level == 'h' ? r.high : 0.0;
  return level == 'l' ? r.low : level == 'c' ? r.central : r.high;
}

const char* kHeader =
    "feature,group,mechanism,sign,year_basis,twh_low,twh_central,twh_high,overlap_history,increment_rule,in_model,"
    "source_name,source_url,source_location,accessed,notes\n";
const char* kGoodRows =
    "a_load,electrification,m,+1,FY2047-48,10,20,30,no,all,layer,MNRE,https://mnre.gov.in,p. 1,2026-11-21,n\n"
    "b_context,climate,m,-1,FY2047-48,,,,partial,0,context,MoES,https://moes.gov.in,p. 2,2026-11-21,n\n";

bool reads_with(const fs::path& dir, const std::string& name, const std::string& extra_row) {
  const fs::path file = dir / (name + ".csv");
  write_text(file, std::string(kHeader) + kGoodRows + extra_row);
  try {
    read_demand_features(file.string());
    return true;
  } catch (const std::exception& e) {
    std::printf("    %-22s throws: %s\n", name.c_str(), e.what());
    return false;
  }
}

void features_load_and_validate(const fs::path& root, const fs::path& tmp) {
  std::printf("features_load_and_validate\n");
  const std::vector<DemandFeature> f = read_demand_features((root / kDemandFeaturesFile).string());
  const FeatureCounts n = count_features(f);
  int layer = 0;
  for (const DemandFeature& x : f) layer += x.layer() ? 1 : 0;
  std::printf("    %zu rows:", f.size());
  for (const auto& [k, v] : n.by_in_model) std::printf(" %s %d", k.c_str(), v);
  std::printf("\n");
  check(f.size() == 44 && n.total == 44, "44 rows");
  check(layer == 11 && n.by_in_model.at("layer") == 11, "11 layer rows");
  check(n.by_in_model.at("core") == 7 && n.by_in_model.at("driver") == 3 && n.by_in_model.at("excluded_rule") == 1 &&
            n.by_in_model.at("context") == 22,
        "counts by in_model: core 7, driver 3, excluded_rule 1, context 22");
  int grouped = 0;
  for (const auto& [g, by] : n.not_layer_by_group)
    for (const auto& [k, v] : by) grouped += (k != "layer") ? v : 1000;
  check(grouped == 33, "rows outside the layer counted by group: 33");
  bool layer_ok = true, rest_blank = true;
  for (const DemandFeature& x : f) {
    if (x.layer())
      layer_ok = layer_ok && (x.sign == 1 || x.sign == -1) && x.low >= 0 && x.low <= x.central &&
                 x.central <= x.high && std::isfinite(x.high);
    else
      rest_blank = rest_blank && std::isnan(x.low) && std::isnan(x.central) && std::isnan(x.high);
  }
  check(layer_ok && rest_blank, "layer rows: signed, 0 <= low <= central <= high; other rows not quantified");
  const auto td =
      std::find_if(f.begin(), f.end(), [](const DemandFeature& x) { return x.feature == "t_and_d_losses"; });
  check(td != f.end() && td->upper_bound_only() && td->layer_central() == 0 && td->layer_low() == 0 &&
            td->layer_high() == td->high,
        "t_and_d_losses (overlap yes): an upper bound, layer low and central 0");
  check(feature_label("t_and_d_losses") == "T&D losses" && feature_label("green_hydrogen") == "Green hydrogen" &&
            feature_group_label("supply_side") == "Supply side and system" &&
            feature_group_label("new_group") == "New group",
        "display labels of features and groups");

  const fs::path dir = tmp / "bad_rows";
  fs::remove_all(dir);
  check(reads_with(dir, "good", ""), "a valid file reads");
  check(!reads_with(dir, "sign", "c,economic,m,+2,FY2047-48,1,2,3,no,all,layer,MoP,u,p,d,n\n"), "sign +2 throws");
  check(!reads_with(dir, "sign_blank", "c,economic,m,,FY2047-48,1,2,3,no,all,layer,MoP,u,p,d,n\n"),
        "blank sign throws");
  check(!reads_with(dir, "order", "c,economic,m,-1,FY2047-48,5,2,3,no,all,layer,MoP,u,p,d,n\n"),
        "low > central throws");
  check(!reads_with(dir, "missing_high", "c,economic,m,-1,FY2047-48,1,2,,no,all,layer,MoP,u,p,d,n\n"),
        "layer row without twh_high throws");
  check(!reads_with(dir, "not_number", "c,economic,m,-1,FY2047-48,1,two,3,no,all,layer,MoP,u,p,d,n\n"),
        "non-numeric TWh throws");
  check(!reads_with(dir, "negative", "c,economic,m,-1,FY2047-48,-1,2,3,no,all,layer,MoP,u,p,d,n\n"),
        "negative TWh throws");
  check(!reads_with(dir, "stray_twh", "c,economic,m,-1,FY2047-48,1,2,3,no,all,context,MoP,u,p,d,n\n"),
        "TWh on a context row throws");
  check(!reads_with(dir, "in_model", "c,economic,m,-1,FY2047-48,,,,no,all,maybe,MoP,u,p,d,n\n"),
        "unknown in_model throws");
  check(!reads_with(dir, "overlap", "c,economic,m,-1,FY2047-48,,,,sometimes,all,context,MoP,u,p,d,n\n"),
        "unknown overlap_history throws");
  check(!reads_with(dir, "repeat", "a_load,economic,m,-1,FY2047-48,,,,no,all,context,MoP,u,p,d,n\n"),
        "repeated feature name throws");
  check(!reads_with(dir, "no_name", ",economic,m,-1,FY2047-48,,,,no,all,context,MoP,u,p,d,n\n"),
        "empty feature name throws");
  {
    const fs::path file = dir / "no_column.csv";
    write_text(file, "feature,group,sign\na,b,+1\n");
    bool threw = false;
    try {
      read_demand_features(file.string());
    } catch (const std::exception&) {
      threw = true;
    }
    check(threw, "missing column throws");
  }
}

void scenario_totals_known(const fs::path& root, const std::vector<DemandFeature>& f) {
  std::printf("scenario_totals_known\n");
  const std::vector<RawRow> raw = raw_layer_rows(root);
  // Expected totals from the raw rows, rule by rule (R22, R24).
  const auto total = [&](auto level_of) {
    double s = 0;
    for (const RawRow& r : raw) s += r.sign * raw_value(r, level_of(r));
    return s;
  };
  const auto is = [](const RawRow& r, std::initializer_list<const char*> names) {
    for (const char* n : names)
      if (r.feature == n) return true;
    return false;
  };
  double central_direct = 0;
  for (const RawRow& r : raw)
    if (r.overlap != "yes") central_direct += r.sign * r.central;
  const std::map<Scenario, double> want = {
      {Scenario::kCore, 0.0},
      {Scenario::kFeaturesCentral, total([](const RawRow&) { return 'c'; })},
      {Scenario::kElectrificationHigh,
       total([&](const RawRow& r) {
         return is(r, {"transport_electrification", "green_hydrogen", "data_centres", "space_cooling"}) ? 'h' : 'c';
       })},
      {Scenario::kEfficiencyDistributed, total([](const RawRow& r) { return r.sign < 0 ? 'h' : 'c'; })},
      {Scenario::kClimateStress,
       total([&](const RawRow& r) { return is(r, {"space_cooling", "storage_round_trip_losses"}) ? 'h' : 'c'; })},
      {Scenario::kAllUpside, total([](const RawRow& r) { return r.sign > 0 ? 'h' : 'l'; })},
      {Scenario::kAllDownside, total([](const RawRow& r) { return r.sign > 0 ? 'l' : 'h'; })},
  };
  const ScenarioAdjustment central = scenario_total(f, Scenario::kFeaturesCentral);
  std::printf("    Features central %.3f TWh; sum of sign x central over rows without overlap %.3f TWh\n",
              central.total, central_direct);
  check(near(central.total, central_direct), "Features central = sum of sign x central over non-'yes' layer rows");
  check(central.parts.size() == 11, "Features central: one part per layer row");
  for (const auto& [s, v] : want) {
    const ScenarioAdjustment a = scenario_total(f, s);
    double parts = 0;
    for (const auto& p : a.parts) parts += p.twh;
    std::printf("    %-26s %9.3f TWh (expected %9.3f)\n", scenario_name(s), a.total, v);
    check(near(a.total, v) && near(parts, a.total) && a.scenario == s,
          std::string(scenario_key(s)) + ": total follows its rule, parts add up");
  }
  check(scenario_total(f, Scenario::kCore).parts.empty(), "core: no layer parts");
  bool threw = false;
  try {
    scenario_total(f, Scenario::kJoint);
  } catch (const std::exception&) {
    threw = true;
  }
  check(threw, "joint is not a constant adjustment: scenario_total throws");
  // A scenario that names a feature missing from the layer fails loudly.
  std::vector<DemandFeature> no_cooling;
  for (const DemandFeature& x : f)
    if (x.feature != "space_cooling") no_cooling.push_back(x);
  threw = false;
  try {
    scenario_total(no_cooling, Scenario::kClimateStress);
  } catch (const std::exception&) {
    threw = true;
  }
  check(threw, "a scenario naming a feature that is not a layer row throws");

  // apply_feature_scenario sets only the add-on and the feature draws.
  ScenarioControls c;
  c.g_start_shift = 0.01;
  c.urban_shift_2047 = 0.02;
  apply_feature_scenario(c, f, Scenario::kAllUpside);
  const double up = scenario_total(f, Scenario::kAllUpside).total;
  check(c.addon_lo == up && c.addon_hi == up && c.feature_draws.empty() && c.g_start_shift == 0.01 &&
            c.urban_shift_2047 == 0.02,
        "named scenario: addon_lo = addon_hi = its total, other settings kept");
  apply_feature_scenario(c, f, Scenario::kJoint);
  check(c.addon_lo == 0 && c.addon_hi == 0 && c.feature_draws.size() == 11 && c.g_start_shift == 0.01,
        "joint: no fixed add-on, 11 feature draws, other settings kept");
}

void all_downside_includes_overlap_rows(const std::vector<DemandFeature>& f) {
  std::printf("all_downside_includes_overlap_rows\n");
  const auto td =
      std::find_if(f.begin(), f.end(), [](const DemandFeature& x) { return x.feature == "t_and_d_losses"; });
  if (td == f.end()) {
    check(false, "t_and_d_losses in the catalogue");
    return;
  }
  const auto part = [&](Scenario s) {
    const ScenarioAdjustment a = scenario_total(f, s);
    for (const auto& p : a.parts)
      if (p.feature == "t_and_d_losses") return p;
    return ScenarioAdjustment::Part{"missing", FeatureLevel::kCentral, kNaN};
  };
  const auto down = part(Scenario::kAllDownside), eff = part(Scenario::kEfficiencyDistributed),
             cen = part(Scenario::kFeaturesCentral), up = part(Scenario::kAllUpside);
  std::printf("    T&D: all downside %.1f (%s), efficiency and distributed %.1f, central %.1f, all upside %.1f TWh\n",
              down.twh, level_name(down.level), eff.twh, cen.twh, up.twh);
  check(down.feature == "t_and_d_losses" && down.twh == -td->high && down.level == FeatureLevel::kHigh,
        "all downside: T&D at -twh_high");
  check(eff.twh == -td->high, "efficiency and distributed: T&D at -twh_high");
  check(cen.twh == 0 && up.twh == 0, "features central and all upside: T&D at 0 (upper bound only)");
  // The downside total is the smallest the layer can reach, the upside total the largest.
  double lo = 0, hi = 0;
  for (const DemandFeature& x : f)
    if (x.layer()) {
      lo += x.sign > 0 ? x.layer_low() : -x.layer_high();
      hi += x.sign > 0 ? x.layer_high() : -x.layer_low();
    }
  check(near(scenario_total(f, Scenario::kAllDownside).total, lo) &&
            near(scenario_total(f, Scenario::kAllUpside).total, hi),
        "all downside / all upside = the lowest / highest sum of layer values");
}

struct Run {
  double q[7];  // p05 p10 p25 p50 p75 p90 p95, national, target year
  double region_p50[kRegions];
};

Run run(const Pipeline& pl, const ScenarioControls& c) {
  const ForecastResult r = pl.forecast(c);
  const size_t t = r.years.size() - 1;
  Run x{};
  for (int k = 0; k < 7; ++k) x.q[k] = r.quant.q[0][t][k];
  for (int g = 0; g < kRegions; ++g) x.region_p50[g] = r.quant.q[1 + g][t][3];
  return x;
}

void joint_quantiles_bracket(const Pipeline& pl, const std::vector<DemandFeature>& f) {
  std::printf("joint_quantiles_bracket\n");
  ScenarioControls base;
  base.draws = 1u << 20;
  const auto with = [&](Scenario s) {
    ScenarioControls c = base;
    apply_feature_scenario(c, f, s);
    return c;
  };
  const Run core = run(pl, base), down = run(pl, with(Scenario::kAllDownside)),
            up = run(pl, with(Scenario::kAllUpside)), joint = run(pl, with(Scenario::kJoint)),
            central = run(pl, with(Scenario::kFeaturesCentral));
  std::printf("    2047 P50: core %.1f | all downside %.1f | joint %.1f | all upside %.1f TWh\n", core.q[3], down.q[3],
              joint.q[3], up.q[3]);
  std::printf("    joint P5 %.1f P10 %.1f P50 %.1f P90 %.1f P95 %.1f TWh\n", joint.q[0], joint.q[1], joint.q[3],
              joint.q[5], joint.q[6]);
  check(down.q[3] < joint.q[3] && joint.q[3] < up.q[3], "joint P50 between all downside and all upside");
  check(joint.q[0] < joint.q[3] && joint.q[3] < joint.q[6], "joint P5 < P50 < P95");
  // A constant adjustment moves every draw by the same TWh: the P50 moves by it (histogram bins aside).
  const double dc = scenario_total(f, Scenario::kFeaturesCentral).total;
  check(std::abs(central.q[3] - core.q[3] - dc) < 0.002 * core.q[3],
        "features central moves the P50 by its total (within 0.2% of the P50)");
  // R23: the adjustment is split by each region's share of the requirement.
  double sc = 0, sf = 0, worst = 0;
  for (int g = 0; g < kRegions; ++g) sc += core.region_p50[g], sf += central.region_p50[g];
  for (int g = 0; g < kRegions; ++g)
    worst = std::max(worst, std::abs(core.region_p50[g] / sc - central.region_p50[g] / sf));
  std::printf("    regional median shares, features central vs core: largest gap %.4f\n", worst);
  check(worst < 0.005, "regional shares unchanged by a constant adjustment (within 0.005)");

  // The joint feature draws: inside the envelope, mean as stated, same as the kernel's stream.
  const std::vector<FeatureDraw> d = joint_distribution(f);
  const std::vector<double> s = sample_joint_totals(d, base.seed, 1u << 18);
  const double lo = scenario_total(f, Scenario::kAllDownside).total, hi = scenario_total(f, Scenario::kAllUpside).total;
  double mean = 0, var = 0, mn = 1e300, mx = -1e300;
  for (double v : s) mean += v, mn = std::min(mn, v), mx = std::max(mx, v);
  mean /= s.size();
  for (double v : s) var += (v - mean) * (v - mean);
  const double se = std::sqrt(var / (s.size() - 1) / s.size());
  std::printf("    joint feature total: mean %.2f (stated %.2f, s.e. %.2f), range %.1f to %.1f in [%.1f, %.1f] TWh\n",
              mean, joint_mean(d), se, mn, mx, lo, hi);
  check(d.size() == 11, "joint: one draw per layer row");
  bool td_uniform = false;
  for (size_t k = 0, j = 0; k < f.size(); ++k)
    if (f[k].layer()) {
      if (f[k].feature == "t_and_d_losses")
        td_uniform = d[j].kind == kFeatureUniform && d[j].lo == 0 && d[j].hi == f[k].high && d[j].sign == -1;
      ++j;
    }
  check(td_uniform, "joint: T&D drawn as -uniform(0, twh_high)");
  check(mn >= lo && mx <= hi, "joint feature draws stay inside all downside .. all upside");
  check(std::abs(mean - joint_mean(d)) < 4 * se, "joint feature mean matches the stated mean (4 s.e.)");
}

void headline_unchanged(const Pipeline& pl, const std::vector<DemandFeature>& f, const fs::path& root) {
  std::printf("headline_unchanged\n");
  ScenarioControls base;  // the default controls: the headline
  ScenarioControls core = base;
  apply_feature_scenario(core, f, Scenario::kJoint);
  apply_feature_scenario(core, f, Scenario::kCore);
  check(core.addon_lo == 0 && core.addon_hi == 0 && core.feature_draws.empty(),
        "core (after the joint scenario): no add-on, no feature draws");
  const ForecastResult a = pl.forecast(base), b = pl.forecast(core);
  bool same = a.years == b.years;
  for (int s = 0; s < kSeries && same; ++s)
    for (size_t t = 0; t < a.years.size(); ++t)
      for (int k = 0; k < 8; ++k) same = same && a.quant.q[s][t][k] == b.quant.q[s][t][k];
  const size_t last = a.years.size() - 1;
  const double p50 = b.quant.q[0][last][3];
  std::printf("    core %d P50 %.3f TWh (%llu draws on %s)\n", b.years[last], p50,
              static_cast<unsigned long long>(b.mc.draws), b.mc.device.c_str());
  check(same, "core = base forecast, every series, year and quantile");
  // Base row of outputs/sensitivity_2047.csv at the current build (energy2047, default controls):
  // P50 4,687.78 TWh after the Jammu and Kashmir price-base correction.
  check(std::abs(p50 / 4687.78 - 1) < 0.005, "core P50 within 0.5% of the build's base row, 4,687.78 TWh");
  const fs::path sens = root / "outputs" / "sensitivity_2047.csv";
  if (fs::exists(sens)) {
    const CsvTable t = CsvTable::read(sens.string());
    for (size_t i = 0; i < t.rows(); ++i)
      if (t.str(i, t.col("case")) == "base")
        std::printf("    outputs/sensitivity_2047.csv base P50 %.3f TWh (information)\n", t.num(i, t.col("p50")));
  }
}

}  // namespace

// Runs one test; an exception fails it and the next test still runs.
template <class F>
void guarded(const char* name, F&& f) {
  try {
    f();
  } catch (const std::exception& e) {
    std::printf("  %s: exception: %s FAIL\n", name, e.what());
    ++fails;
  }
}

int main(int argc, char** argv) {
  try {
    const fs::path root = argc > 1 ? fs::path(argv[1]) : fs::current_path();
    guarded("features_load_and_validate",
            [&] { features_load_and_validate(root, fs::temp_directory_path() / "e47_test_features"); });
    const Pipeline pl(root.string(), CoreModel::kIndia);
    std::vector<DemandFeature> f;
    guarded("demand_features", [&] { f = pl.demand_features(); });
    guarded("scenario_totals_known", [&] { scenario_totals_known(root, f); });
    guarded("all_downside_includes_overlap_rows", [&] { all_downside_includes_overlap_rows(f); });
    guarded("joint_quantiles_bracket", [&] { joint_quantiles_bracket(pl, f); });
    guarded("headline_unchanged", [&] { headline_unchanged(pl, f, root); });
    std::printf(fails ? "FAILED (%d)\n" : "ok\n", fails);
    return fails ? 1 : 0;
  } catch (const std::exception& e) {
    std::printf("error: %s\n", e.what());
    return 2;
  }
}
