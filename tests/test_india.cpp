// India-only core checks.
//  1. Loader: a state whose GSDP is copied unchanged from the previous year (a
//     placeholder, not data) must not dilute national income growth.
//  2. Forecast: the two holdout years (FY2024-25, FY2025-26), never used in
//     fitting, must fall inside the model's 90 percent band.
// Usage: test_india <repo_root>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>

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

}  // namespace

int main(int argc, char** argv) {
  try {
    const std::string root = argc > 1 ? argv[1] : ".";
    stale_rows_are_ignored(fs::temp_directory_path() / "e47_test_india");
    holdout_inside_band(root);
    std::printf(fails ? "FAILED (%d)\n" : "ok\n", fails);
    return fails ? 1 : 0;
  } catch (const std::exception& e) {
    std::printf("error: %s\n", e.what());
    return 2;
  }
}
