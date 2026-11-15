# Data audit and provenance

The competition allows **energy generated** only for forecasting and comparison.
This table lists every dataset the model reads, the columns used, and whether each is a generation quantity.
Detailed extraction notes (URLs, pages, tables, unit conversions, checksums) are in:

- [data/raw/npp/SOURCES.md](../data/raw/npp/SOURCES.md) (National Power Portal: API endpoints, report URL patterns, validation)
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
| Core 2047 prediction uses only the two challenge datasets (kickoff, 13 Nov 2026) | Yes for the primary core: generation from the National Power Portal, population and real GSDP from the Renewable India dataset. External inputs are drivers and constraints the rules allow from literature: population and urban-share projections, the urbanisation coefficient prior, and the strategy's resource, cost and policy data. The cross-country model (Ember, World Bank) is labelled a secondary check. Captive generation (CEA) is reported beside the forecast, never inside it. |

## Forecast inputs: primary core (India only)

| File | Columns used | Unit | Generation? | Source | Used for |
| --- | --- | --- | --- | --- | --- |
| `data/raw/npp/npp_national_generation_fy.csv` | fy_start_year, thermal, hydro, nuclear, RES, total | GWh | Yes | NPP dashboard API `get_generation_chart_list` | Generation history FY2006-07 to FY2023-24; elasticity estimation; holdout RES growth |
| `data/raw/competition/renewable_energy_dataset.csv` | State, Year, Population, GSDP (crore INR) **only** | persons, INR crore (constant prices; 2004-05 base to 2010, 2011-12 base from 2011) | Driver | Organiser dataset (challenge dataset) | National real income per person (chained over matched states), population growth, relative state income, convergence speed |
| `data/raw/npp/npp_region_generation_fy.csv` | All India / region, thermal, nuclear, hydro, fiscal year totals | GWh | Yes | NPP, CEA 18-column monthly reports, Sub-report 2 | Holdout check (FY2024-25, FY2025-26); regional transfer gap |
| `data/external/literature_priors.csv` | urban_excess_gamma: distribution, mean, sd | semi-elasticity | Parameter | Liddle 2013; Shahbaz et al. 2016; Holtedahl & Joutz 2004 | Urbanisation beyond the income-implied path |
| `data/raw/generation/national/cea_national_generation_fy.csv` | scope = captive, source = total, FY2023-24 | GWh | Yes | CEA General Review 2025, Table 1.6 | Captive layer reported beside the forecast |
| `data/raw/npp/npp_merit_generation_hourly.csv` | datetime, source, mean MW | MW, GWh | Yes | NPP Merit India real-time API `demandmet2chartdata` | Solar and wind hourly profiles (strategy and storage) |

## Forecast inputs: secondary check (cross-country)

| File | Columns used | Unit | Generation? | Source | Used for |
| --- | --- | --- | --- | --- | --- |
| `data/raw/generation/downloads/ember/release_generation_yearly_global.csv` | Area, ISO 3 code, Year, Area type, Electricity source = "Total generation", Generation (TWh) | TWh | Yes | Ember yearly electricity data (CC BY 4.0) | Cross-country panel |
| `data/raw/worldbank/downloads/NY.GDP.PCAP.PP.KD/*` | Country Code, year columns | constant 2021 international $ per person | Driver | World Bank WDI | Income driver |
| `data/raw/worldbank/downloads/SP.URB.TOTL.IN.ZS/*` | Country Code, year columns | percent | Driver | World Bank WDI (UN WUP based) | Urbanisation driver |
| `data/raw/worldbank/downloads/SP.POP.TOTL/*` | Country Code, year columns | persons | Driver | World Bank WDI | Generation per person |

## Drivers shared by both cores

| File | Columns used | Unit | Generation? | Source | Used for |
| --- | --- | --- | --- | --- | --- |
| `data/external/population_projection.csv` | state, year, reference_date, population, urban_share, variant | persons, fraction | Driver | UN WPP 2024 (India, medium/low/high); Technical Group on Population Projections 2011-2036 (states, 1 July) | Population paths; state shares; state urban paths |
| `data/external/urbanization.csv` | India rows from UN WUP 2018 | percent | Driver | UN DESA World Urbanization Prospects 2018 | National urban path |
| `data/external/state_region_map.csv` | state, grid_region, row_type | label | No | CEA NEP 2022-32 Annexure 4.1 | State to grid region |
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
| NPP all-India generation ends in FY2023-24; NPP has no renewable generation after that and no regional or state renewable generation at all (the 18-column reports cover conventional stations above 25 MW; Merit India real-time solar and wind start on 18 Sep 2025, national only). | The forecast base is FY2023-24. FY2024-25 and FY2025-26 are a holdout: NPP conventional generation plus renewable generation extended at its FY2018-23 growth rate. |
| NPP chart renewable generation is 15 to 25 percent below other CEA renewable figures (FY2023-24: 189,490 vs 225,835 GWh). | The NPP series is used as published, consistently over time; the difference is a scope difference inside CEA publications. |
| State and regional generation series start only in FY2018-19 (CEA via NPP) or CY2019 (Ember), and show where plants are, not where power is used. | No regional regression is fitted. Regional requirement comes from drivers; NPP regional generation is compared with it as a transfer gap. |
| Organiser GSDP is at constant prices with a base change in 2011 (2004-05 to 2011-12 prices); the 2024 values are incomplete. | National growth is chained over matched states; the 2011 growth is imputed as the mean of 2010 and 2012; 2024 is not used for income growth. The primary elasticity window starts in FY2011-12 (one price base); the spliced window is half of the bootstrap mixture. |
| Urban share is in neither challenge dataset, and its post-2011 values are interpolations. | Urbanisation enters through its excess over the income-implied path with a literature prior; gamma = 0 is reported as the pure two-dataset ablation. |
| Captive plants are outside NPP. | Reported as a separate layer (CEA share held), never inside the forecast. |
| The history includes years of energy shortage (deficits near 10 percent in the late 2000s, under 1 percent by FY2017-18), which may raise the estimated elasticity. | The FY2011-12 window (lower elasticity) carries half the bootstrap weight; an elasticity-decline setting and the first-difference estimate (0.78) show the effect; the direction of the bias is stated. CEA supply-position data are not used because they are requirement (consumption-side) quantities. |
| Ember national generation includes an estimate of captive plants; Ember state and region data do not. | Used only in the secondary check. Regional base mixes are utilities only (1,804 TWh in 2024). |
| State population projections end in 2036. | Each state's 2031-2036 log-share trend is extended to 2047 and renormalised. |
| CEA site links changed; some files came from Internet Archive copies of the original URLs. | Recorded in the SOURCES.md files. |
| The Technical Group report's summary text (38.6 percent urban by 2036) differs from its own tables (39.58 percent). | The tables are used. |
