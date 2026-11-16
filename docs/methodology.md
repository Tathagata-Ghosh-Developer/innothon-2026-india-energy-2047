# Methodology

This document explains how the forecast and the regional strategy are built.

| Topic | Document |
| --- | --- |
| Design choices and rejected alternatives | [decisions.md](decisions.md) |
| Data provenance | [data_audit.md](data_audit.md) |
| Assumptions and limitations | [assumptions_and_uncertainty.md](assumptions_and_uncertainty.md) |
| Numbers | [results.md](results.md) |

```text
challenge datasets: NPP generation (FY1946-47..FY2023-24) + organiser state population and GSDP
   |
   v
India elasticity of generation per person on real GSDP per person
(two estimation windows, moving-block bootstrap; urbanisation through a literature prior)
   |                                     cross-country panel (independent check, justification only)
   v                                         |
Monte Carlo ensemble (CPU OpenMP or GPU CUDA): national + 5 regions, FY2024-25..FY2047-48
   |
   v
2047 regional requirement (planning quantile), split by state population (challenge dataset) and income convergence
   |
   v
least-cost LP (HiGHS)  +  resource-score allocation  +  policy sensitivity sweep  +  cost-CO2 trade-off
   |
   v
dashboard (Dear ImGui + ImPlot, native and WebAssembly) and output tables
```

## 1. What is forecast

The target is India's annual electricity generation (TWh) up to 2047, nationally and for the five grid regions.
The headline is the country's total in TWh; generation per person appears only inside the model, as the quantity the regression explains.
Only energy generated is used, as the competition requires.
Installed capacity and consumption never enter the forecast.

**Scope of the primary forecast.**
The National Power Portal (NPP) series is gross generation of utilities (thermal, hydro, nuclear and renewable sources).
Captive plants inside industries are outside it.
The organisers' ruling of 16 November 2026 says to use energy production from the NPP dataset only and no external data for estimation, so captive generation is neither estimated nor reported (section 2.7).

**Years.**
NPP reports fiscal years (April to March).
The primary forecast labels each fiscal year by its start year: 2023 is FY2023-24, the last year of the NPP all-India series, and 2047 is FY2047-48.

**Rule.**
At the kickoff (13 Nov 2026) the organisers said that the core analysis of the 2047 energy prediction must use only the two challenge datasets: the National Power Portal and the Renewable India dataset.
Public literature may inform growth rates and constraints.
Their written ruling of 16 November 2026 ([decisions.md](decisions.md), D12) adds:

| # | Ruling | How it is applied |
| --- | --- | --- |
| 1 | External population and urbanisation projections may be used if reasonably consistent with the state-level population growth in the challenge dataset; UN India-level growth is a reference | UN WPP 2024 medium for India's population, UN WUP 2018 for its urban share; the consistency check is section 4 and `outputs/population_consistency.csv` |
| 2 | The urbanisation coefficient is the team's own judgment | gamma ~ N(1.0, 0.5), truncated at zero, from published studies (section 2.3) |
| 3 | No captive generation; NPP generation only; no external data for estimation | The captive layer is removed (section 2.7) |
| 4 | The cross-country model may be used for justification only, not estimation | Section 3 |
| 5 | The regional split must use the challenge dataset | State populations from the challenge dataset, grid regions from NPP (section 4) |
| 6 | Resource potentials may be used for justification only, not estimation | They never enter the forecast; they bound the supply plan (section 6), a use the team has asked the organisers to confirm |

The primary forecast follows these rules; section 3 describes a cross-country model that is kept only as an independent check.

## 2. Primary forecast: India only

### 2.1 Data

