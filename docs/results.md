# Results

All numbers are energy generated (TWh).
They come from the default scenario of the primary (India-only) core: 4,194,304 Monte Carlo scenarios, seed 20470815.
Regenerate them with `energy2047 <repo_root>`; the tables are written to `outputs/`.

Years are fiscal years labelled by their start year: 2023 is FY2023-24, 2047 is FY2047-48.
The scope is NPP utility generation; captive plants are reported as a separate layer (section 1.3).

> **Change, 16 November 2026.** The organisers ruled at the kickoff that the core 2047 prediction must use only the two challenge datasets (National Power Portal and the Renewable India dataset).
> The primary forecast is now the India-only model ([methodology.md](methodology.md), section 2).
> The earlier cross-country model is kept as an independent check (section 1.4).
>
> **Update, 16 November 2026, after an independent code review.** The base-year anchor rule was replaced by an AR(1) model-error term, GSDP values copied unchanged from the previous year are no longer counted as zero income growth, and the bootstrap was corrected ([decisions.md](decisions.md), D11).
> Every number below is from the run that includes these changes.

![National forecast](img/1_national_forecast.png)

## 1. National generation requirement

India's utilities generated 1,698 TWh in FY2023-24 (NPP), or 1,225 kWh per person.

| Year | P5 | P10 | P25 | Median | P75 | P90 | P95 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| FY2030-31 | 1,955 | 2,019 | 2,132 | 2,266 | 2,411 | 2,553 | 2,644 |
| FY2035-36 | 2,316 | 2,430 | 2,633 | 2,884 | 3,165 | 3,449 | 3,634 |
| FY2040-41 | 2,695 | 2,870 | 3,190 | 3,594 | 4,061 | 4,542 | 4,864 |
| **FY2047-48** | **3,217** | **3,497** | **4,018** | **4,703** | **5,521** | **6,400** | **7,002** |

- The median requirement for FY2047-48 is **4,703 TWh**, 2.77 times FY2023-24 (4.3 percent a year).
- Generation per person rises to about 2,930 kWh.
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

### 1.3 Captive layer

Captive plants generated 224 TWh in FY2023-24 (CEA General Review 2025), 13.2 percent of NPP utility generation.
Holding that share gives **about 5,320 TWh including captive plants** in FY2047-48 (P5 3,642, P95 7,926).
This layer is reported next to the forecast and never enters it.

### 1.4 Independent check: cross-country model

| Core | Scope | Base | 2047 P5 | 2047 median | 2047 P95 |
| --- | --- | --- | --- | --- | --- |
| India-only (primary) | NPP utilities, FY2047-48 | 1,698 (FY2023-24) | 3,217 | 4,703 | 7,002 |
| India-only + captive layer | utilities + captive | 1,922 | 3,642 | 5,323 | 7,926 |
| Cross-country panel (check) | Ember total incl. captive, calendar 2047 | 2,080 (2025) | 3,348 | 4,528 | 5,610 |

The cross-country median lies inside the India-only range but about 15 percent below the India-only total with captive plants.
The difference is the elasticity: across 153 countries the elasticity at India's income is 0.44 to 0.57, falling with income; India's own data show 0.78 to 0.87 with no decline yet.
Setting an elasticity decline of 0.2 per unit of ln income in the India model gives 4,197 TWh (section 4), which brings the two methods together.

**CEA plan.** The CEA National Electricity Plan projects gross generation of 2,666 TWh for FY2031-32.
Our median for FY2031-32 is 2,382 TWh (P75 2,551, P90 2,717).

The registration-form figure of 6,000 TWh was a pre-data estimate; including captive plants it lies between our median (5,323) and P75 (6,249).

## 2. Regional requirement

![Regional requirement](img/2_regional_requirement.png)

