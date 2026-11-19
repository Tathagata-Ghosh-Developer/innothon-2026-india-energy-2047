# Data audit and provenance

The competition allows **energy generated** only for forecasting and comparison.
This table lists every dataset the model reads, the columns used, and whether each is a generation quantity.
Detailed extraction notes (URLs, pages, tables, unit conversions, checksums) are in:

- [data/raw/npp/SOURCES.md](../data/raw/npp/SOURCES.md) (National Power Portal: API endpoints, report URL patterns, validation)
- [data/raw/generation/SOURCES.md](../data/raw/generation/SOURCES.md)
- [data/external/SOURCES.md](../data/external/SOURCES.md)
- [data/external/techno_economic/SOURCES.md](../data/external/techno_economic/SOURCES.md)

## Source hierarchy

Two sources are primary, because the organisers provided them:

1. **National Power Portal**, <https://npp.gov.in/landing-home>, including the CEA generation reports it hosts (18-column monthly reports, daily generation reports, Merit real-time data).
2. **Renewable India dataset**, <https://github.com/chrisjaimyantony/renewable-india-dataset/blob/main/data/renewable_energy_dataset.csv> (our copy matches the upstream file byte for byte).

Every other source is secondary: UN population projections, CEA and MNRE publications not hosted on NPP, NITI Aayog and DAE publications, IEA costs, Ember and World Bank data, and published studies.
The analysis relies on the two primary sources first.
Secondary sources enter only where the rules allow them: national population and urban-share drivers, limits and prices in the generation mix, a labelled new-loads scenario outside the headline, and benchmarks and justification.
Each secondary input is labelled with its source.

The work has two parts:

- **Part 1, the 2047 requirement** (national and regional): estimated from the two primary sources only. Income is used state-wise: the GSDP and population of 34 states and union territories in the Renewable India dataset.
- **Part 2, the 2047 generation mix**: the kickoff guidance allows publicly available literature for the growth rates and constraints of each energy source, so published figures (build limits, committed builds, resource potentials, costs and emission factors) set the limits and prices of the mix. Its base year starts from NPP generation (primary) and takes only regional renewables from CEA (secondary), because NPP has none by region. Asked whether published figures may act this way, the organisers replied "Use insights from this data source"; a clarification has been requested and no reply has come yet.

## Rule check

| Rule | Status |
| --- | --- |
| Forecast target is energy generated | Yes. Every model predicts generation (TWh) or generation per person (kWh). |
| No installed capacity in forecasting | Yes. No capacity column enters the forecast models. The organiser file's capacity columns are never read. |
| No energy consumption in forecasting or comparison | Yes. Ember "Demand" and "Net imports" rows are dropped at load time. The CEA NEP state demand table (`nep2023_state_demand.csv`) is collected but not used. |
| Comparisons use generation | Yes. The only external benchmark plotted is the CEA NEP *gross generation* projection (2,666 TWh for FY2031-32). The docs also quote CEA's 2026 review gross generation (about 3,596 BU for 2035-36, press reports). CEA energy requirement figures (Electric Power Surveys) appear only as labelled context, never as a benchmark. |
| Capacity figures in the strategy | Only converted to generation (TWh/yr) with the capacity factor stated, as limits on generation: published resource potentials (upper bounds), published additions in GW a year (build limits) and committed nuclear and hydro units (floors). Capacity policy targets (500 GW, 100 GW nuclear) are context only, and the Renewable India dataset's installed capacity is never read. See [decisions.md](decisions.md), D5 and D13. |
| Core 2047 prediction uses only the two challenge datasets (kickoff, 13 Nov 2026) | Yes for the primary core: generation from the National Power Portal, population and real GSDP from the Renewable India dataset. External inputs are drivers and constraints the rules allow from literature: India's population and urban-share projections, the urbanisation coefficient prior, and the strategy's resource, cost and policy data. |
| Organisers' written ruling (16 Nov 2026; [decisions.md](decisions.md), D12) | 1. UN WPP 2024 and UN WUP 2018 are used for India; `outputs/population_consistency.csv` shows the paths are consistent with the dataset's state growth. 2. The urbanisation coefficient is the team's judgment (literature prior). 3. No captive generation: NPP generation only; the CEA captive layer is removed. 4. The cross-country model (Ember, World Bank) is an independent check used for justification only. 5. The regional split uses the challenge dataset (state population; Zone where NPP lists no state) and NPP's state-to-region mapping. 6. Resource potentials never enter the forecast; they only bound the supply plan. On this use the organisers replied "Use insights from this data source"; a clarification has been requested and no reply has come yet (D12). |
| Midpoint panel questions (18 Nov 2026; [decisions.md](decisions.md), D13; [midpoint_panel_answers.md](midpoint_panel_answers.md)) | Income is used state-wise from the Renewable India dataset; the elasticity is estimated for India as a whole (a state panel on NPP state generation is a diagnostic only); regional renewables come from CEA (secondary) because NPP has none by region; new loads are a labelled scenario outside the headline. |

