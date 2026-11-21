// Price base checks (src/core/price_base.hpp) and their use in the model.
//  1. fisher_known: the Fisher relation on the CERC pre-tax WACC at the RBI target.
//  2. deflate_known: the MoSPI GDP deflator (2011-12 series) from FY2021-22 to FY2024-25.
//  3. inr_to_usd_known: RBI reference-rate averages, calendar and fiscal year.
//  4. beyond_data_uses_target: a year past the published series is extended at the RBI
//     CPI target (India) or the FOMC goal (US), and the extension is noted.
//  4b. nominal_outlook_known: real 2024 USD in nominal 2047 INR and USD at the inflation
//     goals (information only).
//  5. jk_deflated: the challenge dataset's Jammu and Kashmir GSDP (current prices from
//     2012) deflated to constant 2011-12 prices; the loader then sees real growth only.
//  5b. readers_apply_basis: on a synthetic dataset with one state at current prices,
//     estimate_convergence and state_panel_data with the basis give what they give on the
//     same dataset at constant prices; with GsdpPriceBasis::none() they do not.
//  6. default_plan_unchanged_by_price_base: the IEA costs are already real 2024 USD, so the
//     default plan at a fixed requirement is the plan of commit a314521 to the last digit.
// Usage: test_price_base <repo_root>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

#include "allocation/allocate.hpp"
#include "core/csv.hpp"
#include "core/price_base.hpp"
#include "forecast/india.hpp"
#include "forecast/state_panel.hpp"
#include "pipeline/pipeline.hpp"
#include "regional/convergence.hpp"

using namespace e47;
namespace fs = std::filesystem;