| Region | FY2030-31 median | FY2040-41 median | FY2047-48 P5 | FY2047-48 median | FY2047-48 P95 | Share of requirement 2047 | Share of NPP conventional generation FY2025-26 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Northern | 626 | 1,011 | 921 | 1,341 | 1,992 | 28.5% | 24.9% |
| Western | 596 | 948 | 852 | 1,248 | 1,867 | 26.6% | 36.0% |
| Southern | 615 | 916 | 766 | 1,144 | 1,740 | 24.3% | 20.1% |
| Eastern | 356 | 597 | 543 | 805 | 1,202 | 17.1% | 17.7% |
| North-Eastern | 73 | 121 | 110 | 161 | 239 | 3.4% | 1.3% |

The requirement is where power is needed (state population and income); NPP generation is where plants are today.

- **Transfer gap.** The Western region produces 36 percent of India's conventional generation but accounts for 27 percent of the 2047 requirement; the Northern and Southern regions need more than they produce today.
- The **Northern** region has the largest requirement because of its population (Uttar Pradesh, Rajasthan).
- The **North-Eastern** share more than doubles relative to today's production share, driven by income convergence and urbanisation.

## 3. 2047 generation strategy

![2047 generation mix](img/3_generation_mix_2047.png)

The dashboard image shows the recommended Robust plan (section 3.2).

### 3.1 Least-cost plan, base policy

| Region | Coal | Gas | Nuclear | Hydro | Solar | Onshore wind | Offshore wind | Bioenergy | Requirement | Net export | Marginal cost (USD/MWh) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Northern | 431 | 0 | 122 | 147 | 504 | 153 | 0 | 0 | 1,342 | +14 | 72.5 |
| Western | 368 | 0 | 170 | 20 | 608 | 246 | 58 | 0 | 1,249 | +220 | 69.7 |
| Southern | 217 | 0 | 114 | 34 | 458 | 233 | 88 | 0 | 1,145 | 0 | 69.7 |
| Eastern | 390 | 0 | 49 | 28 | 117 | 10 | 0 | 0 | 806 | -212 | 72.9 |
| North-Eastern | 7 | 0 | 0 | 120 | 20 | 0 | 0 | 0 | 161 | -13 | 75.6 |

National mix: solar 36.2%, coal 30.0%, onshore wind 13.6%, nuclear 9.7%, hydro 7.4%, offshore wind 3.1%; no gas or bioenergy.
Non-fossil share 70.0 percent (the minimum binds, so coal fills the remaining 30 percent); CO2 1,370 Mt; about 213 billion USD a year.

**The base policy meets the median requirement in every region.**
The Eastern region imports 212 TWh over the West-East corridor, which carries 220 TWh before losses, 92 percent of its capability; the North-Eastern region imports 13 TWh from the North.
Regional marginal costs are 70 to 76 USD/MWh.
The plan has no margin for a higher requirement: planned for the 90th percentile (6,400 TWh) it leaves 1,529 TWh unmet (section 4.2).

### 3.2 Recommended plan: Robust policy

| Source | Share of 2047 generation |
| --- | --- |
| Solar | 50.7% |
| Coal | 21.7% |
| Firmed RE (solar and wind with storage, round-the-clock) | 14.6% |
| Hydro | 7.4% |
| Offshore wind | 2.7% |
| Onshore wind | 1.7% |
| Nuclear | 1.2% |

| Region | Coal | Nuclear | Hydro | Solar | Onshore wind | Offshore wind | Firmed RE | Total | Requirement | Net export | Marginal cost (USD/MWh) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Northern | 239 | 11 | 147 | 706 | 7 | 0 | 176 | 1,286 | 1,342 | -56 | 60.5 |
| Western | 264 | 20 | 20 | 851 | 37 | 58 | 257 | 1,507 | 1,249 | +258 | 58.1 |
| Southern | 124 | 24 | 34 | 641 | 37 | 68 | 243 | 1,171 | 1,145 | +26 | 58.0 |
| Eastern | 390 | 0 | 28 | 164 | 0 | 0 | 12 | 594 | 806 | -212 | 60.8 |
| North-Eastern | 7 | 0 | 120 | 29 | 0 | 0 | 0 | 156 | 161 | -5 | 63.2 |

