#pragma once
// Inputs for the 2047 regional generation strategy.
//
// Every quantity is annual energy (TWh) or cost per unit energy (USD/MWh).
// Published resource potentials in GW are converted to generation potential
// (TWh per year) with sourced capacity factors and used only as upper bounds;
// see docs/decisions.md, D5.
#include <array>
#include <string>
#include <vector>

#include "../forecast/mc.hpp"  // kRegions, region_name

namespace e47 {

enum Tech : int { kCoal, kGas, kNuclear, kHydro, kSolar, kWind, kOffshore, kBio, kFirmRE, kTechs };

const char* tech_key(int s);
const char* tech_label(int s);
bool tech_is_firm(int s);
bool tech_is_fossil(int s);
bool tech_is_variable(int s);

struct Corridor {
  int a = 0, b = 0;      // region indices
  double mw_2032 = 0;    // planned transfer capability, MW (NEP Vol II)
  double loss = 0.035;   // fraction lost per transfer
  std::string name;
};

using RegionTech = std::array<std::array<double, kTechs>, kRegions>;

// Cases of the published build limits (build_limits.csv: gw_per_year_low, _central, _high).
enum BuildCase : int { kBuildLow, kBuildCentral, kBuildHigh, kBuildCases };
const char* build_case_name(int k);  // "low", "central", "high"

// One row of data/external/techno_economic/build_limits.csv for a model source: the
// largest capacity a source may add per year over a period of fiscal years.
struct BuildLimitRow {
  int tech = -1;
  int period_start = 0, period_end = 0;          // fiscal start years, inclusive
  std::array<double, kBuildCases> gw_per_year{};  // low, central, high
  double cuf = 0;                                  // capacity utilisation factor, fraction
};

// Reads build_limits.csv. Rows for sources the model has no variable for (pumped storage,
// which the storage check sizes) are skipped. Throws on a missing column, a missing or
// out-of-range CUF, low > central > high order broken, or overlapping periods of a source.
std::vector<BuildLimitRow> read_build_limits(const std::string& csv_path);

// Generation (TWh a year) in to_year from what source `tech` may add after from_year in a
// case: the sum over build years y = from_year + 1 .. to_year of GW per year x CUF x 8.76.
// Every GW added by to_year generates in to_year. A year after a period's end holds that
// period's rate until the next period starts (so the last period's rate runs on to to_year);
// a year before the first period takes the first period's rate. 0 when the source has no rows.
double published_build_twh(const std::vector<BuildLimitRow>& rows, int tech, int build_case, int from_year,
                           int to_year);

struct AllocationInputs {
  // Base year: fiscal year by its start year (2023 = FY2023-24, April to March).
  // base_twh = NPP regional conventional generation (thermal split into coal and gas
  // with NPP station sums by state) + CEA renewable generation by state (base_res_twh).
  int base_year = 2023;
  RegionTech base_twh{};        // generation in base year (utilities), TWh
  RegionTech base_res_twh{};    // part of base_twh from CEA renewables by state (solar, wind, small hydro, bioenergy)
  int base_unmatched_states = 0;  // renewable-table states with no grid region (skipped and counted)
  std::vector<std::string> base_source_notes;  // base-mix provenance, each line "Primary: ..." or "Secondary: ..."
  RegionTech potential_twh{};   // generation potential at full resource use, TWh/yr
  RegionTech cost{};            // USD/MWh in 2047 (regional resource quality applied)
  std::array<double, kTechs> emission_t_per_mwh{};
  std::array<double, kRegions> nuclear_units{};  // reactor units built or in pipeline
  std::vector<Corridor> corridors;

  // National build limits for 2047 (allocate.cpp, build_caps). Each source's 2047 generation
  // is at most its starting level, max(existing-fleet floor, national base-year generation),
  // plus what the published limits let it add.
  int rate_last_year = 2023;                         // base year, FY2023-24 (fiscal start year)
  std::array<double, kTechs> national_last_twh{};    // base_twh summed over regions, TWh
  // Existing-fleet floor (primary, energy only; no installed capacity): the highest annual NPP
  // generation of each conventional source over the complete fiscal years fleet_first_year to
  // fleet_last_year (FY2018-19 to FY2025-26; a year with a partial row is left out). Nuclear
  // and hydro: NPP regional generation summed over the five regions (Bhutan import and the
  // All-India subtotal left out). Coal (coal and lignite) and gas (with diesel, as in the base
  // mix): NPP station sums by state. NPP hydro is stations above 25 MW, so the hydro floor adds
  // the base small hydro (CEA) to match the scope of the model's hydro. 0 for renewables, which
  // start from the FY2023-24 base. Without it, a source with no published additions (gas) would
  // be held at its FY2023-24 output however much its existing fleet generated before.
  int fleet_first_year = 2018, fleet_last_year = 2025;
  std::array<double, kTechs> fleet_floor_twh{};
  std::array<int, kTechs> fleet_floor_year{};        // fiscal start year of the NPP maximum (0: no floor)
  double fleet_floor_small_hydro_twh = 0;            // base small hydro inside fleet_floor_twh[kHydro]
  // Published limits (build_limits.csv, secondary): rows by source, and the TWh each source
  // can add from rate_last_year to build_target_year, by case [low, central, high][source].
  std::vector<BuildLimitRow> build_limits;
  int build_target_year = 2047;
  std::array<std::array<double, kTechs>, kBuildCases> published_build_twh{};
  std::array<bool, kTechs> has_published_build{};    // false: the record-based fallback applies
  // NPP national generation (primary): largest year-on-year increase between consecutive
  // fiscal years, TWh. NPP gives thermal and renewables as totals, so the thermal record is
  // shared between coal and gas and the renewable record between solar, wind and bioenergy
  // in proportion to their base-year generation. Fallback for a source with no published
  // rows, and a cross-check line in notes.
  std::array<double, kTechs> max_annual_increase{};
  std::vector<std::string> notes;  // derivation notes for the provenance table
};

AllocationInputs load_allocation_inputs(const std::string& repo_root);

}  // namespace e47
