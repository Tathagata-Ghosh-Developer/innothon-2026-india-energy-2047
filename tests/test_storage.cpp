// Storage and land check: loads the Merit hourly shapes and the land and storage
// parameters, sizes storage for two synthetic 2047 plans, and verifies the
// representative year, energy balances, monotonicity, linearity of land in
// generation and run-to-run determinism.
// Usage: test_storage <repo_root>
#include <cmath>
#include <cstdio>

#include "allocation/storage.hpp"

using namespace e47;

namespace {

int fails = 0;

void check(bool ok, const char* what) {
  std::printf("  %-66s %s\n", what, ok ? "ok" : "FAIL");
  if (!ok) ++fails;
}

// Plan with the given national shares of a 4,500 TWh requirement, split across
// regions in proportion to base-year generation.
AllocationResult make_plan(const AllocationInputs& in, const std::array<double, kTechs>& share) {
  AllocationResult a;
  a.ok = true;
  a.status = "synthetic";
  double base_total = 0;
  std::array<double, kRegions> base{};
  for (int r = 0; r < kRegions; ++r) {
    for (int s = 0; s < kTechs; ++s) base[r] += in.base_twh[r][s];
    base_total += base[r];
  }
  for (int r = 0; r < kRegions; ++r) {
    a.requirement[r] = 4500.0 * base[r] / base_total;
    for (int s = 0; s < kTechs; ++s) {
      a.gen[r][s] = share[s] * a.requirement[r];
      a.national[s] += a.gen[r][s];
      a.total_twh += a.gen[r][s];
    }
  }
  return a;
}

void print(const char* name, const StorageSizing& s) {
  std::printf("  %s: power %.0f GW (largest shortfall %.0f) | daily %.0f GWh (%.1f h, median day %.0f, design day %s) | "
              "multi-day %.0f GWh\n",
              name, s.power_gw, s.discharge_gw, s.energy_day_gwh, s.duration_h, s.energy_day_median_gwh,
              s.design_day.c_str(), s.energy_multi_gwh);
  std::printf("    unmet after daily storage %.1f TWh; with unlimited charging %.1f TWh\n", s.unmet_day_twh,
              s.unmet_day_free_charge_twh);
  std::printf("    before storage: shortfall %.0f h/yr, %.1f TWh; surplus %.1f TWh | daily store: unmet %.1f TWh, "
              "curtailed %.1f TWh (%.1f%% of %.0f TWh VRE), throughput %.1f TWh, losses %.1f TWh, %.0f cycles | "
              "multi-day: unmet %.1f, curtailed %.1f TWh\n",
              s.deficit_hours, s.deficit_twh, s.surplus_twh, s.unmet_day_twh, s.curtailed_day_twh, s.curtailed_day_pct(),
              s.vre_twh, s.throughput_twh, s.losses_twh, s.cycles, s.unmet_multi_twh, s.curtailed_multi_twh);
  std::printf("    unmet with carry-over, store x daily energy:");
  for (size_t k = 0; k < s.curve_unmet_twh.size(); ++k)
    std::printf(" x%.1f %.1f", StorageSizing::kCurveMult[k], s.curve_unmet_twh[k]);
  std::printf(" TWh\n");
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const std::string root = argc > 1 ? argv[1] : ".";
    const StorageInputs in = load_storage_inputs(root);
    std::printf("storage inputs: %s\n", in.status.c_str());
    if (!in.ok) return 2;
    for (const auto& n : in.notes) std::printf("  %s\n", n.c_str());
    static const char* names[kProfileSeries] = {"load", "solar", "wind", "hydro", "coal", "gas"};
    std::printf("  trends (ln/yr, same month a year apart):");
    for (int s = 0; s < kProfileSeries; ++s)
      std::printf(" %s %+.3f%s", names[s], in.trend[s], in.detrended[s] ? " (removed)" : "");
    std::printf("\n  coal peak/mean %.3f, gas peak/mean %.3f, round trip %.2f, coal minimum %.2f\n", in.coal_peak_ratio,
                in.gas_peak_ratio, in.round_trip, in.coal_min_level);
    std::printf("  regional CUF solar/wind:");
    for (int r = 0; r < kRegions; ++r) std::printf(" %s %.3f/%.3f", region_name(r), in.cuf_solar[r], in.cuf_wind[r]);
    std::printf("\n  areas km2: India %.0f, wasteland %.0f, regions", in.india_area_km2, in.wasteland_km2);
    for (int r = 0; r < kRegions; ++r) std::printf(" %.0f", in.region_area_km2[r]);
    std::printf("\n");

    std::printf("representative year:\n");
    double wsum = 0, region_sum = 0;
    for (const auto& d : in.days) wsum += d.weight;
    for (double a : in.region_area_km2) region_sum += a;
    check(in.days.size() >= 300, "at least 300 complete days");
    check(std::abs(wsum - 365.0) < 1e-9, "day weights sum to 365");
    for (int s = 0; s < kProfileSeries; ++s) {
      double e = 0;
      for (const auto& d : in.days)
        for (double v : d.mw[s]) e += d.weight * v;
      if (std::abs(e / in.annual_mwh[s] - 1.0) > 1e-12) check(false, "weighted series sums match annual totals");
    }
    check(std::abs(region_sum / in.india_area_km2 - 1.0) < 0.001, "regional areas add up to India (within 0.1%)");
    check(in.coal_peak_ratio > 1.0 && in.coal_peak_ratio < 2.0, "coal peak/mean ratio plausible");

    const AllocationInputs alloc = load_allocation_inputs(root);
    PolicySettings p;
    //                                   coal  gas  nucl hydro solar wind  off  bio  firm
    const std::array<double, kTechs> sa = {0.30, 0.00, 0.05, 0.08, 0.40, 0.10, 0.02, 0.02, 0.03};
    const std::array<double, kTechs> sb = {0.50, 0.00, 0.05, 0.08, 0.20, 0.10, 0.02, 0.02, 0.03};
    const AllocationResult plan_a = make_plan(alloc, sa), plan_b = make_plan(alloc, sb);
    const StorageLandResult a = storage_and_land(in, alloc, p, plan_a);
    const StorageLandResult a2 = storage_and_land(in, alloc, p, plan_a);
    const StorageLandResult b = storage_and_land(in, alloc, p, plan_b);
    std::printf("plan A (40%% solar, %.1f ms):\n", a.ms);
    print("system", a.system);
    print("total ", a.total);
    std::printf("  land: solar %.0f km2 (high %.0f), wind footprint %.0f km2, wind project area %.0f km2; %.2f%% of India\n",
                a.land.solar_total(), a.land.solar_km2_high, a.land.total_km2() - a.land.solar_total(),
                a.land.wind_total_area(), a.pct_india);
    std::printf("plan B (20%% solar, 50%% coal):\n");
    print("system", b.system);

    std::printf("checks:\n");
    check(a.ok && b.ok, "both plans sized");
    for (const StorageSizing* s : {&a.system, &a.total, &b.system}) {
      check(s->power_gw > 0 && s->energy_day_gwh > 0, "storage needed when solar is 20-40% of generation");
      check(s->duration_h > 0.5 && s->duration_h < 24, "daily storage duration between 0.5 and 24 hours");
      check(s->power_gw >= s->discharge_gw, "power rating covers the largest hourly shortfall");
      check(s->unmet_day_twh <= s->deficit_twh + 1e-9, "daily storage does not add shortfall");
      check(s->unmet_day_free_charge_twh <= s->unmet_day_twh + 1e-9, "unlimited charging never adds shortfall");
      check(s->unmet_multi_twh <= s->unmet_day_twh + 1e-6, "carry-over between days does not add shortfall");
      check(s->energy_multi_gwh >= s->energy_day_gwh, "multi-day store at least the daily store");
      bool falling = true;
      for (size_t k = 1; k < s->curve_unmet_twh.size(); ++k)
        falling &= s->curve_unmet_twh[k] <= s->curve_unmet_twh[k - 1] + 1e-9;
      check(falling, "a larger store never leaves more shortfall");
      check(s->curtailed_day_twh <= s->surplus_twh + 1e-9, "curtailment at most the surplus");
      // Cyclic steady state: discharged = round trip x charged, so losses = throughput (1/eta - 1).
      const double expect = s->throughput_twh * (1.0 / in.round_trip - 1.0);
      check(std::abs(s->losses_twh - expect) <= 1e-6 * std::max(1.0, expect), "losses match the round-trip efficiency");
    }
    {
      // Annual balance: surplus - shortfall = generation - load.
      const double gen = plan_a.total_twh, load = a.load_twh;
      check(std::abs((a.system.surplus_twh - a.system.deficit_twh) - (gen - load)) < 1e-6 * gen,
            "surplus - shortfall = generation - load (plan A)");
      const double gb = plan_b.total_twh;
      check(std::abs((b.system.surplus_twh - b.system.deficit_twh) - (gb - b.load_twh)) < 1e-6 * gb,
            "surplus - shortfall = generation - load (plan B)");
    }
    check(a.system.energy_day_gwh > b.system.energy_day_gwh, "more solar needs more storage energy");
    check(std::abs(a.total.vre_twh - a.system.vre_twh - plan_a.national[kFirmRE]) < 1e-6 * plan_a.total_twh &&
              std::abs(a.firm_re_solar_twh + a.firm_re_wind_twh - plan_a.national[kFirmRE]) < 1e-9 * plan_a.total_twh,
          "unbundled firmed RE adds exactly its energy as solar and wind");
    {
      // Without firmed RE both plans split solar across regions alike, so land is linear in generation.
      const std::array<double, kTechs> sc = {0.33, 0.00, 0.05, 0.08, 0.40, 0.10, 0.02, 0.02, 0.00};
      const std::array<double, kTechs> sd = {0.53, 0.00, 0.05, 0.08, 0.20, 0.10, 0.02, 0.02, 0.00};
      const StorageLandResult c = storage_and_land(in, alloc, p, make_plan(alloc, sc));
      const StorageLandResult d = storage_and_land(in, alloc, p, make_plan(alloc, sd));
      check(std::abs(c.land.solar_total() / d.land.solar_total() - 2.0) < 1e-12 &&
                std::abs(c.land.total_km2() - c.land.solar_total() - (d.land.total_km2() - d.land.solar_total())) < 1e-9,
            "solar land doubles with solar generation; wind land unchanged");
    }
    check(a.system.energy_day_gwh == a2.system.energy_day_gwh && a.total.energy_multi_gwh == a2.total.energy_multi_gwh &&
              a.system.unmet_day_twh == a2.system.unmet_day_twh && a.system.power_gw == a2.system.power_gw,
          "identical results on a second run (deterministic)");
    {
      // Gas as peaking supply: the plan's gas energy fills the highest residual hours, at most the
      // fleet's observed peak-to-mean ratio x its mean output. The check reports its hours and energy.
      check(a.total.gas_hours == 0 && a.total.gas_twh == 0, "no gas in the plan: no gas hours");
      const std::array<double, kTechs> se = {0.29, 0.01, 0.05, 0.08, 0.40, 0.10, 0.02, 0.02, 0.03};
      const AllocationResult plan_e = make_plan(alloc, se);
      const StorageLandResult e = storage_and_land(in, alloc, p, plan_e);
      const StorageSizing& g = e.total;
      const double mean_gw = plan_e.national[kGas] * 1e3 / 8760.0;
      std::printf("plan E (1%% gas, %.1f TWh): gas runs %.0f h/yr, %.2f TWh, up to %.1f GW (mean %.1f GW); %.2f TWh "
                  "in 18:00-24:00\n",
                  plan_e.national[kGas], g.gas_hours, g.gas_twh, g.gas_peak_gw, mean_gw, g.gas_evening_twh);
      check(e.ok, "plan E sized");
      check(std::abs(g.gas_twh - plan_e.national[kGas]) < 1e-6 * plan_e.national[kGas], "gas hours carry the plan's gas energy");
      check(g.gas_hours > 0 && g.gas_hours < 8760, "gas runs in part of the year only (peaking)");
      check(g.gas_peak_gw <= std::max(1.0, in.gas_peak_ratio) * mean_gw * (1 + 1e-9), "gas output within its peak-to-mean ratio");
      check(g.gas_evening_twh > 0 && g.gas_evening_twh <= g.gas_twh + 1e-9, "evening gas energy within the total");
    }
    std::printf("%s\n", fails ? "FAIL" : "ok");
    return fails ? 1 : 0;
  } catch (const std::exception& e) {
    std::printf("error: %s\n", e.what());
    return 2;
  }
}
