#pragma once
// States of the regional split, from the challenge data only (organisers' ruling of
// 16 November 2026: "Make use of the dataset provided as part of the challenge"):
//   * population by state and year: the challenge dataset
//     (data/raw/competition/renewable_energy_dataset.csv; only its State, Year,
//     Population and Zone columns are read here);
//   * grid region: the region the National Power Portal reports the state under
//     (data/raw/npp/npp_state_generation_fy.csv). A state that file does not list
//     takes the region of its dataset Zone; Central maps to Western, because Madhya
//     Pradesh and Chhattisgarh are in the Western grid.
#include <map>
#include <string>
#include <vector>

namespace e47 {

struct StateRecord {
  std::string state;          // normalised name (normalize_state)
  std::string zone;           // dataset Zone column
  int region = -1;            // grid region index (region_name)
  std::string region_from;    // where the region came from: "NPP" or "dataset zone"
  std::map<int, double> pop;  // dataset year -> persons
};

struct StatePopulation {
  int last_year = 0;                // last dataset year with a population
  std::vector<StateRecord> states;  // states with a population in last_year, sorted by name
  std::vector<std::string> notes;   // mapping report: zone fallbacks, disagreements, states left out
};

// Throws if a state with a population in the last dataset year has no grid region.
StatePopulation load_state_population(const std::string& organiser_csv, const std::string& npp_state_csv);

// Mean ln growth of population per year from year `from` to year `to`, for one state,
// or for India (state empty) over the states reported in both years. NaN if a year
// is missing.
double state_population_growth(const StatePopulation& sp, const std::string& state, int from, int to);

}  // namespace e47