Regional detail is from the dashboard's 2047 generation mix view with the Robust preset (`docs/img/3_generation_mix_2047.png`); the totals, net exports and corridor flows are also in `outputs/site_summary.json` (`plan`), and the national shares in `outputs/presets.csv`.

Each region uses its strengths: Rajasthan solar and Himalayan hydro in the North; Gujarat and Maharashtra solar, wind and Gulf of Khambhat offshore wind in the West, the largest exporter (258 TWh); solar, wind and offshore wind in the South; the coal fleet in the East, which imports about 210 TWh (194 TWh from the West and 26 TWh from the South before losses); Arunachal hydro in the North-East.

Non-fossil 78.3 percent; CO2 993 Mt; about 176 billion USD a year; **no unmet energy at the median**.
It is cheaper (by 37 billion USD a year) and cleaner (by 377 Mt of CO2) than the base policy, and it leaves 638 TWh unmet at P90 instead of 1,529 TWh.
It needs 35 percent of the solar potential usable (including rooftop and agri-PV), 30 percent of the wind potential, corridors three times the 2032 plan and build rates 2.5 times India's record year.

### 3.3 What the P90 requirement takes

Planned for the 90th-percentile requirement (6,400 TWh), the Robust policy leaves 638 TWh unmet.
Land for solar is the binding constraint (`outputs/p90_levers.csv`, unmet TWh):

| Build rate (x record year) | 35% usable solar | 45% | 55% |
| --- | --- | --- | --- |
| 2.5 | 638 | 0 | 0 |
| 3.0 | 504 | 0 | 0 |
| 3.5 | 370 | 0 | 0 |
| 4.0 | 238 | 0 | 0 |

Faster building alone does not close the gap: four times the record year still leaves 238 TWh unmet.
Usable solar land (rooftop, agri-PV, canal-top and floating solar) does: 45 percent of the potential meets the P90 at the Robust build rate of 2.5 times the record year, for about 257 billion USD a year (232 billion with 55 percent).

### 3.4 Scenario and policy presets

Each scenario preset is crossed with each policy preset ([assumptions_and_uncertainty.md](assumptions_and_uncertainty.md), section 6; full table in `outputs/presets.csv`).

| Scenario | Policy | 2047 median (TWh) | Non-fossil | Coal | CO2 (Mt) | Cost (bn USD) | Unmet at median (TWh) | Unmet at P90 (TWh) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Base | Base | 4,703 | 70.0% | 30.0% | 1,370 | 213 | 0 | 1,529 |
| Base | Low carbon | 4,703 | 80.1% | 16.4% | 822 | 196 | 0 | 1,551 |
| Base | **Robust** | 4,703 | **78.3%** | **21.7%** | **993** | **176** | **0** | **638** |
| Base | Energy security | 4,703 | 70.0% | 30.0% | 1,370 | 214 | 0 | 1,529 |
| High growth | Base | 6,785 | 70.0% | 30.0% | 1,416 | 236 | 1,914 | 4,403 |
| High growth | Robust | 6,785 | 75.0% | 25.0% | 1,396 | 254 | 1,023 | 3,512 |
| Low growth | Base | 3,397 | 76.6% | 23.4% | 772 | 127 | 0 | 0 |
| Low growth | Robust | 3,397 | 80.5% | 19.5% | 643 | 121 | 0 | 0 |

In the base scenario every policy meets the median; they differ in cost, CO2 and the shortfall at P90.
The Robust policy is the cheapest and leaves the smallest P90 shortfall; the Low carbon policy emits least.

### 3.5 Cost of deeper CO2 cuts

The recommended plan (Robust policy, median requirement) is re-solved under a falling national CO2 cap ([methodology.md](methodology.md), section 6.5; `outputs/pareto_2047.csv`).

