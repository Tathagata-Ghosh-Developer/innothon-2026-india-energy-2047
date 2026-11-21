// India-only core checks.
//  1. Loader: a state whose GSDP is copied unchanged from the previous year (a
//     placeholder, not data) must not dilute national income growth.
//  2. Forecast: the two holdout years (FY2024-25, FY2025-26), never used in
//     fitting, must fall inside the model's 90 percent band.
//  3. Elasticity controls: a higher income elasticity gives a higher 2047 median,
//     and each estimation window on its own gives a usable forecast.
//  4. Growth decomposition: population, income, urbanisation and model error add
//     up to the log growth of the central path from the base year to 2047.
// Usage: test_india <repo_root>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "core/csv.hpp"
#include "forecast/india.hpp"
#include "pipeline/pipeline.hpp"

using namespace e47;
namespace fs = std::filesystem;

namespace {

int fails = 0;

void check(bool ok, const char* what) {
  std::printf("  %-70s %s\n", what, ok ? "ok" : "FAIL");
  if (!ok) ++fails;
}

void stale_rows_are_ignored(const fs::path& tmp) {
  std::printf("stale GSDP rows\n");
  fs::create_directories(tmp);
  const fs::path npp = tmp / "npp.csv", org = tmp / "org.csv", urb = tmp / "urb.csv";
  {
    std::ofstream f(npp);
    f << "fiscal_year,fy_start_year,thermal_gwh,hydro_gwh,nuclear_gwh,res_gwh,total_gwh\n";
    for (int y = 2008; y <= 2010; ++y) f << y << "-" << (y + 1) % 100 << "," << y << ",900,50,25,25,1000\n";
  }
  {
    // State A grows 10% a year; state B (same size) repeats its GSDP: a placeholder.
    std::ofstream f(org);
    f << "State,Year,Population,GSDP (crore INR)\n";
    for (int y = 2008; y <= 2010; ++y) {
      f << "A," << y << ",1000000," << 100.0 * std::pow(1.1, y - 2008) << "\n";
      f << "B," << y << ",1000000,100\n";
    }
  }
  {
    std::ofstream f(urb);
    f << "state,year,value,source\n";
    for (int y : {2005, 2010, 2015}) f << "India," << y << ",30,World Urbanization Prospects 2018\n";
  }
  const IndiaSeries s = load_india_series(npp.string(), org.string(), urb.string(), 2008, 2010);
  check(s.gpc_growth.size() == 2, "two growth years");
  const double want = std::log(1.1);
  check(s.gpc_growth.size() == 2 && std::abs(s.gpc_growth[0] - want) < 1e-9 && std::abs(s.gpc_growth[1] - want) < 1e-9,
        "growth comes from the state with real data (ln 1.1 a year)");
}

void holdout_inside_band(const std::string& root) {
  std::printf("holdout years inside the 90%% band\n");
  const Pipeline pl(root, CoreModel::kIndia);
  ScenarioControls c;
  c.draws = 1u << 18;
  c.use_gpu = false;
  const ForecastResult f = pl.forecast(c);
  const IndiaFit& fi = pl.fitted().india_fit;
  std::printf("    residual: base %+.4f, s.d. %.4f, rho %.2f | beta %.3f | income growth %.4f (s.d. %.4f, rho %.2f)\n",
              fi.resid.back(), fi.resid_sd, fi.resid_rho, fi.beta, pl.drivers().g_hist_mean, pl.drivers().g_hist_sd,
              pl.drivers().g_hist_rho);

  // Holdout totals: NPP conventional generation + NPP RES extended at its FY2018-23 growth.
  std::map<int, double> conv;
  const CsvTable t = CsvTable::read((fs::path(root) / "data/raw/npp/npp_region_generation_fy.csv").string());
  for (size_t i = 0; i < t.rows(); ++i) {
    if (t.str(i, t.col("region")) != "All India" || t.str(i, t.col("partial")) == "True") continue;
    const std::string src = t.str(i, t.col("source"));
    if (src == "thermal" || src == "nuclear" || src == "hydro")
      conv[static_cast<int>(t.num(i, t.col("fy_start_year")))] += t.num(i, t.col("generation_gwh")) / 1000.0;
  }
  const auto& obs = pl.fitted().india.obs;
  const IndiaObs& o23 = obs.back();
  const IndiaObs& o18 = obs[obs.size() - 6];
  const double g = std::pow(o23.res_twh / o18.res_twh, 1.0 / (o23.year - o18.year));
  for (int k = 0; k < 2; ++k) {
    const int y = f.years[k];
    const double total = conv.at(y) + o23.res_twh * std::pow(g, y - o23.year);
    const auto& q = f.quant.q[0][k];
    std::printf("    FY%d: holdout %.0f TWh, model P5 %.0f P50 %.0f P95 %.0f\n", y, total, q[0], q[3], q[6]);
    char what[96];
    std::snprintf(what, sizeof what, "FY%d holdout inside P5-P95", y);
    check(total >= q[0] && total <= q[6], what);
  }
}

void elasticity_shift_monotone(const std::string& root) {
  std::printf("elasticity controls\n");
  const Pipeline pl(root, CoreModel::kIndia);
  auto median_2047 = [&](double shift, int window) {
    ScenarioControls c;
    c.draws = 1u << 14;
    c.use_gpu = false;
    c.beta_shift = shift;
    c.beta_window = window;
    const ForecastResult f = pl.forecast(c);
    return f.quant.q[0][f.years.size() - 1][3];
  };
  const double lo = median_2047(-0.063, 0), mid = median_2047(0, 0), hi = median_2047(0.063, 0);
  const double s1 = median_2047(0, 1), s3 = median_2047(0, 2);
  std::printf("    2047 P50: beta -0.063 %.0f | base %.0f | beta +0.063 %.0f | S1 only %.0f | S3 only %.0f TWh\n", lo, mid,
              hi, s1, s3);
  check(lo < mid && mid < hi, "median rises with beta_shift (-0.063 < 0 < +0.063)");
  check(std::isfinite(s1) && std::isfinite(s3), "S1-only and S3-only windows give finite medians");
  check(s3 >= s1, "S3-only median >= S1-only median (S3 beta 0.872 > S1 0.835)");

  // Window selection: slot 4 of a replicate is beta (no urban term, no decline), so the
  // mean over the packed array must equal the mean of that window's half.
  const auto& bb = pl.fitted().india_beta_boot;
  const size_t h1 = bb.size() / 2;
  auto half_mean = [&](size_t from, size_t to) {
    double m = 0;
    for (size_t k = from; k < to; ++k) m += bb[k];
    return m / static_cast<double>(to - from);
  };
  auto slot4_mean = [&](int window) {
    ScenarioControls c;
    c.beta_window = window;
    const std::vector<float> boot = pl.boot_for(c);
    const size_t n = boot.size() / kBootStride;
    double m = 0;
    for (size_t k = 0; k < n; ++k) m += boot[k * kBootStride + 4];
    return m / static_cast<double>(n);
  };
  check(std::abs(slot4_mean(1) - half_mean(0, h1)) < 1e-5, "beta_window 1 uses the S1 replicates only");
  check(std::abs(slot4_mean(2) - half_mean(h1, bb.size())) < 1e-5, "beta_window 2 uses the S3 replicates only");
}

void decomposition_adds_up(const std::string& root) {
  std::printf("growth decomposition of the central path\n");
  const Pipeline pl(root, CoreModel::kIndia);
  for (const double decline : {0.0, 0.2}) {
    ScenarioControls c;
    c.draws = 1u << 12;
    c.use_gpu = false;
    c.elasticity_decline = decline;
    const ForecastResult f = pl.forecast(c);
    const int target = f.years.back();
    const GrowthDecomposition d = pl.decompose_central(c, target);
    const double sum = d.population + d.income + d.urbanisation + d.model_error;
    const double total_path = std::log(f.central[0].back() / d.gen_base_twh);
    std::printf("    decline %.1f, FY%d %.1f -> FY%d %.1f TWh: total %.4f = population %.4f + income %.4f + "
                "urbanisation %.4f + model error %.4f\n",
                decline, d.base_year, d.gen_base_twh, d.target_year, d.gen_target_twh, d.total, d.population, d.income,
                d.urbanisation, d.model_error);
    char what[96];
    std::snprintf(what, sizeof what, "decline %.1f: parts add up to the total (1e-9)", decline);
    check(std::abs(sum - d.total) < 1e-9, what);
    std::snprintf(what, sizeof what, "decline %.1f: total = ln(central %d / base) of forecast() (1e-6)", decline, target);
    check(std::abs(total_path - d.total) < 1e-6, what);
    std::snprintf(what, sizeof what, "decline %.1f: population > 0 and income > 0", decline);
    check(d.population > 0 && d.income > 0, what);
    std::snprintf(what, sizeof what, "decline %.1f: base and target years", decline);
    check(d.base_year == pl.drivers().base_year && d.target_year == target, what);
  }

  // The decomposition is India-core only: the cross-country core (few replicates, it
  // only needs the central path) fills the total and population, the rest stays NaN.
  const Pipeline cc(root, CoreModel::kCrossCountry, 20);
  ScenarioControls c;
  c.draws = 1u << 12;
  c.use_gpu = false;
  const ForecastResult f = cc.forecast(c);
  const GrowthDecomposition d = cc.decompose_central(c, f.years.back());
  check(std::abs(std::log(f.central[0].back() / d.gen_base_twh) - d.total) < 1e-6 && d.population > 0,
        "cross-country core: total and population filled");
  check(std::isnan(d.income) && std::isnan(d.urbanisation) && std::isnan(d.model_error),
        "cross-country core: income, urbanisation and model error NaN");
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const std::string root = argc > 1 ? argv[1] : ".";
    stale_rows_are_ignored(fs::temp_directory_path() / "e47_test_india");
    holdout_inside_band(root);
    elasticity_shift_monotone(root);
    decomposition_adds_up(root);
    std::printf(fails ? "FAILED (%d)\n" : "ok\n", fails);
    return fails ? 1 : 0;
  } catch (const std::exception& e) {
    std::printf("error: %s\n", e.what());
    return 2;
  }
}
