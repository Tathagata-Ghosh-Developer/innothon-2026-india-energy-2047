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

struct AllocationInputs {
  // Base year: fiscal year by its start year (2023 = FY2023-24, April to March).
  // base_twh = NPP regional conventional generation (thermal split into coal and gas
  // with CEA regional shares) + CEA renewable generation by state (base_res_twh).
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

  // National build-rate evidence (Ember national, utilities + captive).
  int rate_last_year = 2025;
  std::array<double, kTechs> national_last_twh{};   // generation in rate_last_year
  std::array<double, kTechs> max_annual_increase{};  // largest year-on-year increase, 2010-2025, TWh
  std::vector<std::string> notes;  // derivation notes for the provenance table
};

AllocationInputs load_allocation_inputs(const std::string& repo_root);

}  // namespace e47