## Forecast inputs: primary core (India only)

| File | Columns used | Unit | Generation? | Source | Used for |
| --- | --- | --- | --- | --- | --- |
| `data/raw/npp/npp_national_generation_fy.csv` | fy_start_year, thermal, hydro, nuclear, RES, total | GWh | Yes | NPP dashboard API `get_generation_chart_list` | Generation history FY2006-07 to FY2023-24; elasticity estimation; holdout RES growth |
| `data/raw/competition/renewable_energy_dataset.csv` | State, Year, Zone, Population, GSDP (crore INR) **only** | persons, INR crore (constant prices; 2004-05 base to 2010, 2011-12 base from 2011), zone label | Driver | Organiser dataset (challenge dataset) | National real income per person (chained over matched states, skipping GSDP values copied from the previous year), population growth, relative state income, convergence speed; state population paths of the regional split (2024 level and 2019-2024 growth); Zone only for a state NPP does not list (Chandigarh) |
| `data/raw/npp/npp_region_generation_fy.csv` | All India / region, thermal, nuclear, hydro, fiscal year totals | GWh | Yes | NPP, CEA 18-column monthly reports, Sub-report 2 | Holdout check (FY2024-25, FY2025-26); regional transfer gap |
| `data/raw/npp/npp_state_generation_fy.csv` | region, state; for the panel check also fiscal year and generation by source | label; GWh | Mapping; generation only in the diagnostic | NPP, CEA 18-column monthly reports, Sub-report 16 | Grid region of each state in the regional split; the state panel check (`outputs/state_panel_check.csv`, diagnostic only, never used by the forecast) |
| `data/external/literature_priors.csv` | urban_excess_gamma: distribution, mean, sd | semi-elasticity | Parameter | Liddle 2013; Shahbaz et al. 2016; Holtedahl & Joutz 2004 | Urbanisation beyond the income-implied path (the team's judgment, as the organisers ruled) |
| `data/raw/npp/npp_merit_generation_hourly.csv` | datetime, source, mean MW | MW, GWh | Yes | NPP Merit India real-time API `demandmet2chartdata` | Solar and wind hourly profiles (strategy and storage) |

`data/raw/generation/national/cea_national_generation_fy.csv` (CEA General Review, including captive plants) stays in the repository but is no longer read: the captive layer was removed under the organisers' ruling of 16 November 2026.

## Scenario inputs outside the headline

| File | Columns used | Unit | Generation? | Source | Used for |
| --- | --- | --- | --- | --- | --- |
| `data/external/new_loads.csv` | load, year, twh_low, twh_central, twh_high, overlaps_history | TWh of end-use electricity | No (end-use, not grossed up for losses) | NITI Aayog (2026), TERI (2024), CEEW (2025), BloombergNEF (2026), IEA (2018); secondary | `outputs/new_loads_2047.csv` and the sensitivity row `new_loads_published` only. Only loads absent from the history (green hydrogen) are added; never in the headline |

## Forecast inputs: independent check (cross-country, justification only)

| File | Columns used | Unit | Generation? | Source | Used for |
| --- | --- | --- | --- | --- | --- |
| `data/raw/generation/downloads/ember/release_generation_yearly_global.csv` | Area, ISO 3 code, Year, Area type, Electricity source = "Total generation", Generation (TWh) | TWh | Yes | Ember yearly electricity data (CC BY 4.0) | Cross-country panel |
| `data/raw/worldbank/downloads/NY.GDP.PCAP.PP.KD/*` | Country Code, year columns | constant 2021 international $ per person | Driver | World Bank WDI | Income driver |
| `data/raw/worldbank/downloads/SP.URB.TOTL.IN.ZS/*` | Country Code, year columns | percent | Driver | World Bank WDI (UN WUP based) | Urbanisation driver |
| `data/raw/worldbank/downloads/SP.POP.TOTL/*` | Country Code, year columns | persons | Driver | World Bank WDI | Generation per person |

## Drivers shared by both cores

| File | Columns used | Unit | Generation? | Source | Used for |
| --- | --- | --- | --- | --- | --- |
| `data/external/population_projection.csv` | state, year, reference_date, population, variant | persons | Driver | UN WPP 2024 (India, medium/low/high); Technical Group on Population Projections 2011-2036 (states, 1 July) | UN WPP 2024: national population paths and the India-level growth of the state paths. Technical Group: only a reference column in `outputs/population_consistency.csv`, never read by the model |
| `data/external/urbanization.csv` | India rows from UN WUP 2018 | percent | Driver | UN DESA World Urbanization Prospects 2018 | National urban path |
| `data/external/techno_economic/policy_targets.csv` | target = "Projected gross generation, Total (CEA NEP 2022-32)" | BU (= TWh) | Yes | CEA NEP 2022-32 Vol I, Table 5.11 | Benchmark points on the chart only |

## Strategy inputs

| File | Columns used | Unit | Generation? | Source | Used for |
| --- | --- | --- | --- | --- | --- |
| `data/raw/npp/npp_region_generation_fy.csv` | FY2023-24 (base) and FY2018-19 to FY2025-26 (fleet floors): region, thermal, nuclear, hydro; subtotal, partial and Bhutan-import rows left out | GWh | Yes | NPP, CEA 18-column monthly reports, Sub-report 2 (primary) | Base-year regional mix: conventional generation by grid region, 1,508.5 TWh in FY2023-24; existing-fleet floors of nuclear and hydro |
| `data/raw/npp/npp_state_generation_fy.csv` | region, state, fiscal year, coal_lignite, gas, diesel | GWh | Yes | NPP, CEA 18-column monthly reports, Sub-report 16 (primary) | Coal and gas split of each region's thermal generation (station sums); existing-fleet floors of coal and gas; region of each renewable-table state |
| `data/raw/generation/state/cea_state_res_by_type_fy.csv` | FY2023-24 state rows: solar, wind, small hydro, biomass and bagasse, waste to energy | GWh | Yes | CEA General Review 2025, Table 4.7 (secondary) | Base-year regional renewables (225.8 TWh), because NPP has no renewable generation by region or state |
| `data/raw/generation/region/cea_region_generation_fy.csv` | FY2023-24 region rows: coal, gas, oil and diesel | GWh | Yes | CEA General Review 2025, Table 4.4 (secondary) | Cross-check of the coal and gas split only (within 0.65 TWh) |
| `data/raw/npp/npp_national_generation_fy.csv` | fiscal year, thermal, hydro, nuclear, RES | GWh | Yes | NPP dashboard API (primary) | Record annual increases: build-limit fallback for a source without published rows (none today) and a cross-check |
| `data/raw/generation/region/ember_region_generation_fy.csv`, `ember_region_generation_cy.csv` | region, source, generation_gwh | GWh | Yes | Ember India state data aggregated to regions (secondary) | Cross-check of the base mix only (FY2023-24 1,734.1 TWh against 1,734.4); the midpoint's CY2024 base (1,801.2 TWh) |
| `data/external/techno_economic/build_limits.csv` | tech, period_start, period_end, gw_per_year_low / central / high, cuf | GW a year, CUF | Converted (GW a year x CUF x 8.76) | CEA NEP 2022-32; CEA input slides (Dec 2024); NITI Aayog (Feb 2026); DAE Lok Sabha answer (03.12.2025); MoP Annual Report (secondary) | Published build limits: each source's 2047 generation at most its starting level plus the published additions |
| `data/external/techno_economic/nuclear_pipeline.csv` | region, units, capacity_mw, status, expected_commissioning | count, MW | Converted (MW x CUF 0.68 x 8.76) | DAE / NPCIL / PIB (secondary) | Nuclear siting key (reactor counts); committed nuclear floor (14,300 MW, 85.2 TWh) |
| `data/raw/generation/state/state_region_map.csv` | state, region | label | No | CEA General Review grouping | Resource potential and committed hydro to regions; region of the two renewable-table territories NPP does not list |
| `data/external/state_region_map.csv` | grid_region, row_type, area_km2 | km² | No | ISFR 2023 areas; CEA NEP 2022-32 Annexure 4.1 grouping | Area of India and of each grid region for the land back-calculation only; not used for the regional split |
| `data/external/resource_potential.csv` | solar ground_mounted_potential_nise_2025 (GW); wind_150m potential (GW); large_hydro identified 2017-23 (MW) and under_construction_mw; small_hydro, biomass, bagasse potential (MW) | GW, MW | Converted | NISE, NIWE, CEA, MNRE (via MNRE RE Statistics, CEA reports) | Generation potential = potential x CUF x 8.76 (upper bound); committed large hydro (13,023.5 MW x CUF 0.36 x 8.76 = 41.1 TWh, a floor); solar technical_potential_nise_2014 and wind_120m potential only in `outputs/sensitivity_mix.csv` |
| `data/external/techno_economic/capacity_factors.csv` | Regional planning CUF (solar new, wind, offshore); observed hydro CUF by region; biomass availability | percent | Conversion factor | CEA NEP 2022-32; Ember observed | Potential conversion; regional cost scaling |
| `data/external/techno_economic/offshore_wind.csv` | NIWE initial potential by state (GW) | GW | Converted | MNRE / NIWE | Offshore generation potential |
| `data/external/techno_economic/lcoe.csv` | IEA lcoe_projection 2035 and 2050; coal, hydro, biomass, round-the-clock RE and solar tariffs | USD/MWh, INR/kWh | Cost | IEA WEO 2025; SECI; CERC; developer releases | 2047 costs |
| `data/external/techno_economic/emission_factors.csv` | coal and gas station-weighted average, the latest period (2025-26) | tCO2/MWh | Factor | CEA CO2 Baseline Database v22 | CO2 of each plan |
| `data/external/techno_economic/cea_costs.csv`, `cea_cost_basis.csv` | NEP capex, fixed O&M, life, CUF, heat rate; CERC debt share, interest and pre-tax RoE; CEA coal variable cost; CERC biomass price and GCV; RBI Rs per USD | Rs crore/MW, percent, Rs/kWh | Cost | CEA NEP 2022-32; CERC order 12/SM/2026; CEA optimal mix 2029-30; RBI Handbook Table 133 | CEA cost basis, a sensitivity only (`outputs/sensitivity_mix.csv`). The CERC rates are nominal and the NEP capital costs real (2021-22), so its cost level is not comparable with the IEA basis |
| `data/external/techno_economic/transmission.csv` | Corridor capability at 2032-03-31; regional loss rates | MW, percent | Converted | CEA / NEP Vol II | Corridor energy limits = MW x scale x utilisation x 8.76 |

## Known data caveats

| Caveat | Handling |
| --- | --- |
| NPP all-India generation ends in FY2023-24; NPP has no renewable generation after that and no regional or state renewable generation at all (the 18-column reports cover conventional stations above 25 MW; Merit India real-time solar and wind start on 18 Sep 2025, national only). | The forecast base is FY2023-24. FY2024-25 and FY2025-26 are a holdout: NPP conventional generation plus renewable generation extended at its FY2018-23 growth rate. |
| NPP chart renewable generation is 15 to 25 percent below other CEA renewable figures (FY2023-24: 189,490 vs 225,835 GWh). | The NPP series is used as published, consistently over time, for the forecast; the difference is a scope difference inside CEA publications. The regional base mix of Part 2 needs renewables by region, which only CEA gives, so its total (1,734.4 TWh) is 36 TWh above NPP's national 1,698 TWh. |
| State and regional generation series start only in FY2018-19 (CEA via NPP) or CY2019 (Ember), and show where plants are, not where power is used. | No regional or state regression is used. Regional requirement comes from drivers; NPP regional generation is compared with it as a transfer gap. A state fixed-effects panel on NPP state generation is reported as a diagnostic only (slope 0.434, s.e. 0.242; state generation per person from 0.08 to 15.9 times the national figure; `outputs/state_panel_check.csv`). |
| Organiser GSDP is at constant prices with a base change in 2011 (2004-05 to 2011-12 prices); the 2024 values are incomplete. | National growth is chained over matched states; the 2011 growth is imputed as the mean of 2010 and 2012 (4.63 percent per person, `outputs/india_fit.csv`, row `splice_growth`); 2024 is not used for income growth. The primary elasticity window starts in FY2011-12 (one price base); the spliced window is half of the bootstrap mixture. |
| Some organiser GSDP values are copied unchanged from the previous year: Telangana, Jammu and Kashmir, Andaman and Nicobar Islands and Lakshadweep repeat their 2006 value to 2010, and Nagaland repeats it in 2007 (Andaman and Nicobar Islands and Lakshadweep also repeat 2023 in 2024, a year that is not used). These are placeholders, not zero growth. | A state-year pair whose GSDP equals the previous year's is left out of national income growth; population growth still uses every matched state. With this rule the full-window elasticity is 0.866 and the imputed 2011 growth 4.63 percent; the FY2011-12 window (0.828) does not include those years and is unaffected. `tests/test_india.cpp` checks the rule. See [decisions.md](decisions.md), D11. |
| Urban share is in neither challenge dataset, and its post-2011 values are interpolations. | Urbanisation enters the national total through its excess over the income-implied path with a literature prior; gamma = 0 is reported as the pure two-dataset ablation. The regional split has no state urban term. |
| Captive plants are outside NPP. | Not estimated or reported (organisers' ruling of 16 Nov 2026: NPP generation only). |
| The challenge dataset's Zone column (North, South, East, West, North-East, Central) does not match NPP's five grid regions. | States take NPP's region; the Zone (Central as Western) is used only where NPP lists no state. NPP prevails for Andaman and Nicobar Islands (Zone South, NPP Eastern), Rajasthan (West, Northern) and Sikkim (North-East, Eastern). The organisers replied "Four, five, or six regions - either is fine" ([decisions.md](decisions.md), D12). |
| Ladakh and Dadra and Nagar Haveli and Daman and Diu have no rows in the challenge dataset. | Left out of the regional split (34 states and union territories remain). |
| The history includes years of energy shortage (deficits near 10 percent in the late 2000s, under 1 percent by FY2017-18), which may raise the estimated elasticity. | The FY2011-12 window (lower elasticity) carries half the bootstrap weight; an elasticity-decline setting and the first-difference estimate (0.78) show the effect; the direction of the bias is stated. CEA supply-position data are not used because they are requirement (consumption-side) quantities. |
| Ember national generation includes an estimate of captive plants; Ember state and region data do not. | Ember global generation is used only in the cross-country check (justification only). Ember is no longer the base mix or the build-rate evidence (both were at the midpoint); its region data are a cross-check of the NPP-based base mix only. |
| The challenge dataset's population column (checked 17 Nov 2026) has no value repeated from one year to the next in any state. Its growth rates are smooth and step down together in 2022 in almost every state (Tamil Nadu 0.47 to 0.30 percent a year, Uttar Pradesh 1.29 to 1.03, all-India 1.07 to 0.93): the pattern of a projection by five-year period, like the Technical Group projections, not of annual counts. Andaman and Nicobar Islands and Lakshadweep are rounded, so their yearly growth alternates (Lakshadweep shows zero growth in six of the eight years 2017-2024). | Population is used as given. The state growth offsets use the same 2019-2024 window as the national rate, so a step shared by all states cancels. The two rounded territories hold about 0.03 percent of the population, too little to move any region. |
| The challenge dataset's installed-capacity columns (solar, wind, small hydro, biomass, waste to energy) repeat their 2021 value in most states from 2022, and its 2024 solar total is about 1.3 times the MNRE figure. | Not used: installed capacity is not a forecast metric under the competition rules. |
| The challenge dataset's state populations end in 2024. | Each state grows at UN WPP 2024 medium growth for India plus its own 2019-2024 offset; the paths are checked against the dataset in `outputs/population_consistency.csv`. |
| CEA site links changed; some files came from Internet Archive copies of the original URLs. | Recorded in the SOURCES.md files. |
