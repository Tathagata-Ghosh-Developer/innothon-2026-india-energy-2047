#pragma once
// Storage and land back-calculated from a 2047 generation plan.
//
// Hourly shapes for a representative year come from the NPP Merit real-time
// series (all-India generation by source, Sep-2025 to Nov-2026, complete days
// only). The load shape is the shape of total generation (sum of the six
// sources), never demand. The plan's annual energy by source is dispatched hour
// by hour with stated flexibility rules; storage power and energy are the
// physical sizes needed to move surplus solar and wind into the hours that the
// firm fleet cannot cover. Land follows from solar and wind generation with
// energy-based land-use intensities (data/external/techno_economic/land_use.csv).
// Installed generating capacity is neither an input nor an output.
#include <array>
#include <string>
#include <vector>

#include "allocate.hpp"

namespace e47 {

constexpr int kDayHours = 24;

// Hourly series of the representative year, one complete Merit day per entry.
// Values are MW, detrended where the series has a fleet-growth trend.
enum ProfileSeries : int { kPLoad, kPSolar, kPWind, kPHydro, kPCoal, kPGas, kProfileSeries };

struct ProfileDay {
  std::string date;   // YYYY-MM-DD (IST)
  int month = 0;      // 1..12
  double weight = 0;  // days of the representative year this day stands for
  std::array<std::array<double, kDayHours>, kProfileSeries> mw{};
};

struct StorageInputs {
  bool ok = false;
  std::string status;
  // Representative year (chronological complete days; weights sum to 365).
  std::vector<ProfileDay> days;
  std::array<double, kProfileSeries> annual_mwh{};  // weighted annual sum of each series
  std::array<double, kProfileSeries> trend{};       // ln growth per year removed (0 = left as observed)
  std::array<bool, kProfileSeries> detrended{};
  double coal_peak_ratio = 0;  // highest hourly coal generation / mean hourly coal generation
  double gas_peak_ratio = 0;
  // Parameters (data/external/techno_economic/storage_parameters.csv).
  double round_trip = 0;      // fraction
  double coal_min_level = 0;  // coal fleet floor as a fraction of its highest hourly output
  // Land (data/external/techno_economic/land_use.csv, data/external/state_region_map.csv).
  double solar_acres_per_mw = 0;      // central (MNRE)
  double solar_acres_per_gwh = 0;     // high case (NREL total area)
  double wind_total_ha_per_mw = 0;    // project area
  double wind_direct_ha_per_mw = 0;   // permanent footprint
  std::array<double, kRegions> cuf_solar{}, cuf_wind{};  // same selection as the allocation inputs
  std::array<double, kRegions> region_area_km2{};
  double india_area_km2 = 0;
  double wasteland_km2 = 0;
  // CEA 2029-30 optimal-mix storage, for comparison.
  double bench_power_gw = 0, bench_energy_gwh = 0, bench_generation_twh = 0, bench_vre_share = 0;
  std::vector<std::string> notes;
};

// Never throws: a missing or unreadable file gives ok = false and a status.
StorageInputs load_storage_inputs(const std::string& repo_root);

// Storage needed by one hourly residual (firm dispatch already applied).
struct StorageSizing {
  double vre_twh = 0;              // solar + wind + offshore generation in this run
  double deficit_hours = 0;        // hours a year with a shortfall after firm dispatch, before storage
  double deficit_twh = 0;          // shortfall energy before storage
  double surplus_twh = 0;          // surplus solar and wind before storage
  double discharge_gw = 0;         // largest hourly shortfall after firm dispatch
  double power_gw = 0;             // charge and discharge rating: >= discharge_gw, raised for midday charging
  double energy_day_gwh = 0;       // daily cycle: largest daily need (design day)
  double energy_day_median_gwh = 0;
  std::string design_day;
  double duration_h = 0;           // energy_day / power
  double unmet_day_twh = 0;        // shortfall left with daily storage
  double unmet_day_free_charge_twh = 0;  // same store if charging were not limited by its power
  double curtailed_day_twh = 0;    // surplus that daily storage cannot take
  double throughput_twh = 0;       // energy discharged a year (daily storage)
  double losses_twh = 0;           // charged - discharged
  double cycles = 0;               // full cycles a year
  double energy_multi_gwh = 0;     // with carry-over between days, while extra energy cycles about weekly
  double unmet_multi_twh = 0;
  double curtailed_multi_twh = 0;
  // Shortfall left with carry-over between days for stores of kCurveMult x the daily energy.
  static constexpr std::array<double, 6> kCurveMult{1.0, 1.5, 2.0, 3.0, 5.0, 10.0};
  std::array<double, 6> curve_unmet_twh{};
  double curtailed_day_pct() const { return vre_twh > 0 ? 100.0 * curtailed_day_twh / vre_twh : 0.0; }
};

struct LandUse {
  std::array<double, kRegions> solar_km2{};        // central intensity
  std::array<double, kRegions> wind_direct_km2{};  // permanent footprint
  std::array<double, kRegions> wind_total_km2{};   // project area (mostly still farmed)
  double solar_km2_high = 0;                       // NREL total-area intensity
  double solar_twh = 0, wind_twh = 0;              // generation behind the land (incl. firmed RE)
  double total_km2() const;                        // solar + wind footprint
  double solar_total() const;
  double wind_total_area() const;
};

struct StorageLandResult {
  bool ok = false;
  std::string status;
  double load_twh = 0;            // requirement met by the plan (requirement - unserved)
  double firm_re_solar_twh = 0;   // firmed RE split back into solar and onshore wind
  double firm_re_wind_twh = 0;
  StorageSizing system;   // firmed RE treated as round-the-clock supply
  StorageSizing total;    // firmed RE unbundled into solar and wind (all storage in the system)
  LandUse land;
  double pct_india = 0;           // land / India's geographical area
  std::array<double, kRegions> pct_region{};
  double pct_wasteland = 0;       // solar + wind footprint / wasteland (2015-16)
  double solar_share_of_potential = 0;  // plan solar (incl. firmed) / solar generation potential
  double solar_share_of_usable = 0;     // same / (policy usable fraction x potential)
  double ms = 0;
};

StorageLandResult storage_and_land(const StorageInputs& in, const AllocationInputs& alloc, const PolicySettings& p,
                                   const AllocationResult& plan);

}  // namespace e47