| CO2 (Mt) | Cost (bn USD/yr) | Marginal abatement cost (USD/t) | Non-fossil | Coal | Nuclear |
| --- | --- | --- | --- | --- | --- |
| 993 (no cap) | 175.72 | 0 | 78.3% | 21.7% | 1.2% |
| 964 | 175.74 | 0.68 | 78.9% | 21.1% | 1.8% |
| 934 | 175.76 | 0.68 | 79.5% | 20.5% | 2.4% |
| 905 | 175.79 | 1.03 | 80.2% | 19.8% | 3.1% |
| 876 | 175.83 | 1.39 | 80.8% | 19.2% | 3.7% |
| 847 | 175.87 | 1.39 | 81.5% | 18.5% | 4.3% |
| 818 | 175.91 | 1.39 | 82.1% | 17.9% | 5.0% |
| 789 | 175.95 | 1.39 | 82.7% | 17.3% | 5.6% |
| 760 | 176.01 | 3.86 | 83.4% | 16.6% | 6.3% |
| 731 | 176.10 | 3.86 | 84.0% | 16.0% | 6.9% |
| 702 | 176.20 | 3.86 | 84.7% | 15.4% | 7.5% |
| 673 | 176.30 | 3.86 | 85.3% | 14.7% | 8.2% |
| **643** | **176.39** | **3.97** | **85.9%** | **14.1%** | **8.8%** |

- **Deeper cuts cost almost nothing.** Cutting the plan's CO2 from 993 to 643 Mt a year (35 percent) raises its cost from 175.72 to 176.39 billion USD a year: 0.67 billion USD, or 0.4 percent.
- The marginal abatement cost rises in steps from 0.68 to 3.97 USD per tonne; the average over the whole cut is about 1.9 USD per tonne.
- Nuclear replaces coal: its share rises from 1.2 to 8.8 percent while coal falls from 21.7 to 14.1 percent.
  Solar (50.7 percent) and firmed RE (14.6 percent) do not change, because solar is already at its usable land limit.
- **The coal fleet floor is the limit.** At 643 Mt every region's coal is at the Robust policy's floor of 50 percent of its 2024 output (664 TWh nationally, at 0.969 t CO2 per MWh), because the fleet is young.
  Any lower cap could only be met by leaving requirement unserved.

### 3.6 Storage and land the plans need

Each least-cost plan is dispatched hour by hour over a representative year built from NPP Merit hourly generation ([methodology.md](methodology.md), section 6.4; full table in `outputs/storage_land_2047.csv`).
Storage power and energy are physical sizes of the plan, not forecasts.

| Plan (2047 requirement) | Storage power | Storage energy (daily cycle) | Duration | Curtailed solar and wind | Shortfall left after storage | Solar and wind land | Share of India's area |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **Robust policy, median (4,703 TWh)** | **447 GW** | **3,039 GWh** | **6.8 h** | **3.7%** | **243 TWh (5.2%)** | **19,990 km²** | **0.61%** |
| Base policy, median (4,703 TWh) | 254 GW | 1,718 GWh | 6.8 h | 4.9% | 181 TWh (3.9%) | 14,403 km² | 0.44% |
| Robust policy at P90 with 45% usable solar and 3.5x build rates (6,400 TWh) | 580 GW | 4,066 GWh | 7.0 h | 2.5% | 260 TWh (4.1%) | 25,402 km² | 0.77% |

**Storage.**
Before storage, load exceeds supply in about 5,400 hours a year in the Robust plan, mostly evenings and nights after the solar peak.
The design day is 13 April 2026, before the monsoon, with high load, strong sun and little wind; the median day needs 2,369 GWh.
The store cycles about 250 times a year, discharges 758 TWh (16 percent of the requirement) and loses 134 TWh in the round trip.
In the Robust plan, plain solar is already at its usable limit, so the firmed RE that the LP builds uses wind potential and counts here as 689 TWh of onshore wind.
If firmed RE is instead treated as a round-the-clock block with its storage inside the plants, the rest of the system needs 465 GW and 3,291 GWh.
The two views put the Robust plan's storage at about 450 to 465 GW and 3.0 to 3.3 TWh.

