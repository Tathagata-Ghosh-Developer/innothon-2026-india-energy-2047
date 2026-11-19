# Results

All numbers are energy generated (TWh).
They come from the default scenario of the primary (India-only) core: 4,194,304 Monte Carlo scenarios, seed 20470815.
Regenerate them with `energy2047 <repo_root>`; the tables are written to `outputs/`.

Years are fiscal years labelled by their start year: 2023 is FY2023-24, 2047 is FY2047-48.
The scope is the country's total NPP utility generation, and the headline is that total in TWh, not generation per person.

> **Change, 16 November 2026.** The organisers ruled at the kickoff that the core 2047 prediction must use only the two challenge datasets (National Power Portal and the Renewable India dataset).
> The primary forecast is now the India-only model ([methodology.md](methodology.md), section 2).
> The earlier cross-country model is kept as an independent check (section 1.4).
>
> **Update, 16 November 2026, after an independent code review.** The base-year anchor rule was replaced by an AR(1) model-error term, GSDP values copied unchanged from the previous year are no longer counted as zero income growth, and the bootstrap was corrected ([decisions.md](decisions.md), D11).
>
> **Update, 16 November 2026, after the organisers' written ruling.** The captive layer is removed, the regional split uses only the challenge data (dataset state populations and NPP grid regions), and the cross-country model is used for justification only ([decisions.md](decisions.md), D12).
> The national forecast does not change; the regional requirements and, through them, the regional plans do.
>
> **Update, 19 November 2026, after the midpoint review** ([decisions.md](decisions.md), D13).
> Part 1, the requirement, gains evidence but does not change: a growth decomposition, elasticity sensitivities, a state-by-state table, a state panel check and a new-loads scenario outside the headline (sections 1.5, 1.6, 2.2 and 4.1). The forecast files are byte-identical to the midpoint run.
> Part 2, the mix, now uses published figures, per the kickoff guidance: a FY2023-24 regional base mix from NPP (conventional, primary) and CEA (renewables, secondary) instead of Ember, published build limits instead of record years, committed nuclear and hydro builds, the gas fleet kept for peaking in the Robust policy, CEA v22 emission factors, and mix sensitivities (sections 3.7 and 3.8).
> Every number below is from the run that includes these changes.

![National forecast](img/1_national_forecast.png)

## 1. National generation requirement

India's utilities generated 1,698 TWh in FY2023-24 (NPP).

| Year | P5 | P10 | P25 | Median | P75 | P90 | P95 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| FY2030-31 | 1,955 | 2,019 | 2,132 | 2,266 | 2,411 | 2,553 | 2,644 |
| FY2035-36 | 2,316 | 2,430 | 2,633 | 2,884 | 3,165 | 3,449 | 3,634 |
| FY2040-41 | 2,695 | 2,870 | 3,190 | 3,594 | 4,061 | 4,542 | 4,864 |
| **FY2047-48** | **3,217** | **3,497** | **4,018** | **4,703** | **5,521** | **6,400** | **7,002** |

- The median requirement for FY2047-48 is **4,703 TWh**, 2.77 times FY2023-24 (4.3 percent a year).
- That is **3,005 TWh more** than the 1,698 TWh generated in FY2023-24.
- The 80 percent range (P10 to P90) is 3,497 to 6,400 TWh.

Central paths in FY2047-48 (medium drivers, mean growth, point estimates, expected model-error path):

| Model | FY2047-48 (TWh) | Role |
| --- | --- | --- |
| India elasticity with the urbanisation prior | 4,600 | Forecast (weight 1) |
| India elasticity, no urban term (two datasets only) | 4,386 | Ablation |
| Damped trend on NPP generation (no drivers) | 3,604 | Benchmark |

### 1.1 Estimation

| Quantity | Value |
| --- | --- |
| Income elasticity, FY2011-12 to FY2023-24 | 0.828 (s.e. 0.063) |
| Income elasticity, FY2006-07 to FY2023-24 (spliced price bases) | 0.866 (s.e. 0.031) |
| First-difference elasticity (pandemic years and the imputed 2011 change left out) | 0.782 / 0.787 |
| Elasticity used: block-bootstrap mixture of both windows | median 0.850, 90% interval 0.731 to 0.939 |
| FY2020-21 (pandemic) effect | -0.055 log points |
| Urbanisation prior gamma | normal(1.0, 0.5), truncated at 0 |
| Model error | AR(1) residual starting at +0.070 log points (FY2023-24), persistence 0.51 (Durbin-Watson 0.99), residual s.d. 0.037 |
| Imputed 2011 growth of GSDP per person (price-base splice) | 4.63 percent |
| Income growth per person, FY2012-13 to FY2023-24 without the pandemic years | mean 4.95 percent, s.d. 1.50 percent, persistence 0.63 |

### 1.2 Validation

| Check | Result |
| --- | --- |
| Rolling backtest, origins FY2012-13 to FY2018-19, MAPE at 1 / 3 / 5 years | 2.0% / 4.2% / 6.4% |
| Holdout FY2024-25 (NPP conventional 1,569 + renewable extended 216 = 1,785) | model 1,727 (P5 to P95: 1,632 to 1,827), error -3.3% |
| Holdout FY2025-26 (1,530 + 247 = 1,776) | model 1,786 (1,661 to 1,922), error +0.5% |

Both holdout years fall inside the model's 90 percent range.
NPP has no renewable generation for the holdout years, so renewable output is extended at its FY2018-19 to FY2023-24 growth rate (14.1 percent a year).
Conventional generation alone fell 2.5 percent in FY2025-26 while renewable output kept rising, so a check on conventional output alone would be biased.