| Item | Source | Coverage | Use |
| --- | --- | --- | --- |
| All-India generation by source (GWh) | NPP dashboard (`dashBoard/get_generation_chart_list`) | FY1946-47 to FY2023-24; annual from FY2006-07 | Generation history |
| State population and GSDP | Renewable India dataset (organiser) | 2006 to 2024, 32 to 34 states | Population and real income per person; state population paths and relative income of the regional split |
| Region and state conventional generation, monthly | NPP, CEA 18-column reports (Sub-reports 2 and 16) | Jul-2018 to Sep-2026 | Holdout check, regional supply today, grid region of each state |
| Hourly generation by source | NPP, Merit India real-time data | Sep-2025 to Nov-2026 | Solar and wind profiles |
| Urban share | UN World Urbanization Prospects 2018 (driver) | 1950 to 2050 | National urbanisation path |
| Population projection | UN WPP 2024 medium, low and high for India (driver) | to 2047 | National population paths; India-level growth on which the state paths are built |

The installed-capacity columns of the organiser dataset are never read.
The Technical Group on Population Projections (2020) is no longer used for the state paths; it appears only as a reference column in the population consistency check (section 4).

**National income per person.**
State GSDP and population are summed over the states reported in both of two consecutive years, and the national series is built from those year-on-year growth rates.
A state that enters or leaves the dataset therefore does not move the series.
Population growth uses every matched state.
Income growth uses only the matched states whose GSDP changed from the year before.
Several states repeat one GSDP figure across years: Telangana, Jammu and Kashmir, Andaman and Nicobar Islands and Lakshadweep for 2006 to 2010, and Nagaland for 2007.
These copies are placeholders, not observations, and counting them as zero growth would understate national growth.
The organiser GSDP changes price base in 2011 (from 2004-05 to 2011-12 prices), so the jump in that year is not growth.
Its growth per person is imputed as the mean of 2010 and 2012 (4.63 percent); the splice requires both neighbouring years.
The 2024 GSDP values are incomplete and are not used.

### 2.2 Model

e = NPP generation per person (kWh), y = real GSDP per person (thousand INR at 2011-12 prices), x = ln(y / 10):

ln e(t) = a + beta x(t) + c D2020(t) + error

D2020 marks FY2020-21, the pandemic year.
beta is the income elasticity of generation per person.
It is the only parameter estimated from the two datasets.

| Estimate | FY2011-12 to FY2023-24 (one price base) | FY2006-07 to FY2023-24 (spliced) |
| --- | --- | --- |
| beta | 0.828 (s.e. 0.063) | 0.866 (s.e. 0.031) |
| beta, first differences (FY2020-21 and FY2021-22 changes left out; also the imputed 2011 change in the spliced window) | 0.782 | 0.787 |
| FY2020-21 effect | -0.055 | |
| Residual s.d. | 0.037 | |
| Durbin-Watson | 0.99 | |
| FY2023-24 residual | +0.070 | |

The first-difference estimates answer the concern that two trending series can produce a spurious fit: growth in generation per person still follows growth in income per person with an elasticity near 0.8.
The imputed 2011 change is left out of the first differences because it is not an observed change.

### 2.3 Urbanisation

Urban share rose with income in India (29.6 percent in FY2006-07, 36.4 percent in FY2023-24), so India's own national data cannot separate the two effects.
Adding urban share to the regression makes both coefficients unstable across estimation windows.
The urban share for the years since the 2011 census is itself an interpolation, so it carries no independent information.

beta therefore already contains the urbanisation that came with income.
What it cannot contain is urbanisation that runs faster than income in the future.
The projection adds only that excess:

ln e(t) - ln e(base) = beta dx + gamma (du - kappa dx)

- kappa = 0.092 is the slope of urban share on x over the same years and regressors, so in the sample the excess urban share is uncorrelated with x and beta does not depend on gamma.
- gamma comes from the literature as a prior: normal with mean 1.0 and s.d. 0.5, truncated at zero (a negative draw is redrawn, not set to zero).
  Liddle (2013) reports an elasticity of residential electricity to urbanisation of 1.02 (s.e. 0.13) in a low-income panel that includes India; Shahbaz et al. (2016) report 0.56 to 0.80 for India's total energy use; Holtedahl and Joutz (2004) report a semi-elasticity of 3.3 to 3.9 for residential electricity in Taiwan.
  Residential use is about a quarter of India's electricity, which puts the total-electricity value near 1.0.