**Multi-day storage does not pay.**
Carrying energy between days helps little.
In the Robust plan, doubling the store's energy cuts the remaining shortfall only from 240 to 225 TWh, so each extra GWh serves about 5 GWh a year.
The multi-day size therefore equals the daily size; the remaining shortfall is weather over weeks and seasons, which firm generation has to cover.
Unlimited charging power would save only 12 TWh, so the power rating is not the limit either.

**The plans need about 4 to 5 percent more generation.**
The shortfall left after storage is the storage losses (134 TWh in the Robust plan) plus days when the sun and wind fall short (about 110 TWh).
Annual energy balances in the LP do not see either, so the plan needs that much more generation, for example more solar with storage or longer coal running.
With a stiffer coal fleet (minimum 55 percent instead of 40 percent) the Robust plan needs 467 GW and 3,206 GWh.
With a 75 percent round trip, typical of pumped storage, the shortfall left rises to 314 TWh.

**Comparison with CEA.**
CEA's optimal mix for 2029-30 needs 60.6 GW and 336 GWh of storage (18.98 GW pumped storage and 41.65 GW batteries) for 2,441 TWh of generation with 31 percent solar and wind ([CEA, April 2023](https://jmkresearch.com/wp-content/uploads/2023/05/Report_on_Optimal_Generation_Mix_in_year_2030_1683189805.pdf), section 8.1.2).
That is 0.44 GWh of storage per TWh of solar and wind and 5.5 hours.
The 2047 plans need 0.69 (Base) to 0.92 (Robust) GWh per TWh at about 6.8 hours.
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

**Income growth dominates.** The elasticity and the urbanisation term move the median by 5 to 11 percent; the income path moves it by a quarter to a third.

### 4.2 Policy settings (base plan: 1,370 Mt CO2, 213 billion USD, no unmet energy)

| Setting (low / high) | CO2 (Mt) | Unmet (TWh) | Cost (bn USD) |
| --- | --- | --- | --- |
| Requirement (P10 / P90) | 772 / 1,416 | 0 / **1,529** | 133 / 232 |
| Usable solar land (15% / 35%) | 1,132 / 1,117 | **808** / 0 | 199 / 178 |
| Usable wind land (15% / 35%) | 1,309 / 1,370 | 202 / 0 | 211 / 207 |
| Build-rate acceleration (1.5x / 3.0x) | 1,370 / 1,370 | 0 / 0 | 218 / 213 |
| Carbon price (0 / 60 USD/t) | 1,370 / 1,137 | 0 / 0 | 213 / 225 |
| Coal cost (0.7x / 1.3x) | 1,370 / 1,368 | 0 / 0 | 184 / 242 |
| Non-fossil minimum (60% / 85%) | 1,415 / infeasible | 0 / - | 213 / - |

At the median, only land decides adequacy: 15 percent usable solar or wind land leaves requirement unmet, while build rates, corridors and the carbon price move only cost and CO2.
Lower CO2 with 15 percent solar land is not an improvement: it comes from unmet requirement.
More usable solar land (35 percent) is also the largest cost saving, 213 to 178 billion USD a year.

## 5. Performance

| Run | 1,048,576 scenarios | Scenarios per second |
| --- | --- | --- |
| CPU, 1 thread | 23.4 s | 45,000 |
| CPU, 16 threads (OpenMP) | 2.42 s | 434,000 |
| GPU, RTX 3070 Ti Laptop (CUDA) | 8.4 ms | 125 million |

These were measured on 16 November 2026 (`outputs/timings.csv`; the single-thread time is scaled from a 1/16 run); CPU timings vary with background load.
The India-only core fits in under 0.2 s at start-up (2,000 block-bootstrap replicates in a few milliseconds); 4.2 million scenarios take about 35 ms on the GPU.
