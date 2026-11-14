// energy2047: fits the forecast models, runs the backtests and writes results.
//
// Usage: energy2047 [repo_root]
// Outputs go to <repo_root>/outputs/.
#include <omp.h>

#include <cstdio>
#include <filesystem>
#include <string>

#include "core/csv.hpp"
#include "core/stats.hpp"
#include "forecast/backtest.hpp"
#include "forecast/models.hpp"
#include "forecast/panel.hpp"
#include "regional/convergence.hpp"

namespace fs = std::filesystem;
using namespace e47;

namespace {

std::string find_wdi(const fs::path& dir) {
  for (const auto& e : fs::directory_iterator(dir)) {
    const std::string n = e.path().filename().string();
    if (n.rfind("API_", 0) == 0) return e.path().string();
  }
  throw std::runtime_error("no API_*.csv in " + dir.string());
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const fs::path root = argc > 1 ? fs::path(argv[1]) : fs::current_path();
    const fs::path wb = root / "data/raw/worldbank/downloads";
    const fs::path out = root / "outputs";
    fs::create_directories(out);

    PanelSources src;
    src.ember_generation_csv = (root / "data/raw/generation/downloads/ember/release_generation_yearly_global.csv").string();
    src.wb_gdp_pc_csv = find_wdi(wb / "NY.GDP.PCAP.PP.KD");
    src.wb_urban_csv = find_wdi(wb / "SP.URB.TOTL.IN.ZS");
    src.wb_population_csv = find_wdi(wb / "SP.POP.TOTL");

    Timer t_load;
    const Panel panel = load_panel(src, PanelFilter{});
    std::printf("panel: %zu countries, %zu observations (%.0f ms)\n", panel.countries.size(), panel.n_obs(),
                t_load.ms());
    const CountrySeries* india = panel.find("IND");
    if (!india) throw std::runtime_error("India missing from panel");
    const Obs& last = india->obs.back();
    std::printf("India %d: %.1f TWh, %.0f kWh/person, GDP pc %.0f, urban %.3f\n", last.year, last.gen_twh,
                last.kwh_per_capita(), last.gdp_pc, last.urban);

    const Design d = make_design(panel);
    const FeQuad q = fit_fe_quad(d);
    const FeGompertz g = fit_fe_gompertz(d);
    const FitStats sq = fit_stats(d, q), sg = fit_stats(d, g);
    std::printf("FE-Quad     b1=%.4f b2=%.4f g=%.4f d=%.4f  within R2=%.3f  elasticity@India=%.3f\n", q.b1, q.b2,
                q.g, q.d, sq.r2_within, q.elasticity(last.gdp_pc));
    std::printf("FE-Gompertz a=%.4f b=%.4f c=%.4f d=%.4f  within R2=%.3f  elasticity@India=%.3f\n", g.a, g.b, g.c,
                g.d, sg.r2_within, g.elasticity(last.gdp_pc));

    // Bootstrap timing: 1 thread vs all threads.
    const int B = 2000;
    const int nthreads = omp_get_max_threads();
    omp_set_num_threads(1);
    Timer t1;
    auto bq1 = bootstrap_fe_gompertz(d, 64, 7);
    const double ms1 = t1.ms() * (static_cast<double>(B) / 64.0);
    omp_set_num_threads(nthreads);
    Timer tn;
    auto bq = bootstrap_fe_quad(d, B, 11);
    auto bg = bootstrap_fe_gompertz(d, B, 7);
    std::printf("bootstrap %d replicates x 2 models: %.0f ms on %d threads (Gompertz 1-thread estimate %.0f ms)\n", B,
                tn.ms(), nthreads, ms1);

    {
      CsvWriter w((out / "bootstrap_params.csv").string());
      w.field("replicate").field("q_b1").field("q_b2").field("q_g").field("q_d");
      w.field("g_a").field("g_b").field("g_c").field("g_d");
      w.endrow();
      for (int r = 0; r < B; ++r) {
        w.field(r).field(bq[r].b1, 8).field(bq[r].b2, 8).field(bq[r].g, 8).field(bq[r].d, 8);
        w.field(bg[r].a, 8).field(bg[r].b, 8).field(bg[r].c, 8).field(bg[r].d, 8);
        w.endrow();
      }
    }

    {
      const ConvergenceResult cr = estimate_convergence(
          (root / "data/raw/competition/renewable_energy_dataset.csv").string(), 2006, 2024, 4000, 99);
      std::printf("state income convergence %d-%d: %d states, slope %.4f, lambda %.4f/yr (se %.4f)\n", cr.year0,
                  cr.year1, cr.n_states, cr.slope, cr.lambda, cr.lambda_se);
    }

    CsvWriter bw((out / "backtest.csv").string());
    bw.field("origin").field("year").field("actual_twh").field("fe_quad_twh").field("fe_gompertz_twh").field(
        "damped_twh");
    bw.endrow();
    for (int origin : {2005, 2010, 2015}) {
      const BacktestResult r = run_backtest(panel, "IND", origin, last.year);
      std::printf("backtest origin %d -> %d: MAPE quad %.1f%% gomp %.1f%% damped %.1f%% | last-year APE %.1f%% %.1f%% %.1f%%\n",
                  origin, last.year, 100 * r.mape[0], 100 * r.mape[1], 100 * r.mape[2], 100 * r.ape_last[0],
                  100 * r.ape_last[1], 100 * r.ape_last[2]);
      for (const auto& p : r.points) {
        bw.field(origin).field(p.year).field(p.actual_twh).field(p.fe_quad_twh).field(p.fe_gompertz_twh).field(
            p.damped_twh);
        bw.endrow();
      }
    }
    return 0;
  } catch (const std::exception& e) {
    std::fprintf(stderr, "error: %s\n", e.what());
    return 1;
  }
}