namespace {

int fails = 0;

void check(bool ok, const std::string& what) {
  std::printf("  %-86s %s\n", what.c_str(), ok ? "ok" : "FAIL");
  if (!ok) ++fails;
}

template <class F>
bool throws(F f) {
  try {
    f();
  } catch (const std::exception&) {
    return true;
  }
  return false;
}

void fisher_known() {
  std::printf("fisher_known\n");
  const double r = real_rate(0.12585, 0.04);
  std::printf("    real_rate(12.585%%, 4%%) = %.8f\n", r);
  check(std::abs(r - 0.08255) < 5e-6 && std::abs(r - (1.12585 / 1.04 - 1.0)) < 1e-15,
        "(1.12585 / 1.04) - 1 = 0.08255: CERC pre-tax WACC made real at the RBI target");
  check(std::abs(real_rate(0.07, 0.0) - 0.07) < 1e-15, "zero inflation leaves the rate unchanged");
  bool inverse = true;
  for (double pi : {0.02, 0.06})
    inverse &= std::abs((1.0 + real_rate(0.12585, pi)) * (1.0 + pi) - 1.12585) < 1e-15;
  check(inverse, "(1 + real) x (1 + inflation) = 1 + nominal at 2% and 6%");
  check(throws([] { return real_rate(0.1, -1.0); }), "inflation of -100% or below: error");
}

void deflate_known(const PriceBase& pb) {
  std::printf("deflate_known\n");
  const double v = pb.deflate_inr(157.087, "2021-22", "2024-25");
  const double ratio = pb.deflate_inr(1.0, "2021-22", "2024-25");
  std::printf("    157.087 (FY2021-22) -> %.6f (FY2024-25); ratio %.6f\n", v, ratio);
  check(std::abs(v - 175.923) < 1e-9, "GDP deflator: 157.087 in FY2021-22 is 175.923 in FY2024-25");
  check(std::abs(ratio - 1.11991) < 5e-6, "FY2021-22 -> FY2024-25 ratio 1.11991");
  check(std::abs(pb.deflate_inr(1.0, "2021-22", "2024-25", PriceBase::Index::kWpi) - 154.9 / 139.4) < 1e-12,
        "WPI (2011-12 = 100): 139.4 -> 154.9");
  check(pb.deflate_inr(5.0, "2024-25", "2024-25") == 5.0, "same fiscal year: unchanged");
  check(std::abs(pb.deflate_inr(pb.deflate_inr(3.0, "2012-13", "2023-24"), "2023-24", "2012-13") - 3.0) < 1e-12,
        "there and back: unchanged");
  check(std::abs(pb.usd_rebase(110.159, 2021, 2024) - 125.587) < 1e-9, "US GDP deflator: 110.159 (2021) -> 125.587 (2024)");
  check(throws([&] { return pb.deflate_inr(1.0, "2021-23", "2024-25"); }), "malformed fiscal year: error");
  check(throws([&] { return pb.deflate_inr(1.0, "2009-10", "2024-25"); }), "before the published series: error");
}

void inr_to_usd_known(const PriceBase& pb) {
  std::printf("inr_to_usd_known\n");
  std::printf("    Rs 1000 = %.6f USD at the calendar-2024 rate %.4f\n", pb.inr_to_usd(1000.0, "2024"),
              pb.inr_per_usd("2024"));
  check(pb.inr_per_usd("2024") == 83.6693, "calendar 2024: Rs 83.6693 per USD (RBI)");
  check(std::abs(pb.inr_to_usd(1000.0, "2024") - 1000.0 / 83.6693) < 1e-12, "Rs 1000 / 83.6693 per USD");
  check(pb.inr_per_usd("2021-22") == 74.5039 && std::abs(pb.inr_to_usd(74.5039, "2021-22") - 1.0) < 1e-15,
        "FY2021-22: Rs 74.5039 per USD (RBI fiscal-year average)");
  check(throws([&] { return pb.inr_to_usd(1.0, "2019"); }) && throws([&] { return pb.inr_to_usd(1.0, "2030-31"); }),
        "a period with no published rate: error (rates are never extended)");
}

void beyond_data_uses_target(const PriceBase& pb) {
  std::printf("beyond_data_uses_target\n");
  check(pb.inflation_target() == 0.04, "RBI CPI target read from the file: 4%");
  check(pb.us_inflation_goal() == 0.02, "FOMC longer-run goal read from the file: 2%");
  std::string note;
  const double v = pb.deflate_inr(100.0, "2025-26", "2026-27", PriceBase::Index::kGdpDeflator, &note);
  std::printf("    100 (FY2025-26) -> %.9f (FY2026-27); note: %s\n", v, note.c_str());
  check(std::abs(v - 104.0) < 1e-9, "FY2026-27 = FY2025-26 x 1.04 (last published year x the target)");
  check(!note.empty() && note.find("target") != std::string::npos, "the extension is noted");
  const double two = pb.deflate_inr(100.0, "2024-25", "2027-28");
  check(std::abs(two - 100.0 * 176.890 * 1.04 * 1.04 / 175.923) < 1e-9, "two years past the series: x 1.04 twice");
  std::string none;
  pb.deflate_inr(1.0, "2021-22", "2024-25", PriceBase::Index::kGdpDeflator, &none);
  check(none.empty(), "inside the published series: no note");
  std::string us;
  const double u = pb.usd_rebase(100.0, 2025, 2027, &us);
  check(std::abs(u - 100.0 * 1.02 * 1.02) < 1e-9 && !us.empty(), "US deflator past 2025: x 1.02 a year, noted");
}

// Information only: a real 2024 USD amount in nominal 2047 money at the inflation goals.
void nominal_outlook_known(const PriceBase& pb) {
  std::printf("nominal_outlook_known\n");
  const double inr = pb.nominal_inr_from_real_usd(1.0, 2047), usd = pb.nominal_usd_from_real_usd(1.0, 2047);
  std::printf("    1 USD (real 2024) = Rs %.4f or USD %.6f in 2047 money\n", inr, usd);
  check(std::abs(inr - 83.6693 * std::pow(1.04, 23)) < 1e-9, "INR: x 83.6693 (RBI, 2024) x 1.04^23 (RBI target, 2024 to 2047)");
  check(std::abs(usd - std::pow(1.02, 23)) < 1e-12, "USD: x 1.02^23 (FOMC goal, 2024 to 2047)");
  check(pb.nominal_usd_from_real_usd(5.0, 2024) == 5.0, "2024: unchanged");
  check(throws([&] { return pb.nominal_inr_from_real_usd(1.0, 2023); }), "a year before 2024: error");
}

void jk_deflated(const std::string& root, const PriceBase& pb, const fs::path& tmp) {
  std::printf("jk_deflated\n");
  const GsdpPriceBasis basis = GsdpPriceBasis::load(root + "/data/external/macro/gsdp_price_basis.csv", pb);
  check(basis.is_current("Jammu And Kashmir", 2012) && basis.is_current("jammu and kashmir", 2024) &&
            !basis.is_current("Jammu And Kashmir", 2011) && !basis.is_current("Maharashtra", 2016),
        "listed: Jammu and Kashmir from 2012; not 2011, not another state");
  // MoSPI GSDP, 2011-12 series, Jammu & Kashmir FY2016-17: current 124,848, constant 100,203
  // (data/external/macro/SOURCES.md, section 7). The dataset's 2016 value is 124,848.
  const double v = basis.constant_2011_12("Jammu And Kashmir", 2016, 124848.0);
  std::printf("    2016: 124,848 at current prices -> %.1f at 2011-12 prices (MoSPI constant 100,203, %+.2f%%)\n", v,
              100.0 * (v / 100203.0 - 1.0));
  check(std::abs(v / 100203.0 - 1.0) < 0.01, "2016 within 1% of the MoSPI constant-price value 100,203");
  check(std::abs(v - 124848.0 / 1.25052) < 1e-6, "divided by the national GDP deflator of FY2016-17 (125.052 / 100)");
  check(basis.constant_2011_12("Jammu And Kashmir", 2011, 78256.0) == 78256.0, "2011 (base year): unchanged");
  check(basis.constant_2011_12("Maharashtra", 2016, 1.0) == 1.0, "a state not listed: unchanged");

  // Loader: state A is at constant prices, B at current prices (listed), both growing 5% a
  // year in real terms. After the correction the national growth is the real 5% only.
  fs::create_directories(tmp);
  const fs::path npp = tmp / "npp.csv", org = tmp / "org.csv", urb = tmp / "urb.csv", lst = tmp / "basis.csv";
  {
    std::ofstream f(npp);
    f << "fiscal_year,fy_start_year,thermal_gwh,hydro_gwh,nuclear_gwh,res_gwh,total_gwh\n";
    for (int y = 2011; y <= 2014; ++y) f << y << "-" << (y + 1) % 100 << "," << y << ",900,50,25,25,1000\n";
  }
  {
    std::ofstream f(org);
    f << "State,Year,Population,GSDP (crore INR)\n";
    f.precision(17);
    for (int y = 2011; y <= 2014; ++y) {
      const std::string fy = std::to_string(y) + "-" + (((y + 1) % 100) < 10 ? "0" : "") + std::to_string((y + 1) % 100);
      f << "A," << y << ",1000000," << 100.0 * std::pow(1.05, y - 2011) << "\n";
      f << "B," << y << ",1000000," << 100.0 * std::pow(1.05, y - 2011) * pb.deflate_inr(1.0, "2011-12", fy) << "\n";
    }
  }
  {
    std::ofstream f(urb);
    f << "state,year,value,source\n";
    for (int y : {2010, 2015}) f << "India," << y << ",30,World Urbanization Prospects 2018\n";
  }
  {
    std::ofstream f(lst);
    f << "state,from_year,basis,note\nB,2012,current_prices,test row\n";
  }
  const GsdpPriceBasis b2 = GsdpPriceBasis::load(lst.string(), pb), none = GsdpPriceBasis::none();
  const IndiaSeries raw = load_india_series(npp.string(), org.string(), urb.string(), 2011, 2014, none);
  const IndiaSeries fixed = load_india_series(npp.string(), org.string(), urb.string(), 2011, 2014, b2);
  GsdpOptions drop;
  drop.exclude_states = {"b"};
  const IndiaSeries only_a = load_india_series(npp.string(), org.string(), urb.string(), 2011, 2014, none, drop);
  bool real = fixed.gpc_growth.size() == 3, inflated = raw.gpc_growth.size() == 3, a_only = only_a.gpc_growth.size() == 3;
  for (size_t k = 0; k < 3 && real && inflated && a_only; ++k) {
    real &= std::abs(fixed.gpc_growth[k] - std::log(1.05)) < 1e-12;
    inflated &= raw.gpc_growth[k] > std::log(1.05) + 1e-3;
    a_only &= std::abs(only_a.gpc_growth[k] - std::log(1.05)) < 1e-12;
  }
  std::printf("    loader growth 2012: raw %.5f, corrected %.5f, B left out %.5f (ln 1.05 = %.5f)\n",
              raw.gpc_growth.empty() ? 0.0 : raw.gpc_growth[0], fixed.gpc_growth.empty() ? 0.0 : fixed.gpc_growth[0],
              only_a.gpc_growth.empty() ? 0.0 : only_a.gpc_growth[0], std::log(1.05));
  check(inflated, "uncorrected: the current-price state adds inflation to income growth");
  check(real, "corrected: income growth is the real 5% a year");
  check(fixed.gsdp_current_corrected == 3 && raw.gsdp_current_corrected == 0, "three state-years deflated (2012-2014)");
  check(a_only, "a state left out of income growth: growth from the others only");

  // The model: every Jammu and Kashmir year from 2012 to 2024 is deflated.
  const Pipeline pl(root, CoreModel::kIndia);
  std::printf("    pipeline: %d state-years deflated\n", pl.fitted().india.gsdp_current_corrected);
  check(pl.fitted().india.gsdp_current_corrected == 13, "pipeline: Jammu and Kashmir 2012-2024 (13 state-years) deflated");
}

// Synthetic repository: six states, 2006-2024, with real income per person growing at
// different rates. In one copy every GSDP is at constant 2011-12 prices; in the other, Beta's
// GSDP from 2012 is at current prices (x the GDP deflator), as Jammu and Kashmir's is in the
// challenge dataset, and a basis file lists it. Both GSDP readers given the basis must see
// the constant-price data exactly; given none(), they must see the inflation.
void write_synthetic_repo(const fs::path& root, const PriceBase& pb, bool beta_current) {
  const char* name[] = {"Alpha", "Beta", "Gamma", "Delta", "Epsilon", "Zeta"};
  const char* region[] = {"Northern", "Southern", "Eastern", "Western", "Northern", "Southern"};
  const auto pop = [](int s, int y) { return 1e6 * (s + 1) * (1.0 + 0.01 * (y - 2006)); };
  const auto ln_y = [](int s, int y) { return 3.0 + 0.3 * s + (0.06 - 0.005 * s) * (y - 2006); };  // thousand INR
  fs::create_directories(root / "data/raw/competition");
  fs::create_directories(root / "data/raw/npp");
  {
    std::ofstream f(root / "data/raw/competition/renewable_energy_dataset.csv", std::ios::binary);
    f.precision(17);
    f << "State,Year,Zone,Population,GSDP (crore INR)\n";
    for (int s = 0; s < 6; ++s)
      for (int y = 2006; y <= 2024; ++y) {
        double gsdp = std::exp(ln_y(s, y)) * 1e3 * pop(s, y) / 1e7;  // crore INR at 2011-12 prices
        if (beta_current && s == 1 && y >= 2012) gsdp *= pb.inr_index(y) / pb.inr_index(2011);
        f << name[s] << "," << y << ",North," << pop(s, y) << "," << gsdp << "\n";
      }
  }
  {
    std::ofstream f(root / "data/raw/npp/npp_state_generation_fy.csv", std::ios::binary);
    f.precision(17);
    f << "fiscal_year,fy_start_year,region,state,source,generation_gwh,is_subtotal,partial\n";
    for (int s = 0; s < 6; ++s)
      for (int y = 2018; y <= 2023; ++y)  // ln kWh per person = 0.7 ln income per person + a state effect
        f << y << "-" << (y + 1) % 100 << "," << y << "," << region[s] << "," << name[s] << ",coal_lignite,"
          << std::exp(1.0 + 0.1 * s + 0.7 * ln_y(s, y)) * pop(s, y) / 1e6 << ",False,False\n";
  }
}

void readers_apply_basis(const PriceBase& pb, const fs::path& tmp) {
  std::printf("readers_apply_basis\n");
  fs::remove_all(tmp);
  const fs::path real = tmp / "constant", mixed = tmp / "current";
  write_synthetic_repo(real, pb, false);
  write_synthetic_repo(mixed, pb, true);
  {
    std::ofstream f(tmp / "basis.csv", std::ios::binary);
    f << "state,from_year,basis,note\nBeta,2012,current_prices,synthetic\n";
  }
  const GsdpPriceBasis basis = GsdpPriceBasis::load((tmp / "basis.csv").string(), pb), none = GsdpPriceBasis::none();
  const auto org = [](const fs::path& r) { return (r / "data/raw/competition/renewable_energy_dataset.csv").string(); };
  const auto near = [](double a, double b) { return std::abs(a - b) < 1e-12 * std::max(1.0, std::abs(b)); };

  // estimate_convergence
  const ConvergenceResult c_real = estimate_convergence(org(real), 2006, 2024, 200, 7, none);
  const ConvergenceResult c_fix = estimate_convergence(org(mixed), 2006, 2024, 200, 7, basis);
  const ConvergenceResult c_raw = estimate_convergence(org(mixed), 2006, 2024, 200, 7, none);
  const auto beta_rel = [](const ConvergenceResult& c) {
    for (const StateIncome& si : c.states)
      if (si.state == "beta") return si.ln_rel.at(2024);
    return kNaN;
  };
  std::printf("    convergence: lambda constant prices %.6f | current, with basis %.6f | current, none %.6f; Beta "
              "2024 relative income %.4f | %.4f | %.4f\n",
              c_real.lambda, c_fix.lambda, c_raw.lambda, beta_rel(c_real), beta_rel(c_fix), beta_rel(c_raw));
  check(c_fix.n_states == 6 && near(c_fix.slope, c_real.slope) && near(c_fix.lambda, c_real.lambda) &&
            near(c_fix.lambda_se, c_real.lambda_se) && near(beta_rel(c_fix), beta_rel(c_real)),
        "estimate_convergence with the basis = the constant-price data (slope, lambda, s.e., Beta)");
  check(beta_rel(c_raw) - beta_rel(c_real) > 0.3 && std::abs(c_raw.slope - c_real.slope) > 1e-4,
        "estimate_convergence with none(): Beta carries its inflation, the slope moves");

  // state_panel_data and state_panel_check
  const StatePanelData d_real = state_panel_data(real.string(), none);
  const StatePanelData d_fix = state_panel_data(mixed.string(), basis);
  const StatePanelData d_raw = state_panel_data(mixed.string(), none);
  bool same = d_fix.obs.size() == d_real.obs.size() && d_raw.obs.size() == d_real.obs.size() && d_real.obs.size() == 36;
  // Without the basis, Beta's ln income per person is too high by exactly ln(deflator / 100).
  bool gap = true;
  double raw_gap = 1e9;
  for (size_t i = 0; same && i < d_real.obs.size(); ++i) {
    const PanelObs &a = d_real.obs[i], &b = d_fix.obs[i], &c = d_raw.obs[i];
    same &= a.state == b.state && a.year == b.year && near(b.ln_income_pc, a.ln_income_pc) && b.ln_gen_pc == a.ln_gen_pc;
    if (a.state != "beta") continue;
    gap &= std::abs(c.ln_income_pc - a.ln_income_pc - std::log(pb.inr_index(a.year) / pb.inr_index(2011))) < 1e-12;
    raw_gap = std::min(raw_gap, c.ln_income_pc - a.ln_income_pc);
  }
  const PanelFit f_real = fe_slope(d_real.obs), f_fix = state_panel_check(mixed.string(), basis),
                 f_raw = state_panel_check(mixed.string(), none);
  std::printf("    state panel: slope constant prices %.6f | current, with basis %.6f | current, none %.6f; Beta "
              "income gap without the basis >= %.4f log points\n",
              f_real.slope, f_fix.slope, f_raw.slope, raw_gap);
  check(same && near(f_fix.slope, f_real.slope), "state_panel_data / state_panel_check with the basis = the constant-price data");
  check(gap && raw_gap > 0.25 && std::abs(f_raw.slope - f_real.slope) > 1e-4,
        "state_panel_data with none(): Beta's income carries ln(deflator), the slope moves");
}

void default_plan_unchanged_by_price_base(const std::string& root) {
  std::printf("default_plan_unchanged_by_price_base\n");
  const AllocationInputs in = load_allocation_inputs(root);
  bool usd2024 = true;
  int n = 0;
  {
    const CsvTable t = CsvTable::read(root + "/data/external/techno_economic/lcoe.csv");
    for (size_t i = 0; i < t.rows(); ++i)
      if (t.str(i, t.col("cost_metric")) == "lcoe_projection") {
        usd2024 &= t.str(i, t.col("unit")) == "USD(2024)/MWh";
        ++n;
      }
  }
  check(usd2024 && n > 0, "every IEA LCOE row is USD(2024)/MWh: already on the price base");
  double base_total = 0;
  std::array<double, kRegions> base_r{};
  for (int r = 0; r < kRegions; ++r) {
    for (int s = 0; s < kTechs; ++s) base_r[r] += in.base_twh[r][s];
    base_total += base_r[r];
  }
  std::array<double, kRegions> req{};
  for (int r = 0; r < kRegions; ++r) req[r] = 4500.0 * base_r[r] / base_total;  // as in test_allocation
  const PolicySettings p;
  const AllocationResult lp = solve_least_cost(in, p, req);
  std::printf("    default plan at 4,500 TWh: cost %.9f bn USD, CO2 %.9f Mt\n", lp.cost_busd, lp.co2_mt);
  // Commit a314521 (before the price base): cost 199.401019100 bn USD, CO2 1312.647750673 Mt.
  check(lp.ok && std::abs(lp.cost_busd - 199.401019100) < 1e-6 && std::abs(lp.co2_mt - 1312.647750673) < 1e-6,
        "default plan = the plan before the price base (199.401019 bn USD, 1312.647751 Mt)");
  const RegionTech table = cost_table(in, p);
  bool same = true;
  for (int r = 0; r < kRegions; ++r)
    for (int s = 0; s < kTechs; ++s) same &= table[r][s] == in.cost[r][s];
  check(same, "cost table of the default policy = the IEA table");
  bool flat = true;
  for (double pi : {0.02, 0.06}) {
    PolicySettings q;
    q.cea_inflation = pi;
    const AllocationResult a = solve_least_cost(in, q, req);
    flat &= a.ok && a.cost_busd == lp.cost_busd && a.co2_mt == lp.co2_mt;
  }
  check(flat, "IEA basis: cea_inflation 2% or 6% changes nothing");
  bool note = false;
  for (const std::string& s : in.notes) note |= s.rfind("Price base", 0) == 0 && s.find("real 2024 USD") != std::string::npos;
  check(note, "provenance note states the price base (real 2024 USD)");
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const std::string root = argc > 1 ? argv[1] : ".";
    const PriceBase pb = PriceBase::load(root + "/data/external/macro/price_indices.csv");
    fisher_known();
    deflate_known(pb);
    inr_to_usd_known(pb);
    beyond_data_uses_target(pb);
    nominal_outlook_known(pb);
    jk_deflated(root, pb, fs::temp_directory_path() / "e47_test_price_base");
    readers_apply_basis(pb, fs::temp_directory_path() / "e47_test_gsdp_readers");
    default_plan_unchanged_by_price_base(root);
    std::printf(fails ? "FAILED (%d)\n" : "ok\n", fails);
    return fails ? 1 : 0;
  } catch (const std::exception& e) {
    std::printf("error: %s\n", e.what());
    return 2;
  }
}
