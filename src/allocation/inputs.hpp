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

// CEA station-weighted average emission rates (CO2 Baseline Database, tCO2/MWh net) of the
// latest period in emission_factors.csv. Only rows with basis "actual" and a fiscal-year
// period ("2025-26") count, so a projected row is never used. Rows of other metrics and fuels
// are ignored. Throws if the file has no basis column, or the latest such period lacks coal or gas.
struct StationEmissionFactors {
  std::string period;  // fiscal year as written in the file, e.g. "2025-26"
  double coal = 0, gas = 0;
  std::string source;  // source_name of the period's coal row (database version), if the file has one
};
StationEmissionFactors read_station_emission_factors(const std::string& csv_path);

// Capital recovery factor: rate / (1 - (1 + rate)^-years); 1 / years at rate 0.
double capital_recovery_factor(double rate, double years);

// CEA-basis levelised cost, Rs/kWh: CRF(wacc, life) x capex / (CUF x 8,760 h) + fixed O&M
// (fixed_om_pct of capex a year, over the same kWh) + fuel. capex in Rs crore per MW, CUF and
// wacc as fractions.
double cea_lcoe_rs_per_kwh(double capex_cr_per_mw, double fixed_om_pct, double life_years, double cuf,
                           double fuel_rs_per_kwh, double wacc);

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
  // Alternative potentials for the sensitivity cases (PolicySettings::solar_potential_set,
  // wind_height): NISE 2014 solar technical potential and NIWE 120 m wind potential, each x
  // the same regional CUF x 8.76 as the defaults (NISE 2025 ground-mounted, NIWE 150 m).
  std::array<double, kRegions> solar_potential_nise2014_twh{}, wind_120m_potential_twh{};
  // Every cost is in real 2024 USD (core/price_base.hpp), the price year of IEA WEO 2025.
  RegionTech cost{};            // USD/MWh in 2047 (regional resource quality applied): IEA basis, the default
  // Regional resource quality applied to the national solar and onshore wind costs of both
  // bases: mean regional CUF / the region's CUF (1 for the other sources).
  std::array<double, kRegions> solar_cost_scale{}, wind_cost_scale{};
  // CEA cost basis (PolicySettings::cost_basis), USD/MWh with the same regional resource
  // quality: cea_lcoe_rs_per_kwh on the CEA NEP 2022-32 inputs (cea_costs.csv) with a CERC
  // pre-tax WACC and CEA/CERC fuel costs (cea_cost_basis.csv), on real 2024 USD: capex
  // (2021-22 prices) and fuel (each at its own price year) moved to FY2024-25 rupees with the
  // MoSPI GDP deflator, converted at the calendar-2024 RBI average, and the nominal CERC WACC
  // made real with the Fisher relation at the RBI CPI target. Sources with no NEP capex
  // (nuclear, gas) keep their IEA cost (cea_from_iea). cost_cea and cea_lcoe_usd_per_mwh are
  // at the default inflation (cea_inflation_target); cea_cost_table gives any other.
  RegionTech cost_cea{};
  std::array<double, kTechs> cea_lcoe_usd_per_mwh{};  // national CEA-basis cost before regional scaling
  std::array<bool, kTechs> cea_from_iea{};            // true: the CEA basis keeps the IEA cost
  // NEP inputs of each CEA-basis source in FY2024-25 rupees (capex Rs crore per MW, fuel Rs/kWh).
  struct CeaNep {
    double capex_cr_per_mw = 0, fixed_om_pct = 0, life_years = 0, cuf = 0, fuel_rs_per_kwh = 0;
  };
  std::array<CeaNep, kTechs> cea_nep{};
  double cea_firm_premium = 0;     // IEA-basis round-the-clock premium over solar, USD/MWh (national)
  double cea_wacc_nominal = 0;     // CERC pre-tax WACC, nominal, fraction
  double cea_inflation_target = 0; // RBI CPI target, fraction: the default inflation of the real WACC
  double cea_wacc = 0;             // real pre-tax WACC at cea_inflation_target (Fisher), fraction
  double cea_capex_deflator = 0;   // GDP deflator FY2024-25 / FY2021-22 (NEP capex price year)
  double cea_inr_per_usd = 0;      // Rs per USD: RBI calendar-2024 average
  std::array<double, kTechs> emission_t_per_mwh{};
  std::string emission_period;  // CEA CO2 Baseline Database period of emission_t_per_mwh
  std::array<double, kRegions> nuclear_units{};  // reactor units built or in pipeline
  // Committed builds (secondary): plants already being built generate in 2047 whatever the cost
  // ranking says, so they raise the regional lower bound of their source above the FY2023-24
  // base (allocate.cpp, make_bounds). Energy only: committed MW / 1000 x CUF x 8.76, a supply-side
  // conversion; installed capacity is never forecast. Nuclear (nuclear_pipeline.csv): units under
  // construction or sanctioned, plus units commissioned after the base year (a dd.mm.yyyy date
  // after March of base_year + 1). Large hydro: CEA under-construction MW by state
  // (resource_potential.csv, under_construction_mw), each state in its grid region. CUF: the
  // lowest of the source's build_limits.csv rows, so a committed plant never counts for more
  // than the build cap credits a published addition (nuclear 0.68, hydro 0.36).
  std::array<double, kRegions> committed_nuclear_mw{}, committed_hydro_mw{};
  double committed_nuclear_cuf = 0, committed_hydro_cuf = 0;
  RegionTech committed_twh{};  // kNuclear and kHydro entries only, TWh a year
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

// CEA-basis costs in real 2024 USD/MWh at an expected inflation rate (fraction): the real
// pre-tax WACC is real_rate(cea_wacc_nominal, inflation); everything else is fixed. National
// costs (before regional scaling), and the table by region (solar and wind scaled by regional
// CUF as in the IEA basis). Nuclear and gas: the IEA cost. Firmed RE: CEA solar + the IEA-basis
// round-the-clock premium. Throws std::invalid_argument if inflation <= -1.
std::array<double, kTechs> cea_lcoe_usd(const AllocationInputs& in, double inflation);
RegionTech cea_cost_table(const AllocationInputs& in, double inflation);

}  // namespace e47
