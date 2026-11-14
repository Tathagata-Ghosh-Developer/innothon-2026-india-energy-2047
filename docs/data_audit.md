# Data audit and provenance

The competition allows **energy generated** only for forecasting and comparison.
This table lists every dataset the model reads, the columns used, and whether each is a generation quantity.
Detailed extraction notes (URLs, pages, tables, unit conversions, checksums) are in:

- [data/raw/generation/SOURCES.md](../data/raw/generation/SOURCES.md)
- [data/external/SOURCES.md](../data/external/SOURCES.md)
- [data/external/techno_economic/SOURCES.md](../data/external/techno_economic/SOURCES.md)

## Rule check

| Rule | Status |
| --- | --- |
| Forecast target is energy generated | Yes. Every model predicts generation (TWh) or generation per person (kWh). |
| No installed capacity in forecasting | Yes. No capacity column enters the forecast models. The organiser file's capacity columns are never read. |
| No energy consumption in forecasting or comparison | Yes. Ember "Demand" and "Net imports" rows are dropped at load time. The CEA NEP state demand table (`nep2023_state_demand.csv`) is collected but not used. |
| Comparisons use generation | Yes. The only external benchmark plotted is the CEA NEP *gross generation* projection. |
| Capacity figures in the strategy | Only to convert published resource potentials (GW) into generation potential (TWh/yr), as upper bounds, with the capacity factor stated. Capacity policy targets (500 GW, 100 GW nuclear) are context only. See [decisions.md](decisions.md), D5. |

## Forecast inputs

| File | Columns used | Unit | Generation? | Source | Used for |
| --- | --- | --- | --- | --- | --- |
| `data/raw/generation/downloads/ember/release_generation_yearly_global.csv` | Area, ISO 3 code, Year, Area type, Electricity source = "Total generation", Generation (TWh) | TWh | Yes | Ember yearly electricity data (CC BY 4.0) | Cross-country panel; India history |
| `data/raw/worldbank/downloads/NY.GDP.PCAP.PP.KD/*` | Country Code, year columns | constant 2021 international $ per person | Driver | World Bank WDI | Income driver |
| `data/raw/worldbank/downloads/SP.URB.TOTL.IN.ZS/*` | Country Code, year columns | percent | Driver | World Bank WDI (UN WUP based) | Urbanisation driver |
| `data/raw/worldbank/downloads/SP.POP.TOTL/*` | Country Code, year columns | persons | Driver | World Bank WDI | Generation per person |
| `data/external/population_projection.csv` | state, year, reference_date, population, urban_share, variant | persons, fraction | Driver | UN WPP 2024 (India, medium/low/high); Technical Group on Population Projections 2011-2036 (states, 1 July) | Population paths; state shares; state urban paths |
| `data/external/urbanization.csv` | India rows from UN WUP 2018 | percent | Driver | UN DESA World Urbanization Prospects 2018 | National urban path |
| `data/external/state_region_map.csv` | state, grid_region, row_type | label | No | CEA NEP 2022-32 Annexure 4.1 | State to grid region |
| `data/raw/competition/renewable_energy_dataset.csv` | State, Year, Population, GSDP (crore INR) **only** | persons, INR crore (nominal) | Driver | Organiser dataset | Relative state income, convergence speed |
| `data/external/techno_economic/policy_targets.csv` | target = "Projected gross generation, Total (CEA NEP 2022-32)" | BU (= TWh) | Yes | CEA NEP 2022-32 Vol I, Table 5.11 | Benchmark points on the chart only |

## Strategy inputs

| File | Columns used | Unit | Generation? | Source | Used for |
| --- | --- | --- | --- | --- | --- |
| `data/raw/generation/region/ember_region_generation_cy.csv` | year = 2024, region, source, generation_gwh | GWh | Yes | Ember India state data aggregated to regions | Base-year regional mix; existing-fleet lower bounds |
| `data/raw/generation/national/ember_national_generation_cy.csv` | year, source, generation_gwh | GWh | Yes | Ember | Record annual generation increase by source (build-rate limit) |
| `data/raw/generation/state/state_region_map.csv` | state, region | label | No | CEA General Review grouping | Resource potential to regions |
| `data/external/resource_potential.csv` | solar ground_mounted_potential_nise_2025 (GW); wind_150m potential (GW); large_hydro identified 2017-23 (MW); small_hydro, biomass, bagasse potential (MW) | GW, MW | Converted | NISE, NIWE, CEA, MNRE (via MNRE RE Statistics, CEA reports) | Generation potential = potential x CUF x 8.76 (upper bound) |
| `data/external/techno_economic/capacity_factors.csv` | Regional planning CUF (solar new, wind, offshore); observed hydro CUF by region; biomass availability | percent | Conversion factor | CEA NEP 2022-32; Ember observed | Potential conversion; regional cost scaling |
| `data/external/techno_economic/offshore_wind.csv` | NIWE initial potential by state (GW) | GW | Converted | MNRE / NIWE | Offshore generation potential |
| `data/external/techno_economic/lcoe.csv` | IEA lcoe_projection 2035 and 2050; coal, hydro, biomass, round-the-clock RE and solar tariffs | USD/MWh, INR/kWh | Cost | IEA WEO 2025; SECI; CERC; developer releases | 2047 costs |
| `data/external/techno_economic/emission_factors.csv` | coal and gas station-weighted average, 2024-25 | tCO2/MWh | Factor | CEA CO2 Baseline Database v21 | CO2 of each plan |
| `data/external/techno_economic/transmission.csv` | Corridor capability at 2032-03-31; regional loss rates | MW, percent | Converted | CEA / NEP Vol II | Corridor energy limits = MW x scale x utilisation x 8.76 |
| `data/external/techno_economic/nuclear_pipeline.csv` | region, units | count | No | DAE / NPCIL | Nuclear siting key (reactor counts, not MW) |

## Known data caveats

| Caveat | Handling |
| --- | --- |
| State and regional generation series start only in FY2018-19 (CEA) or CY2019 (Ember). | No state-level regression is fitted. Regional requirement comes from drivers, not regional generation history. |
| Ember national generation includes an estimate of captive plants; Ember state and region data do not. | The national forecast uses the national series (2,080 TWh in 2025). Regional base mixes are utilities only (1,804 TWh in 2024). |
| Organiser GSDP is nominal and partly imputed. | It is used only as a ratio to the national value, where a common price level cancels. |
| State population projections end in 2036. | Each state's 2031-2036 log-share trend is extended to 2047 and renormalised. |
| CEA site links changed; some files came from Internet Archive copies of the original URLs. | Recorded in the SOURCES.md files. |
| The Technical Group report's summary text (38.6 percent urban by 2036) differs from its own tables (39.58 percent). | The tables are used. |