- gamma = 0 is the pure two-dataset run.
  It is a separate model slot in the Monte Carlo and a sensitivity case in every output.

### 2.4 Parameter uncertainty

Residuals are serially correlated (Durbin-Watson 0.99), so the bootstrap resamples circular blocks of three years.
The resampling pool holds the residuals of the years without the pandemic dummy (the dummy year's residual is zero by construction).
They are rescaled by the square root of (number of pooled residuals) / (number of years minus number of regressors), so the bootstrap does not understate the spread of a fit that used up degrees of freedom.
2,000 replicates are drawn: half from the FY2011-12 window and half from the spliced FY2006-07 window, so the choice of window is part of the uncertainty.
The resulting elasticity has a median of 0.850 and a 90 percent interval of 0.731 to 0.939.
Each replicate also draws gamma from its prior.

### 2.5 Base year and model error

The projection starts from observed FY2023-24 generation per person.
FY2023-24 lies 0.070 log points above the fitted line, almost two residual standard deviations (a hot, fast-growth year).
Carrying that residual unchanged to 2047 would raise the whole path; dropping it at once would ignore that the residuals persist.
The Monte Carlo therefore carries the model error of the India core as an AR(1) residual of ln(kWh per person):

r(t) = rho r(t-1) + s eps(t), with r(FY2023-24) = +0.070 and eps(t) standard normal

- rho = 1 - DW / 2 = 0.51, from the Durbin-Watson statistic (0.99).
  With 13 to 18 points a direct lag regression is dominated by the slow drift of the early, supply-constrained years; the Durbin-Watson relation uses every residual.
- s = residual s.d. x sqrt(1 - rho^2) = 0.037 x 0.86 = 0.032, so in steady state the residual has the regression's residual s.d. (0.037).
- Each draw follows the fitted relation plus r(t).
  The base-year residual reverts to the fitted relation: its expected value is 0.036 in FY2024-25 and 0.018 in FY2025-26 (it halves in about a year), while new model error keeps entering every year.
- Central paths (results, section 1) use the expected residual path, 0.070 x rho^k after k years.
- The residual moves the national total only; it is common to every state, so it does not change the regional shares (section 4).

This replaces an earlier threshold rule that moved the anchor to the fitted value plus the mean residual of the last three years ([decisions.md](decisions.md), D11).
`outputs/india_fit.csv` reports the inputs (`resid_base_year`, `resid_sd`, `durbin_watson`); its `anchor_shift` row is now 0.

### 2.6 Validation

**Rolling-origin backtest.**
The model is fitted on FY2006-07 up to each origin from FY2012-13 to FY2018-19 and predicts the following years with the observed drivers.

| Horizon (years) | 1 | 2 | 3 | 4 | 5 |
| --- | --- | --- | --- | --- | --- |
| Elasticity model, MAPE | 2.0% | 3.6% | 4.2% | 5.3% | 6.4% |
| Damped trend (no drivers), MAPE | 2.2% | 4.8% | 6.5% | 7.1% | 6.9% |

Backtest errors are not used as ensemble weights: with seven short origins they are too noisy.
The damped trend is shown as a benchmark only, because it has no population or urbanisation term.

**Holdout: FY2024-25 and FY2025-26.**
These years were not used in fitting.
NPP publishes their conventional generation (thermal, nuclear, hydro) but no renewable generation, so the check adds NPP renewable generation extended at its FY2018-19 to FY2023-24 growth rate (14.1 percent a year).

| Year | NPP conventional | Renewable (extended) | Total | Model P50 | Model P5 to P95 | Error |
| --- | --- | --- | --- | --- | --- | --- |
| FY2024-25 | 1,569 | 216 | 1,785 | 1,727 | 1,632 to 1,827 | -3.3% |
| FY2025-26 | 1,530 | 247 | 1,776 | 1,786 | 1,661 to 1,922 | +0.5% |

Both holdout years fall inside the model's 90 percent range.

### 2.7 Captive generation

Captive plants inside industries are outside the NPP series, so they are not in the forecast.
Until 16 November 2026 the outputs reported a captive layer beside the forecast, from CEA's General Review.
The organisers' ruling ("Use energy production ... from the NPP dataset. Do not use any external data for estimation") rules that out, so the layer has been removed from the code, the outputs and the dashboard ([decisions.md](decisions.md), D12).

## 3. Independent check: cross-country panel

This model uses data outside the two challenge datasets, so it is not the core forecast.
Under the organisers' ruling of 16 November 2026 it is used for justification only, never for estimation: no number from it enters the India-only forecast, the regional split or the plan.
It answers one question: does an independent method, based on how 153 countries' generation grew with income and urbanisation, land in the same range?

| Item | Source | Coverage |
| --- | --- | --- |
| Total electricity generation (TWh) | Ember yearly electricity data, "Total generation" | 1990 to 2025 |
| Population | World Bank WDI, SP.POP.TOTL | 1990 to 2025 |
| GDP per capita, PPP, constant 2021 international $ | World Bank WDI, NY.GDP.PCAP.PP.KD | 1990 to 2025 |
| Urban population share | World Bank WDI, SP.URB.TOTL.IN.ZS | 1990 to 2025 |

The panel keeps countries with at least 1 million people and at least 10 complete years: 153 countries and 4,370 country-years.
Ember's India series is total generation, beyond NPP's utility series, and uses calendar years.

**FE-Quad.** ln e(i,t) = alpha(i) + b1 x + b2 x^2 + g u + d tau, with x = ln(GDP per capita / 10,000), u = urban share, tau = (year - 2000) / 10.

**FE-Gompertz.** ln e(i,t) = alpha(i) - a exp(-b y) + c u + d tau, with y = GDP per capita in thousand $; b by a one-dimensional search (log grid plus golden section), (a, c, d) by least squares at each b.

**Damped trend.** Additive damped-trend exponential smoothing on ln(India total generation), fitted by a parallel grid search.

| Model | Income | Urban share | Trend per decade | Within R^2 | Elasticity at India 2025 |
| --- | --- | --- | --- | --- | --- |
| FE-Quad | b1 = 0.444, b2 = -0.137 | g = 2.63 | d = 0.023 | 0.465 | 0.44 |
| FE-Gompertz | a = 2.37, b = 0.223 | c = 2.46 | d = 0.032 | 0.481 | 0.57 |

A country-cluster bootstrap gives 2,000 replicates per model; ensemble weights are 1 / MAPE of a 2010-origin backtest.
The cross-country elasticity at India's income (0.44 to 0.57) is well below India's own (0.83): richer countries' elasticities fall, and the panel carries that saturation into India's future.
The primary model can reproduce it through the "elasticity decline" setting.

## 4. Regional requirement

A region's generation is not its requirement, because power flows between regions.
Under the organisers' ruling of 16 November 2026 the split uses the challenge data only (`src/regional/states.cpp` and the `Pipeline` constructor).
Each of the 34 states and union territories with a 2024 population in the challenge dataset gets a weight:

w(s,t) = population share(s,t) x exp( f(y(s,t)) )

| Term | Meaning |
| --- | --- |
| f | The same income response as the national model in that scenario |
| y(s,t) | State income per person: national income x the state's relative income, which converges over time |

Regional requirement = national generation x (sum of w over the region) / (sum of all w).
The regions add up exactly to the national total.
The model-error term of the national core (section 2.5) is common to every state, so it scales the national total and leaves the shares unchanged.
The split has no state urbanisation term, because the challenge dataset has no state urban share.
Urbanisation acts only through the national total (section 2.3), which the split divides but does not change.

**Grid regions.** Each state is placed in the grid region under which NPP reports its generation (`data/raw/npp/npp_state_generation_fy.csv`).
A state that NPP does not list takes the region of its Zone in the challenge dataset; Central (Madhya Pradesh, Chhattisgarh) maps to Western.
Where NPP and the Zone disagree, NPP is used:

| State | Dataset Zone | Region used (NPP) |
| --- | --- | --- |
| Andaman and Nicobar Islands | South | Eastern |
| Rajasthan | West | Northern |
| Sikkim | North-East | Eastern |

Chandigarh, which NPP does not list, is Northern by its Zone.
Ladakh (listed by NPP) and Dadra and Nagar Haveli and Daman and Diu have no rows in the challenge dataset, so they are not in the split.
A state with a population but no region stops the run.
`tests/test_regions.cpp` checks the mapping against the two files, that the shares sum to 1 in every year, that the first-year shares equal the dataset's 2024 shares, and that the national forecast does not depend on the split.

**Population.** Each state's path starts from its 2024 population in the challenge dataset and grows at the UN WPP 2024 medium growth of India plus the state's own offset over the dataset's last five years:

g(s,y) = g_UN(y) + [ g(s, 2019-2024) - g(India, 2019-2024) ]

so P(s,y) = P(s,2024) x WPP(y) / WPP(2024) x exp( offset(s) x (y - 2024) ), with growth in log points a year.
India's 2019-2024 growth is taken over the states reported in both years.
As in the India core, dataset year t is fiscal year t, so the dataset's 2024 is FY2024-25, the first projection year.
The shares are renormalised every year and only they enter the Monte Carlo; the national population path of the forecast (section 5) is unchanged.

**Consistency with the challenge dataset.** The ruling allows external projections that are "reasonably consistent with the state-level population growth provided in the GitHub dataset".
`outputs/population_consistency.csv` compares, for India and each state, the dataset's compound growth (2011-2024 and 2019-2024), the model's paths (2024-2030 and 2024-2047) and UN WPP 2024 medium; a Technical Group column (2024-2030) is a reference only and is not read by the model.

| India, percent a year | 2011-2024 | 2019-2024 | 2024-2030 | 2024-2047 |
| --- | --- | --- | --- | --- |
| Challenge dataset (sum of states) | 1.12 | 0.97 | | |
| UN WPP 2024 medium | 1.08 | 0.88 | 0.83 | 0.60 |
| Model path (the forecast's population input: UN WPP 2024 medium growth) | | | 0.83 | 0.60 |
| Technical Group 2020 (reference only) | | | 0.77 | |

- The dataset's 2024 total is 1.399 billion, against 1.403 billion in the Technical Group projection for 1 July 2024.
- Every state's path to 2030 lies 0.14 points a year below its own 2019-2024 growth in the dataset (0.137 to 0.140 points): the gap between the dataset's national growth (0.97 percent) and UN growth (0.83 percent), which is the same for every state, so the ranking and spread of state growth rates are those of the dataset.
- To 2047, state growth ranges from -0.08 percent a year (Lakshadweep) to 1.90 percent (Puducherry).

**State income.** In the base year, state income relative to the national average comes from the organiser dataset (state GSDP and population, 2024).
Only the State, Year, Zone, Population and GSDP columns of that file are read.
Relative income gaps then close at a speed estimated from the same data (2006 to 2024), by long-difference beta-convergence over 32 states:

| Estimate | Value |
| --- | --- |
| lambda | 0.021 per year |
| Bootstrap s.e. | 0.011 |
| Half-life | about 33 years |

A common price level cancels in the ratio, so the 2011 price-base change does not affect it.

**Where power is needed vs where it is generated.**
NPP regional generation shows where plants are, not where power is used, because the Eastern and Western regions export.
The model never fits regional generation; it compares the 2047 regional requirement with today's regional shares of NPP conventional generation and reports the transfer gap (results, section 2).

## 5. Monte Carlo ensemble

Each draw is one consistent future (primary core):

| Input | Distribution |
| --- | --- |
| Population path | UN WPP 2024 medium, anchored on FY2023-24; normal shift scaled so low/high is a 90 percent band |
| Urban share path | UN WUP 2018 change, anchored on FY2023-24; normal shift with s.d. growing to 2 points by 2047 |
| Income growth per person | Mean path (4.95 percent a year to 2030, the FY2012-13 to FY2023-24 organiser average without the two pandemic years, then slowing by 1.5 points by 2047), persistent shift (s.d. 0.9 points, so the 90 percent band of the average spans about +-1.5 points) and AR(1) annual shocks (s.d. 1.50 percent; persistence 0.63, estimated from consecutive retained years only) |
| Model | India elasticity with the urban prior (default weight 1); no-urban-term and damped-trend slots for ablation |
| Elasticity and gamma | One of 2,000 block-bootstrap replicates, gamma from its prior (truncated normal) |
| Model error (India core) | AR(1) residual of ln(kWh per person): starts at the FY2023-24 residual (+0.070), persistence 0.51, innovation s.d. 0.032 (section 2.5) |
| Income convergence speed | Normal(0.021, 0.011) |
| New loads | Uniform between dashboard limits (0 by default), ramped to 2047 |

Random numbers come from the Philox 4x32-10 counter-based generator, keyed by draw index.
A draw therefore has identical inputs on the CPU and the GPU.
Outputs (national and five regions, 24 years) go into 1,024-bin histograms.
Quantiles are read from those histograms.

**Optimisation:**

- **CPU path:** OpenMP over draws, thread-private histograms, and a parallel merge.
- **GPU path:**
  - Inputs live in constant memory, which broadcasts to every thread of a warp.
  - Histograms are updated by atomic add.
  - The model choice is shared by each group of 32 draws, so a warp never diverges on the model branch.
  - The kernel is compiled to machine code for the RTX 30 series (sm_86).
- **Web path:** the same kernel compiled to WebAssembly (no GPU, fewer draws), see [deployment.md](deployment.md).

A parity test checks that CPU double, CPU float and GPU float agree within 0.2 percent at the 5th, 50th and 95th percentiles of every series.
The CUDA kernel passes the Compute Sanitizer memcheck, initcheck, racecheck and synccheck tools with no errors.

| Run | 1,048,576 scenarios |
| --- | --- |
| CPU, 1 thread | 20.0 s |
| CPU, 16 threads | 2.21 s |
| GPU, RTX 3070 Ti Laptop | 7.7 ms |

## 6. Regional generation strategy for 2047

The 2047 requirement is taken at a planning quantile (the median by default) and split by the regional medians.
Two allocation methods then meet it.

### 6.1 Least-cost linear program (HiGHS)

**Variables:** generation of each source in each region, and the flow in each direction on each of the 7 inter-regional corridors.

| Constraint | Detail |
| --- | --- |
| Regional balance | Production + imports after losses - exports = requirement (with a costly unserved-energy slack) |
| Resource limits | Solar, wind, offshore wind, hydro and bioenergy at most their usable generation potential |
| Shared resource | Solar, onshore wind and firmed RE share each region's solar plus wind limit |
| Existing fleets | Lower bounds at 2024 generation for nuclear, hydro, solar and wind; coal at least 60 percent of its 2024 level |
| Coal ceiling | National coal at most 110 percent of 2024; regional coal at most 150 percent |
| Firm generation | At least 45 percent nationally and 25 percent in each region |
| Policy shares | Non-fossil share at least 70 percent; gas at most 5 percent; nuclear at most 10 percent of the requirement, sited by reactor units in the pipeline |
| Build rates | Each source's 2047 generation at most its 2025 level plus 22 years x 2 x its record annual increase |
| Corridors | Flow at most 2 x the 2032 plan x 60 percent utilisation; losses of 3.1 to 4.2 percent |
| CO2 cap (optional) | National CO2 at most a cap in Mt per year; off by default (section 6.5) |

Resource potentials, costs and emission factors come from published sources, not from the challenge datasets.
The organisers ruled on 16 November 2026 that resource potentials are for justification, not estimation: they never enter the forecast or the regional split, and here they only bound the supply plan.
Whether potentials, costs and emission factors may serve as limits and prices inside the mix optimisation is one of the follow-up questions sent to the organisers ([decisions.md](decisions.md), D12).

**Objective:** minimise generation cost plus carbon price times emissions.

**Costs:**

| Source | 2047 cost basis |
| --- | --- |
| Coal, gas, nuclear, solar, onshore and offshore wind | IEA WEO 2025 India LCOE projections interpolated to 2047 |
| Solar and wind (regional) | Scaled by the regional capacity factor |
| Hydro and bioenergy | Their tariff ratio to recent new-coal bids |
| Firmed RE | Solar plus today's round-the-clock premium over plain solar |

Every solution is checked against the constraint rows independently of the solver.
The dual value of each regional balance gives a marginal cost of supply per region.

### 6.2 Resource-score allocation

The national total of each source is shared across regions in proportion to a score, within the same bounds, by water-filling:

| Sources | Score |
| --- | --- |
| Solar, wind, offshore wind, hydro, bioenergy | Usable generation potential |
| Coal, gas | 2024 generation (existing fleet) |
| Nuclear | Reactor units |

Every score can be checked by hand.
Comparing it with the least-cost plan shows where transmission limits and costs move generation.

### 6.3 Policy sensitivity

Fourteen settings (policy shares, land use, corridors, build rates, carbon price, costs and the requirement quantile) are each moved to a low and a high value.
The 28 resulting least-cost problems are solved in parallel with OpenMP.

### 6.4 Storage and land back-calculation

At the kickoff the organisers asked how much storage and how much land the proposed mix needs.
Both are worked back from the least-cost plan's annual generation by source; nothing feeds back into the forecast or the plan.
The code is in `src/allocation/storage.cpp`, the factors in `data/external/techno_economic/land_use.csv` and `storage_parameters.csv`, and the results in `outputs/storage_land_2047.csv`.

**Representative year.**
The hourly shapes come from the NPP Merit real-time series (all-India generation by source, `data/raw/npp/npp_merit_generation_hourly.csv`).
Only complete days are used: 332 days between 19 September 2025 and 13 November 2026 with all six sources present in all 24 hours.
Each calendar month is weighted to its number of days in a 365-day year, so September to November, which appear in both years, are not counted twice.
The load shape is the shape of total generation (the sum of the six sources); the demand-met series is not used.
Solar generation grew by about 40 percent a year over the record as new plants came on line, which would make later months look sunnier.
Fleet growth is therefore removed from the load, solar, coal and gas shapes, using the change between the same calendar month a year apart.
Wind and hydro differences between the two years are weather, not growth, and are kept.
Offshore wind takes the onshore wind shape, since India has no offshore wind yet.

**Hourly dispatch of the plan.**
The hourly load is the requirement the plan meets (requirement minus any unserved energy of the LP) times the load shape.
Each source keeps its annual energy from the plan and is dispatched with simple, stated rules.

| Source | Hourly rule |
| --- | --- |
| Solar, onshore and offshore wind | Annual generation times the Merit hourly shape |
| Nuclear, bioenergy | Flat |
| Hydro | Each day keeps its observed share of annual hydro (water availability) and moves within that day's observed hourly range towards the hours of highest remaining load |
| Coal | Annual energy of the plan; each hour between 40 percent of the fleet's peak hour (CEA flexible operation regulations, 2023) and that peak, which is 1.26 times its mean hour as in the Merit thermal series; follows the remaining load as closely as these bounds allow |
| Gas | Annual energy of the plan, used for the highest remaining hours (peaking), up to 3.7 times its mean hour as in the Merit gas series |
| Firmed RE | Two views. Headline: split back into the solar and onshore wind it is built from, in proportion to the usable potential the plan leaves in each region (the LP draws firmed RE from the shared solar plus wind resource), so all storage sits in the system. Check: a round-the-clock block whose storage is inside the plants |

What remains in each hour is a surplus (solar and wind that the load cannot take) or a shortfall.

**Storage sizing.**
One store charges from surplus and discharges into shortfall with an 85 percent round trip (NREL Annual Technology Baseline 2025).
Each run is solved in its cyclic steady state, so the store ends each cycle where it started.
For one store and a shortfall objective, using it greedily is optimal, because stored energy covers the same shortfall whenever it is released.

| Quantity | Rule |
| --- | --- |
| Power (charge and discharge) | At least the largest hourly shortfall after dispatch; raised for midday charging while each extra GW still serves at least 365 GWh of shortfall a year (one hour a day at full power) |
| Energy, daily cycle | Each complete day is a cyclic 24-hour sequence; the smallest store that leaves no more shortfall than an unlimited one; the design value is the largest over the 332 days |
| Energy, multi-day | The days in calendar order with carry-over between them; the store grows beyond the daily size while each extra GWh still serves at least 52 GWh of shortfall a year (about one cycle a week); larger stores would be seasonal storage |

The run also reports the hours and energy of shortfall before storage, curtailed solar and wind, storage losses and cycles.
It also reports the shortfall left after storage, which is extra generation the plan needs hour by hour.
The days are sized in parallel with OpenMP; per-day results are summed in a fixed order, so results do not depend on the thread count.
Both views of a plan are sized in about 90 ms, so the dashboard recomputes storage with every strategy update.

**Land.**
Land is generation times an energy-based land-use intensity (area per GWh a year).
Per-MW norms are converted to energy with the same regional capacity factors the allocation uses (CEA NEP 2022-32 planning values).
Installed capacity is never reported.

| Item | Intensity | Source |
| --- | --- | --- |
| Solar, central | 4 acres per MW, which is 1.9 to 2.3 acres per GWh a year at the regional capacity factor | MNRE (PIB, 20 July 2023) |
| Solar, high case | 3.4 acres per GWh a year (total site area, large PV) | Ong et al. 2013, NREL/TP-6A20-56290, Table ES-1 |
| Onshore wind, footprint | 0.3 ha per MW (pads, roads, substations) | Denholm et al. 2009, NREL/TP-6A2-45834, Table 1 |
| Onshore wind, project area | 34.5 ha per MW (mostly still farmed) | Denholm et al. 2009, Table 1 |
| Offshore wind | No land | |

The land of solar plus the wind footprint is compared with India's geographical area (3,287,469 km², ISFR 2023), with each grid region's area (sum of its states) and with India's wasteland (557,666 km² in 2015-16, Wastelands Atlas 2019).

**Limitations.**
The calculation is national and hourly, so it ignores transmission limits within the hour and reserves below an hour.
One year of weather is used, and a weak monsoon or a cloudier year would change the design day.
The load keeps today's daily shape, so 2047 cooling, electric vehicles and hydrogen loads could move it.
All solar is counted as ground-mounted, which overstates land if rooftop and agri-PV supply part of it.
Pumped storage and batteries are not told apart, and one round trip applies to both.

### 6.5 Cost-CO2 trade-off

The least-cost LP accepts an optional national CO2 cap (Mt per year).
The dual value of the cap row is the marginal abatement cost: the extra system cost, in USD per tonne, of the last tonne the cap removes.

The trade-off curve is an epsilon-constraint sweep of that cap (`pareto_cost_co2` in `src/allocation/allocate.cpp`):

1. Solve the plan without a cap and without a carbon price, so that the cost axis is resource cost only and the dual is not net of a carbon price.
2. Find by bisection the lowest cap that still meets the requirement in full; below it the LP would leave requirement unserved rather than emit.
3. Solve twelve caps spaced evenly between the uncapped CO2 and that lowest cap, in parallel with OpenMP.
4. Keep the plans that meet the requirement, order them by falling CO2 and remove dominated points.

The command-line run sweeps the recommended plan (Robust policy, median requirement) and writes `outputs/pareto_2047.csv` (results, section 3.5).
`tests/test_pareto.cpp` checks a slack and a binding cap, compares the dual with the finite-difference slope of the curve and checks that the frontier is monotone.