### 1.3 Scope: no captive layer

The forecast covers NPP utility generation only.
Captive plants inside industries are outside the NPP series.
Earlier runs reported a captive layer beside the forecast from CEA data; the organisers' ruling of 16 November 2026 (NPP generation only, no external data for estimation) rules that out, so it is no longer computed or reported ([decisions.md](decisions.md), D12).

### 1.4 Independent check: cross-country model

The cross-country model uses data outside the challenge datasets, so it is used for justification only, not for estimation.

| Core | Scope | Base | 2047 P5 | 2047 median | 2047 P95 |
| --- | --- | --- | --- | --- | --- |
| India-only (primary) | NPP utilities, FY2047-48 | 1,698 (FY2023-24) | 3,217 | 4,703 | 7,002 |
| Cross-country panel (independent check) | Ember total generation (beyond NPP's utility series), calendar 2047 | 2,080 (2025) | 3,348 | 4,528 | 5,610 |

The cross-country median lies inside the India-only range, 4 percent below the India-only median, although its scope (total generation) is wider.
The difference is the elasticity: across 153 countries the elasticity at India's income is 0.44 to 0.57, falling with income; India's own data show 0.78 to 0.87 with no decline yet.
Setting an elasticity decline of 0.2 per unit of ln income in the India model gives 4,197 TWh (section 4), which brings the two methods together.

**CEA benchmarks (generation only).**

- The CEA National Electricity Plan 2022-32 projects gross generation of 2,666 TWh for FY2031-32. Our median for FY2031-32 is 2,382 TWh, 11 percent lower; CEA's figure lies between our P75 (2,551) and P90 (2,717).
- CEA's 2026 review gives about 3,596 BU of gross generation for 2035-36 (press reports; the document is not public). Our median for FY2035-36 is 2,884 TWh, 19.8 percent lower; CEA's figure is close to our P95 (3,634).
- CEA's energy requirement projections (the Electric Power Surveys, for example 2,474 BU for FY2031-32) are consumption-side and ex-bus, so they are context only and never a benchmark for generation.

The registration-form figure of 6,000 TWh was a pre-data estimate; it lies between our P75 (5,521) and P90 (6,400).

### 1.5 What drives the total

Income is the largest driver of the 2047 requirement, but not the only one.
`outputs/growth_decomposition_2047.csv` splits the growth of the central path, 1,698 TWh in FY2023-24 to 4,600 TWh in FY2047-48 (0.997 log points), into its parts ([methodology.md](methodology.md), section 2.8):

| Part | Log points | Share of growth |
| --- | --- | --- |
| Population (UN WPP 2024 medium) | 0.148 | 14.8% |
| Income (elasticity 0.828 x the change in ln GSDP per person) | 0.872 | 87.5% |
| Urbanisation beyond the income path (gamma 1.0) | 0.048 | 4.8% |
| Model error (the high FY2023-24 residual, +0.070, fading) | -0.070 | -7.1% |
| **Total** | **0.997** | **100%** |

The decomposition is of the central path (4,600 TWh), not of the median (4,703 TWh).
The elasticity is estimated for India as a whole; its sensitivities are in section 4.1: one standard error either way moves the median by -6.4 or +6.9 percent, about 1 percent per 0.01 of elasticity.

### 1.6 New loads (a separate scenario, outside the headline)

Published estimates of new loads (secondary; `outputs/new_loads_2047.csv`) are scaled from their published year, 2050, to 2047 (24/27). They are end-use electricity, not grossed up for losses.

| Load | Low | Central | High | Treatment |
| --- | --- | --- | --- | --- |
| Green hydrogen | 418 | 796 | 1,237 | Added in the scenario: absent from the historical series |
| Electric vehicles | 373 | 405 | 687 | Reported, not added: the income elasticity already carries part of it |
| Data centres | 356 | 356 | 565 | Reported, not added |
| Space cooling | 517 | 697 | 1,200 | Reported, not added |
| Electric cooking | 117 | 117 | 135 | Reported, not added |
| All five | 1,781 | 2,372 | 3,825 | Reference only: would double count |

TWh in FY2047-48.

With green hydrogen added, if supplied from the grid, the FY2047-48 median is 5,537 TWh (P5 3,991, P95 7,861), 17.7 percent above the headline.
The scenario is never the headline; the plan is stress-tested at the headline P90 (6,400 TWh), which is 1,697 TWh above the median.

## 2. Regional requirement

![Regional requirement](img/2_regional_requirement.png)

| Region | FY2030-31 median | FY2040-41 median | FY2047-48 P5 | FY2047-48 median | FY2047-48 P95 | Share of requirement 2047 | Share of NPP conventional generation FY2025-26 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Northern | 634 | 1,047 | 961 | 1,405 | 2,088 | 29.9% | 24.9% |
| Western | 595 | 940 | 836 | 1,226 | 1,835 | 26.1% | 36.0% |
| Southern | 601 | 868 | 721 | 1,068 | 1,624 | 22.7% | 20.1% |
| Eastern | 363 | 617 | 565 | 840 | 1,254 | 17.9% | 17.7% |
| North-Eastern | 73 | 121 | 110 | 161 | 239 | 3.4% | 1.3% |

The requirement is where power is needed (state population and income in the challenge dataset); NPP generation is where plants are today.
Shares of requirement are the planned requirements of `outputs/strategy_2047_regions.csv` (the national median split by the regional medians).

- **Transfer gap.** The Western region produces 36 percent of India's conventional generation but accounts for 26 percent of the 2047 requirement; the Northern, Southern and North-Eastern regions need more than they produce today, and the Eastern region is close to balance.
- The **Northern** region has the largest requirement because of its population (Uttar Pradesh, Rajasthan, Delhi); its share of India's population rises from 31.3 to 32.4 percent by 2047 because its states grew faster than India in the dataset.
- The **Southern** region's population share falls from 19.6 to 17.6 percent by 2047, because its states grew more slowly than India in 2019-2024 (Tamil Nadu 0.37, Kerala 0.45, Andhra Pradesh 0.43 percent a year against 0.97 percent); its higher income keeps its share of requirement (22.7 percent) above its population share.
- The **North-Eastern** share more than doubles relative to today's production share, driven by income convergence.

### 2.1 State population paths and the consistency check

State populations start from the challenge dataset's 2024 values and grow at the UN WPP 2024 medium growth of India plus each state's own 2019-2024 offset from India's growth in the dataset ([methodology.md](methodology.md), section 4).
States sit in the grid region NPP reports them under; the dataset's Zone is used only where NPP does not list a state (Chandigarh), and NPP prevails where the two disagree (Andaman and Nicobar Islands and Sikkim are Eastern, Rajasthan is Northern).
The 34 states and union territories are those with a 2024 population in the dataset; Ladakh and Dadra and Nagar Haveli and Daman and Diu have none.

`outputs/population_consistency.csv` checks the paths against the dataset, as the organisers' ruling asks:

| Percent a year | Challenge dataset 2011-2024 | Challenge dataset 2019-2024 | Model 2024-2030 | Model 2024-2047 |
| --- | --- | --- | --- | --- |
| India | 1.12 | 0.97 | 0.83 (= UN WPP 2024 medium) | 0.60 (= UN WPP 2024 medium) |
| Fastest state (Puducherry) | 2.33 | 2.27 | 2.13 | 1.90 |
| Slowest state (Lakshadweep) | 0.57 | 0.29 | 0.15 | -0.08 |

- Every state's path to 2030 lies 0.14 points a year below its own 2019-2024 growth: the same gap as between the dataset's national growth and UN growth, so the dataset's ranking of states is kept.
- The dataset's 2024 total (1.399 billion) is close to the Technical Group projection for 1 July 2024 (1.403 billion), which is shown as a reference column only.

| Region, share of population | 2024 (dataset) | 2047 (model) |
| --- | --- | --- |
| Northern | 31.3% | 32.4% |
| Western | 22.8% | 23.4% |
| Southern | 19.6% | 17.6% |
| Eastern | 22.6% | 22.9% |
| North-Eastern | 3.7% | 3.7% |

### 2.2 Income state by state

Income enters the split state-wise: each of the 34 states and union territories has its own GSDP per person from the Renewable India dataset, relative to India's, converging at 2.1 percent a year.
`outputs/state_income_2047.csv` shows the split on the central path; selected states:

| State | Grid region | Population share, 2024 | Income relative to India, 2024 to 2047 | Generation per person relative to India's, 2047 | Population share, 2047 | Share of the 2047 requirement |
| --- | --- | --- | --- | --- | --- | --- |
| Uttar Pradesh | Northern | 17.0% | 0.42 to 0.58 | 0.67 | 17.6% | 12.4% |
| Maharashtra | Western | 9.1% | 1.36 to 1.21 | 1.15 | 8.8% | 10.6% |
| Bihar | Eastern | 9.2% | 0.32 to 0.49 | 0.59 | 10.3% | 6.4% |
| Gujarat | Western | 5.2% | 1.62 to 1.35 | 1.25 | 5.5% | 7.2% |
| Tamil Nadu | Southern | 5.5% | 1.68 to 1.38 | 1.27 | 4.8% | 6.3% |
| Delhi | Northern | 1.6% | 2.75 to 1.87 | 1.59 | 1.9% | 3.2% |

- Generation per person relative to India's runs from 0.59 (Bihar) to 1.77 (Goa) in 2047. It equals relative income to the power 0.736: the elasticity 0.828 less gamma x kappa (1.0 x 0.092), the income-linked part of the urbanisation term, which the split keeps because the dataset has no state urban share ([methodology.md](methodology.md), section 4).
- Summed by region, the state table gives Northern 30.0, Western 26.0, Southern 22.5, Eastern 18.1 and North-Eastern 3.4 percent of the requirement, within 0.3 points of the Monte Carlo regional medians (29.9, 26.1, 22.7, 17.9 and 3.4 percent).

### 2.3 State panel check: why the elasticity is not estimated state by state

`outputs/state_panel_check.csv` is a diagnostic only, never used by the forecast.
It regresses ln(NPP state generation per person) on ln(GSDP per person) with state fixed effects, FY2018-19 to FY2023-24.

| Quantity | Value |
| --- | --- |
| Within slope (s.e., not clustered by state) | 0.434 (0.242) |
| State-years; states | 188; 32 (Ladakh skipped: not in the dataset; Goa: no NPP generation) |
| National elasticity, for comparison | 0.828 (s.e. 0.063) |
| National conventional generation per person, FY2018-19 to FY2023-24 | 979 kWh |
| State generation per person relative to national | 0.08 (Nagaland) to 15.9 (Sikkim); Himachal Pradesh 5.2, Chhattisgarh 4.7, Delhi 0.26, Kerala 0.20 |
| Share of the variance of ln generation per person between states | 97.1% |

NPP state generation covers conventional stations only and is recorded where the plant stands, so a state's figure measures where plants are, not what the state needs.
The fixed effects absorb 97 percent of the variance, and the slope rests on six years of movement within states, which include the pandemic year and new plants.
Its standard error is almost four times the national one, and the national 0.828 lies within two of its standard errors.
The elasticity is therefore estimated where generation and need coincide, India as a whole, and applied state by state (section 2.2).

## 3. 2047 generation strategy

![2047 generation mix](img/3_generation_mix_2047.png)

The dashboard image shows the recommended Robust plan (section 3.2).
Every plan starts from the FY2023-24 regional mix (NPP conventional generation plus CEA renewables) and is bounded by published build limits and committed builds (section 3.7).
These published figures are used in Part 2 only, following the kickoff guidance, and each is labelled secondary; none enters the requirement.

### 3.1 Least-cost plan, base policy

From `outputs/strategy_2047.csv` and `outputs/strategy_2047_regions.csv` (TWh):

| Region | Coal | Gas | Nuclear | Hydro | Solar | Onshore wind | Offshore wind | Bioenergy | Requirement | Net export | Marginal cost (USD/MWh) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Northern | 410 | 0 | 122 | 147 | 504 | 153 | 0 | 18 | 1,406 | -52 | 107.7 |
| Western | 472 | 0 | 170 | 18 | 608 | 246 | 18 | 0 | 1,227 | +306 | 103.9 |
| Southern | 150 | 0 | 130 | 34 | 458 | 233 | 88 | 0 | 1,069 | +23 | 103.9 |
| Eastern | 374 | 0 | 49 | 28 | 117 | 10 | 0 | 9 | 840 | -253 | 108.5 |
| North-Eastern | 8 | 0 | 0 | 120 | 20 | 0 | 0 | 1 | 161 | -11 | 112.2 |

National mix: solar 36.2%, coal 30.0%, onshore wind 13.6%, nuclear 10.0%, hydro 7.3%, offshore wind 2.3%, bioenergy 0.6%; no gas.
Non-fossil share 70.0 percent (the minimum binds, so coal fills the remaining 30 percent); CO2 1,374 Mt; about 215 billion USD a year.
Nuclear sits at its 10 percent share limit, above the committed floor; hydro sits at its usable potential (346 TWh).

**The base policy meets the median requirement in every region.**
The Eastern region imports 253 TWh: the West-East corridor carries 240 TWh before losses, all of its capability, and the South sends 23 TWh.
The West also sends 66 TWh to the North, and the North-Eastern region imports 12 TWh from the North.
Regional marginal costs (the duals of the regional balances) are 104 to 112 USD/MWh.
The plan has no margin for a higher requirement: planned for the 90th percentile (6,400 TWh) it leaves 1,463 TWh unmet (section 4.2).

### 3.2 Recommended plan: Robust policy

| Source | Share of 2047 generation |
| --- | --- |
| Solar | 50.7% |
| Coal | 19.4% |
| Firmed RE (solar and wind with storage, round-the-clock) | 14.2% |
| Hydro | 7.3% |
| Nuclear | 2.8% |
| Offshore wind | 2.4% |
| Onshore wind | 2.1% |
| Gas | 1.1% |

| Region | Coal | Gas | Nuclear | Hydro | Solar | Wind (on- and offshore) | Firmed RE | Total | Requirement | Net export |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Northern | 154 | 12 | 52 | 147 | 706 | 8 | 175 | 1,253 | 1,406 | -153 |
| Western | 260 | 20 | 24 | 18 | 851 | 110 | 242 | 1,525 | 1,227 | +299 |
| Southern | 123 | 4 | 57 | 34 | 641 | 93 | 243 | 1,194 | 1,069 | +125 |
| Eastern | 374 | 1 | 0 | 28 | 164 | 0 | 12 | 578 | 840 | -262 |
| North-Eastern | 3 | 16 | 0 | 120 | 29 | 0 | under 1 | 167 | 161 | +6 |

TWh. The six sources by region are `plan.vectors` in `outputs/site_summary.json`; totals, requirements and net exports are `plan.regions`; firmed RE is the regional total less the six sources (Robust has no bioenergy). National shares are in `outputs/presets.csv`.

Each region uses its strengths, as published resources and today's fleet suggest:

- **Northern:** Rajasthan solar and Himalayan hydro; it imports 153 TWh (152 TWh from the West and 6 TWh from the North-East before losses).
- **Western:** Gujarat and Maharashtra solar and wind; the largest exporter (299 TWh) and the largest gas region (20 TWh, 38 percent of the gas kept for peaking).
- **Southern:** solar, wind and the largest nuclear share (57 TWh, Kudankulam and Kalpakkam); it exports 125 TWh to the East.
- **Eastern:** the coal fleet (374 TWh, 41 percent of the plan's coal); it imports 262 TWh (147 TWh from the West and 125 TWh from the South before losses).
- **North-Eastern:** Arunachal hydro (120 TWh) and Assam and Tripura gas (16 TWh); it exports 6 TWh.

Non-fossil 79.5 percent; CO2 912 Mt; about 177.5 billion USD a year; **no unmet energy at the median**.
It is cheaper (by 38 billion USD a year) and cleaner (by 462 Mt of CO2) than the base policy, and it leaves 633 TWh unmet at P90 instead of 1,463 TWh.
It needs 35 percent of the solar potential usable (including rooftop and agri-PV), 30 percent of the wind potential, corridors three times the 2032 plan and build rates at the published high-case limits; its coal floor is 50 percent of FY2023-24 coal, and it keeps the existing gas fleet at its best NPP year (51 TWh) for peaking.
Nuclear is at its committed floor in every region (133 TWh); hydro is at its usable potential (346 TWh).

### 3.3 What the P90 requirement takes

Planned for the 90th-percentile requirement (6,400 TWh), the Robust policy leaves 633 TWh unmet.
Land for solar is the binding constraint (`outputs/p90_levers.csv`, unmet TWh):

| Build limits (x the published high case) | 35% usable solar | 45% | 55% |
| --- | --- | --- | --- |
| 1.0 | 633 | 0 | 0 |
| 1.25 | 420 | 0 | 0 |
| 1.5 | 265 | 0 | 0 |
| 2.0 | 219 | 0 | 0 |

Faster building alone does not close the gap: twice the published high-case limits still leaves 219 TWh unmet.
Usable solar land (rooftop, agri-PV, canal-top and floating solar) does: 45 percent of the potential meets the P90 already at the published high-case limits, for about 258 billion USD a year (244 billion with 55 percent).

### 3.4 Scenario and policy presets

Each scenario preset is crossed with each policy preset ([assumptions_and_uncertainty.md](assumptions_and_uncertainty.md), section 6; full table in `outputs/presets.csv`).

| Scenario | Policy | 2047 median (TWh) | Non-fossil | Coal | CO2 (Mt) | Cost (bn USD) | Unmet at median (TWh) | Unmet at P90 (TWh) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Base | Base | 4,703 | 70.0% | 30.0% | 1,374 | 215 | 0 | 1,463 |
| Base | Low carbon | 4,703 | 81.7% | 18.1% | 832 | 193 | 0 | 1,486 |
| Base | **Robust** | 4,703 | **79.5%** | **19.4%** | **912** | **178** | **0** | **633** |
| Base | Energy security | 4,703 | 70.0% | 30.0% | 1,374 | 216 | 0 | 1,463 |
| High growth | Base | 6,785 | 70.1% | 28.9% | 1,407 | 237 | 1,847 | 4,337 |
| High growth | Robust | 6,785 | 75.0% | 24.1% | 1,375 | 254 | 1,016 | 3,506 |
| Low growth | Base | 3,397 | 77.3% | 22.7% | 754 | 129 | 0 | 0 |
| Low growth | Robust | 3,397 | 79.5% | 19.0% | 653 | 125 | 0 | 0 |

In the base scenario every policy meets the median; they differ in cost, CO2 and the shortfall at P90.
The Robust policy is the cheapest and leaves the smallest P90 shortfall; the Low carbon policy emits least.

**Stress test at the 5th percentile** (`outputs/presets.csv`, columns `unserved_at_p05_twh` and `cost_at_p05_busd`).
Every policy has a feasible plan at the Base scenario's P05 (3,217 TWh) and at every scenario's median.
At the Low growth scenario's P05 (2,332 TWh), the Base, Robust and Energy security policies have no feasible plan; Low carbon has one (86.9 billion USD a year).
The reason is a known limit ([decisions.md](decisions.md), D13): the coal floor (for Robust, 50 percent of each region's FY2023-24 coal, 647 TWh) and the gas kept for peaking (51 TWh) are fixed in TWh, while the non-fossil minimum (75 percent) is a share of generation.
Below about (647 + 51) / 0.25 = 2,794 TWh of generation the two floors exceed the fossil allowance; the lowest feasible requirement found by bisection is 2,780 TWh.
The Base policy's bound is about 2,590 TWh and Energy security's about 3,020 TWh. The dashboard explains the infeasible state and names the levers: the coal floor, the gas kept for peaking and the non-fossil minimum.
As at the midpoint, High growth with the Energy security policy has no feasible plan at P90.

### 3.5 Cost of deeper CO2 cuts

The recommended plan (Robust policy, median requirement) is re-solved under a falling national CO2 cap ([methodology.md](methodology.md), section 6.5; `outputs/pareto_2047.csv`).

| CO2 (Mt) | Cost (bn USD/yr) | Marginal abatement cost (USD/t) | Non-fossil | Coal | Nuclear |
| --- | --- | --- | --- | --- | --- |
| 912 (no cap) | 177.51 | 0 | 79.5% | 19.4% | 2.8% |
| 890 | 177.52 | 1.03 | 80.0% | 18.9% | 3.3% |
| 869 | 177.55 | 1.03 | 80.5% | 18.4% | 3.8% |
| 847 | 177.57 | 1.03 | 81.0% | 18.0% | 4.2% |
| 825 | 177.60 | 1.40 | 81.4% | 17.5% | 4.7% |
| 804 | 177.63 | 1.40 | 81.9% | 17.0% | 5.2% |
| 782 | 177.66 | 1.40 | 82.4% | 16.5% | 5.7% |
| 761 | 177.69 | 1.40 | 82.8% | 16.1% | 6.1% |
| 739 | 177.72 | 1.40 | 83.3% | 15.6% | 6.6% |
| 717 | 177.75 | 1.40 | 83.8% | 15.1% | 7.1% |
| 696 | 177.82 | 4.04 | 84.3% | 14.7% | 7.6% |
| 674 | 177.89 | 4.04 | 84.7% | 14.2% | 8.0% |
| **653** | **177.97** | **4.04** | **85.2%** | **13.7%** | **8.5%** |

- **Deeper cuts cost almost nothing.** Cutting the plan's CO2 from 912 to 653 Mt a year (28 percent) raises its cost from 177.51 to 177.97 billion USD a year: 0.46 billion USD, or 0.3 percent.
- The marginal abatement cost rises in steps from 1.03 to 4.04 USD per tonne; the average over the whole cut is about 1.8 USD per tonne.
- Nuclear replaces coal: its share rises from 2.8 to 8.5 percent while coal falls from 19.4 to 13.7 percent.
  Solar (50.7 percent) and firmed RE (14.2 percent) hardly change, because solar is already at its usable land limit, and gas stays at 1.1 percent, the fleet kept for peaking.
- **The coal fleet floor is the limit.** At 653 Mt every region's coal is at the Robust policy's floor of 50 percent of its FY2023-24 output (647 TWh nationally, 629 Mt at the CEA v22 factor of 0.971 t CO2 per MWh), because the fleet is young; the gas kept for peaking adds 24 Mt (51 TWh at 0.470).
  Any lower cap could only be met by leaving requirement unserved.

### 3.6 Storage and land the plans need

Each least-cost plan is dispatched hour by hour over a representative year built from NPP Merit hourly generation ([methodology.md](methodology.md), section 6.4; full table in `outputs/storage_land_2047.csv`).
Storage power and energy are physical sizes of the plan, not forecasts.

| Plan (2047 requirement) | Storage power | Storage energy (daily cycle) | Duration | Curtailed solar and wind | Shortfall left after storage | Solar and wind land | Share of India's area |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **Robust policy, median (4,703 TWh)** | **448 GW** | **3,044 GWh** | **6.8 h** | **3.2%** | **226 TWh (4.8%)** | **19,990 km²** | **0.61%** |
| Base policy, median (4,703 TWh) | 256 GW | 1,744 GWh | 6.8 h | 4.7% | 172 TWh (3.7%) | 14,403 km² | 0.44% |
| Robust policy at P90 with 45% usable solar, at the published high-case build limits (6,400 TWh) | 577 GW | 4,051 GWh | 7.0 h | 2.1% | 243 TWh (3.8%) | 25,402 km² | 0.77% |

**Storage.**
Before storage, load exceeds supply in about 5,400 hours a year in the Robust plan, mostly evenings and nights after the solar peak.
The design day is 12 April 2026, before the monsoon, with high load, strong sun and little wind; the median day needs 2,381 GWh.
The store cycles about 250 times a year, discharges 770 TWh (16 percent of the requirement) and loses 136 TWh in the round trip.
In the Robust plan, plain solar is already at its usable limit, so the firmed RE that the LP builds uses wind potential and counts here as 672 TWh of onshore wind.
If firmed RE is instead treated as a round-the-clock block with its storage inside the plants, the rest of the system needs 464 GW and 3,248 GWh.
The two views put the Robust plan's storage at about 448 to 464 GW and 3.0 to 3.2 TWh.

**Gas kept for peaking.**
The Robust plan's 51 TWh of gas runs 2,698 hours a year, at most 21.4 GW, and 25.0 TWh of it (49 percent) falls between 18:00 and 24:00 IST, after the solar peak.

**Multi-day storage does not pay.**
Carrying energy between days helps little.
In the Robust plan, doubling the store's energy cuts the remaining shortfall only from 222 to 209 TWh, so each extra GWh serves about 4 GWh a year.
The multi-day size therefore equals the daily size; the remaining shortfall is weather over weeks and seasons, which firm generation has to cover.
Unlimited charging power would save only 12 TWh, so the power rating is not the limit either.

**The plans need about 4 to 5 percent more generation.**
The shortfall left after storage is the storage losses (136 TWh in the Robust plan) plus days when the sun and wind fall short (about 90 TWh).
Annual energy balances in the LP do not see either, so the plan needs that much more generation, for example more solar with storage or longer coal running.
With a stiffer coal fleet (minimum 55 percent instead of 40 percent) the Robust plan needs 468 GW and 3,206 GWh.
With a 75 percent round trip, typical of pumped storage, the shortfall left rises to 299 TWh.

**Comparison with CEA.**
CEA's optimal mix for 2029-30 needs 60.6 GW and 336 GWh of storage (18.98 GW pumped storage and 41.65 GW batteries) for 2,441 TWh of generation with 31 percent solar and wind ([CEA, April 2023](https://jmkresearch.com/wp-content/uploads/2023/05/Report_on_Optimal_Generation_Mix_in_year_2030_1683189805.pdf), section 8.1.2).
That is 0.44 GWh of storage per TWh of solar and wind and 5.5 hours.
The 2047 plans need 0.71 (Base) to 0.93 (Robust) GWh per TWh at about 6.8 hours.
Storage energy grows 5 to 9 times from 2029-30 while solar and wind generation grows 3 to 4 times, as expected when solar's share rises.
The Ministry of Power's Annual Report 2025-26 already projects 62 GW of pumped storage (6 to 7 hours) and 99 GW / 396 GWh of batteries for 2034-35 (`data/external/techno_economic/policy_targets.csv`).

**Land.**
The Robust plan's solar needs 18,942 km² at India's norm of 4 acres per MW, or 32,887 km² at NREL's US average, and the wind turbines take 1,047 km².
Together that is 0.61 percent of India's area and 3.6 percent of its wasteland.
Wind farms also spread over about 120,000 km², most of which stays in farming.
The densest regions are the Southern (0.85 percent of its area), Western (0.74 percent) and Northern (0.58 percent).
The solar uses 35 percent of the NISE ground-mounted solar potential, all of the usable share in the Robust policy.
Meeting the P90 requirement with 45 percent of that potential raises the land to 25,402 km² (0.77 percent).
All solar is counted as ground-mounted; rooftop and agri-PV would reduce the land.

### 3.7 Published figures in the mix

Following the kickoff guidance, publicly available literature sets the growth rates and constraints of each source in Part 2 ([methodology.md](methodology.md), section 6.1).
Asked whether published potentials, costs and emission factors may act as limits and prices in the mix, the organisers replied "Use insights from this data source"; a clarification has been requested and no reply has come yet ([decisions.md](decisions.md), D12).

**Base mix, FY2023-24** (TWh; `outputs/strategy_2047.csv`, column `base_fy2023_24_twh`):

| Region | Coal | Gas | Nuclear | Hydro | Solar | Wind | Bioenergy | Total |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Northern | 273.5 | 7.3 | 9.9 | 77.4 | 46.6 | 8.4 | 5.5 | 428.6 |
| Western | 520.6 | 12.2 | 15.7 | 18.3 | 24.3 | 38.0 | 5.5 | 634.5 |
| Southern | 246.0 | 2.2 | 22.3 | 22.8 | 43.5 | 37.0 | 4.3 | 378.2 |
| Eastern | 249.7 | 0.3 | 0 | 18.4 | 1.2 | 0 | 1.8 | 271.4 |
| North-Eastern | 5.1 | 9.7 | 0 | 6.6 | 0.3 | 0 | 0.0 | 21.7 |
| **India** | **1,294.9** | **31.7** | **47.9** | **143.5** | **116.0** | **83.4** | **17.0** | **1,734.4** |

- Coal, gas, nuclear and hydro (stations above 25 MW) are NPP regional generation, 1,508.5 TWh (primary); thermal is split into coal and gas by NPP's own station sums.
- Solar, wind, bioenergy and small hydro (9.5 TWh, inside hydro) are CEA General Review Table 4.7 state renewables, 225.8 TWh (secondary), because NPP has no renewable generation by region or state.
- The total is 36 TWh above NPP's national 1,698 TWh, because CEA's renewables are 36.3 TWh above NPP's renewable total.
- At the midpoint the base mix was Ember's CY2024 state data (1,801.2 TWh, secondary); Ember is now a cross-check only. The move to FY2023-24 lowers the coal floors slightly: at the base policy's 60 percent, from 796.8 to 776.9 TWh.

**Published build limits.** Each source's 2047 generation is capped at its starting level plus the published additions to 2047 (CEA NEP 2022-32, the CEA input slides of Dec 2024, NITI Aayog 2026, the DAE nuclear target and the MoP hydro schedule; secondary), converted from GW a year with each source's capacity factor.
The starting level is the larger of FY2023-24 and the existing fleet's best NPP year from FY2018-19 to FY2025-26: coal 1,331.9, gas 51.1, nuclear 56.7 and hydro 176.7 TWh.
This replaces the midpoint's rule of the record annual increase (Ember) times an acceleration factor: the Robust policy moves from 2.5 times the record year to 1.0 times the published high case.
In the Base and High growth plans, hydro is limited by its usable potential (346 TWh), not by the published additions.

**Committed builds** (`outputs/committed_builds_2047.csv`, Robust policy, TWh unless stated):

| Region | Nuclear, FY2023-24 | Committed nuclear, MW | Nuclear floor | Nuclear in the plan | Hydro, FY2023-24 | Committed large hydro, MW | Hydro floor | Hydro in the plan |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Northern | 9.9 | 7,000 | 51.6 | 51.6 | 77.4 | 6,177.5 | 96.9 | 146.7 |
| Western | 15.7 | 1,400 | 24.0 | 24.0 | 18.3 | 0 | 18.3 | 18.3 |
| Southern | 22.3 | 5,900 | 57.5 | 57.5 | 22.8 | 1,230 | 26.7 | 33.6 |
| Eastern | 0 | 0 | 0 | 0 | 18.4 | 740 | 20.8 | 27.9 |
| North-Eastern | 0 | 0 | 0 | 0 | 6.6 | 4,876 | 22.0 | 119.6 |
| **India** | **47.9** | **14,300** | **133.1** | **133.1** | **143.5** | **13,023.5** | **184.6** | **346.1** |

- Committed nuclear (under construction and sanctioned, plus RAPP-7) adds 85.2 TWh at CUF 0.68; committed large hydro (under construction) adds 41.1 TWh at CUF 0.36. MW are converted to TWh only to bound generation; installed capacity is not a forecast metric.
- The floors apply in every policy. In the Robust plan the nuclear floor binds in every region; in the base policy nuclear is at its 10 percent share limit (470 TWh), above the floor.
- The Robust plan keeps the gas fleet at its best NPP year (51.07 TWh, FY2020-21), sited by the FY2023-24 gas shares: Western 19.6, North-Eastern 15.7, Northern 11.7, Southern 3.6 and Eastern 0.5 TWh.

**Emission factors:** CEA CO2 Baseline Database v22.0 (FY2025-26): coal 0.971 and gas 0.470 tCO2/MWh (secondary).

**How the Robust plan moved from the midpoint** ([decisions.md](decisions.md), D13): non-fossil 78.3 to 79.5 percent, CO2 993 to 912 Mt, cost 175.9 to 177.5 billion USD a year, unmet at P90 638 to 633 TWh.
The committed nuclear (85 TWh) and the gas kept for peaking (51 TWh) push coal out at the margin.

### 3.8 Mix sensitivity of the recommended plan

`outputs/sensitivity_mix.csv` re-solves the Robust plan with one published input changed at a time ([methodology.md](methodology.md), section 6.6):

| Case | Non-fossil | Coal | CO2 (Mt) | Cost (bn USD/yr) | Unmet at median (TWh) | Unmet at P90 (TWh) |
| --- | --- | --- | --- | --- | --- | --- |
| Robust as given | 79.5% | 19.4% | 912 | 177.5 | 0 | 633 |
| CEA cost basis (cost in its own basis) | 75.0% | 23.9% | 1,118 | 250.3 | 0 | 633 |
| NISE 2014 solar potential | 75.0% | 23.4% | 772 | 181.3 | 1,418 | 3,114 |
| NIWE 120 m wind potential | 75.0% | 23.9% | 1,118 | 183.7 | 0 | 1,044 |
| Low-case build limits | 75.0% | 23.4% | 739 | 152.7 | 1,555 | 3,252 |
| Central-case build limits | 79.4% | 19.5% | 918 | 177.5 | 0 | 703 |

- The NISE 2014 and low-case rows cannot meet the median requirement. Their lower CO2 and cost describe less generation, not a better plan.
- The CEA cost basis applies nominal CERC rates to real 2021-22 NEP capital costs, so its cost level is not comparable with the IEA basis; it shows only the direction of the mix. With CEA's dearer solar the plan stops at the 75 percent non-fossil minimum and coal rises to 23.9 percent.
- On the central-case build limits the Robust plan still meets the median, and leaves 70 TWh more unmet at P90 than on the high case.

## 4. Sensitivity

![Uncertainty and sensitivity](img/5_uncertainty_sensitivity.png)

### 4.1 Requirement (FY2047-48, `outputs/sensitivity_2047.csv`)

| Case | P5 | Median | P95 | Median vs base |
| --- | --- | --- | --- | --- |
| Base | 3,217 | 4,703 | 7,002 | |
| Income growth 1.5 points above history | 4,192 | 6,169 | 9,315 | +31% |
| Income growth 1.5 points below history | 2,455 | 3,588 | 5,286 | -24% |
| Elasticity falls 0.2 per unit of ln income | 3,089 | 4,197 | 5,643 | -11% |
| Urban share 5 points above the UN path | 3,360 | 4,954 | 7,402 | +5% |
| No urban term (two datasets only) | 2,950 | 4,453 | 6,871 | -5% |
| Population uncertainty doubled | 3,148 | 4,694 | 7,104 | 0% |
| Damped trend (no drivers) | 2,807 | 3,604 | 4,627 | -23% |
| Elasticity one OLS s.e. lower (-0.063) | 3,094 | 4,400 | 6,379 | -6.4% |
| Elasticity one OLS s.e. higher (+0.063) | 3,345 | 5,027 | 7,689 | +6.9% |
| Elasticity from the FY2011-12 window only | 3,168 | 4,583 | 6,835 | -2.6% |
| Elasticity from the FY2006-07 window only | 3,278 | 4,829 | 7,140 | +2.7% |
| Elasticity recentred on the first-difference estimate | 3,091 | 4,392 | 6,363 | -6.6% |
| New loads: published green hydrogen added (outside the headline, section 1.6) | 3,991 | 5,537 | 7,861 | +17.7% |

**Income growth dominates.** The elasticity and the urbanisation term move the median by 3 to 11 percent; the income path moves it by a quarter to a third.
The elasticity is estimated for India as a whole and applied state by state (section 2.2); each 0.01 of elasticity moves the median by about 1 percent.

### 4.2 Policy settings (base plan: 1,374 Mt CO2, 215 billion USD, no unmet energy)

From `outputs/tornado.csv`:

| Setting (low / high) | CO2 (Mt) | Unmet (TWh) | Cost (bn USD) |
| --- | --- | --- | --- |
| Requirement (P10 / P90) | 754 / 1,407 | 0 / **1,463** | 134 / 236 |
| Usable solar land (15% / 35%) | 1,110 / 1,067 | **891** / 0 | 192 / 179 |
| Usable wind land (15% / 35%) | 1,276 / 1,363 | 284 / 0 | 205 / 207 |
| Build limits (0.75x / 1.50x the published central case) | 1,338 / 1,374 | 70 / 0 | 213 / 213 |
| Gas kept for peaking (0% / 100% of the fleet floor) | 1,374 / 1,348 | 0 / 0 | 215 / 216 |
| Carbon price (0 / 60 USD/t) | 1,374 / 1,297 | 0 / 0 | 215 / 219 |
| Coal cost (0.7x / 1.3x) | 1,374 / 1,368 | 0 / 0 | 186 / 245 |
| Non-fossil minimum (60% / 85%) | 1,392 / infeasible | 0 / - | 214 / - |

At the median, land and build limits decide adequacy: 15 percent usable solar or wind land, or 0.75 times the published build limits, leaves requirement unmet, while corridors, the gas kept for peaking and the carbon price move only cost and CO2.
Lower CO2 with 15 percent solar land or slower building is not an improvement: it comes from unmet requirement.
More usable solar land (35 percent) is also the largest cost saving, 215 to 179 billion USD a year.

## 5. Performance

| Run | 1,048,576 scenarios | Scenarios per second |
| --- | --- | --- |
| CPU, 1 thread | 19.85 s | 53,000 |
| CPU, 16 threads (OpenMP) | 4.15 s | 253,000 |
| GPU, RTX 3070 Ti Laptop (CUDA) | 7.75 ms | 135 million |

These are from `outputs/timings.csv` of the current run (the single-thread time is scaled from a 1/16 run). Timings vary by run and machine load.
The India-only core fits in under 0.2 s at start-up (2,000 block-bootstrap replicates in a few milliseconds); the default 4.2 million scenarios are four times the table's count.
