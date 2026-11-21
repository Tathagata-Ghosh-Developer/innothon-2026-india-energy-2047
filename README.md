# India Energy 2047

[![CI](https://github.com/Tathagata-Ghosh-Developer/innothon-2026-india-energy-2047/actions/workflows/ci.yml/badge.svg)](https://github.com/Tathagata-Ghosh-Developer/innothon-2026-india-energy-2047/actions/workflows/ci.yml)

Forecasting India's electricity generation requirement for 2047 and a region-specific plan to meet it.
Team submission for ExxonMobil INNOTHON 2026 (Indian Institute of Science, Bengaluru).

**Live dashboard:** <https://india-energy-2047.vercel.app> (WebAssembly build of the C++ dashboard; first load downloads about 11 MB).

![Dashboard](docs/img/1_national_forecast.png)

## Headline results (default scenario, primary India-only core)

Years are fiscal years by start year (2023 = FY2023-24, 2047 = FY2047-48).
Scope: the country's total utility generation as published by the National Power Portal (NPP); the headline is total TWh, not generation per person.

| | FY2023-24 (observed, NPP) | FY2047-48 median | 80% range (P10-P90) | 90% range (P5-P95) |
| --- | --- | --- | --- | --- |
| National generation | 1,698 TWh | **4,688 TWh** | 3,471 - 6,408 TWh | 3,190 - 7,021 TWh |
| Increase over FY2023-24 | | +2,990 TWh (2.76 times, 4.3% a year) | | |

- Income elasticity of generation per person, estimated for India as a whole from India's own data: **0.835** (s.e. 0.065; FY2011-12 to FY2023-24; 0.79 in first differences). One standard error either way moves the 2047 median by -6.5% / +7.0%.
- Holdout years not used in fitting: FY2024-25 error -3.3%, FY2025-26 error +0.5%; both inside the model's 90% range.
- Income is the largest driver but not the only one: of the growth to FY2047-48 on the central path, income accounts for 88%, population for 15% and urbanisation beyond income for 5%, while the fading of the high FY2023-24 level takes back 7% (`outputs/growth_decomposition_2047.csv`).
- Income growth dominates the uncertainty: +-1.5 points of growth moves the 2047 median by +31% / -24%.
- Every other driver of the requirement, 44 in all, is catalogued in [docs/demand_features.md](docs/demand_features.md), each quantified size from an official source. The 11 that the history does not carry form a labelled layer outside the headline: with every feature at its central value the median is 5,810 TWh (+23.9%), with the features drawn jointly 5,668 TWh (+20.9%; P5 4,130, P95 8,017), and the median of every combination of feature levels lies between All downside (4,865) and All upside (6,555).
- Independent cross-country check, used for justification and not for estimation (153 countries, total generation in calendar 2047): 4,528 TWh.

The regional split uses only the challenge data: the population and GSDP of 34 states and union territories in the Renewable India dataset, with each state's own income applied state by state (the income gaps barely converge: 0.2 percent a year), state populations grown at the UN's India growth plus each state's own 2019-2024 offset, and the grid region NPP reports each state under.
The dataset's GSDP is at constant 2011-12 prices except for Jammu and Kashmir from 2012, which is at current prices and is deflated with the MoSPI GDP deflator (secondary).

| Region | FY2047-48 requirement (median) | Share of requirement | Share of NPP conventional generation, FY2025-26 |
| --- | --- | --- | --- |
| Northern | 1,342 TWh | 28.6% | 24.9% |
| Western | 1,276 TWh | 27.2% | 36.0% |
| Southern | 1,188 TWh | 25.3% | 20.1% |
| Eastern | 730 TWh | 15.6% | 17.7% |
| North-Eastern | 151 TWh | 3.2% | 1.3% |

Two plans for 2047 (median requirement), built from the FY2023-24 regional mix (NPP conventional generation plus CEA renewables) within published build limits. Costs are in real 2024 USD:

| Plan | Solar | Firmed RE | Coal | Wind (on- and offshore) | Hydro | Nuclear | Gas and bioenergy | Non-fossil | CO2 | Cost at median | Unmet at median | Unmet at P90 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Least cost, base policy | 36.4% | 0% | 30.0% | 15.9% | 7.4% | 10.0% | 0.3% | 70.0% | 1,367 Mt | 213 bn USD/yr | 0 TWh | 1,471 TWh |
| **Robust policy (recommended)** | 50.9% | 14.6% | 19.0% | 4.2% | 7.4% | 2.8% | 1.1% | **79.9%** | **891 Mt** | **176 bn USD/yr** | **0 TWh** | **639 TWh** |

- Firmed RE means solar and wind backed by storage, sold round the clock.
- The Robust plan opens more land for solar (rooftop and agri-PV), triples inter-regional corridors, builds at the published high-case limits, builds storage-firmed renewables and keeps the existing gas fleet (51 TWh, its best NPP year) for peaking; both plans meet the median, where the Robust plan is cheaper (by 37 bn USD a year) and cleaner (by 476 Mt of CO2). At P90 it leaves less than half the base policy's shortfall (639 against 1,471 TWh) and costs more, 253 against 236 bn USD a year, because it generates more.
- Nuclear and hydro in every plan include the committed builds (14,300 MW of nuclear and 13,023.5 MW of large hydro under construction or sanctioned).
- Meeting the P90 requirement (6,408 TWh) additionally needs about 45 percent of the solar potential usable; land, not money or build rate, is the binding constraint (building at twice the published high-case limits without more land still leaves 217 TWh unmet).
- Deeper CO2 cuts are cheap: a CO2 cap takes the Robust plan from 891 to 653 Mt a year (-27%) for 0.45 bn USD/yr more (marginal cost at most 4.0 USD/t), until the coal fleet floor binds.
- On the CEA cost basis, now also in real 2024 USD, the Robust plan costs 203 bn USD a year (182 to 225 across the RBI inflation band); in 2047 money its IEA-basis cost is Rs 36.3 lakh crore or 278 bn USD a year (information only).
- Known limit: in the Low growth scenario, the Robust policy has no feasible plan at the 5th-percentile requirement (2,306 TWh), because its coal floor and the gas kept for peaking exceed its fossil allowance there ([docs/decisions.md](docs/decisions.md), D13).
- Details are in [docs/results.md](docs/results.md). The answers to the midpoint panel's five questions are in [docs/midpoint_panel_answers.md](docs/midpoint_panel_answers.md).

The final deck and its PDF ([docs/deck/](docs/deck/)), the Beamer decks ([docs/beamer/](docs/beamer/)) and the Q&A notes were updated on 21 November 2026, after the final evaluation, to the current run. The talk script (`docs/final_talk.md`) and the midpoint slide keep the numbers presented then (the 20 November run: a median of 4,703 TWh). [docs/decisions.md](docs/decisions.md), D14, records every change.

## Compliance rule

Only **energy generated** (GWh or TWh) is used for forecasting and comparison.
Installed capacity and energy consumption are never forecasting inputs or comparison metrics.
The core 2047 prediction (Part 1: the national requirement and its regional split) uses only the two challenge datasets: NPP generation and the organiser state population and GSDP.
One series in them is corrected for inflation: Jammu and Kashmir's GSDP, which the dataset gives at current prices from FY2012-13, is put on constant prices with the MoSPI GDP deflator (secondary); the run without Jammu and Kashmir gives 4,683 TWh.
The organisers' written ruling of 16 November 2026 is applied: NPP generation only (no captive generation), the regional split from the challenge data, and the cross-country model and resource potentials for justification only ([docs/decisions.md](docs/decisions.md), D12).
The generation mix (Part 2) uses published figures, as the kickoff guidance allows publicly available literature for the growth rates and constraints of each energy source: build limits (CEA NEP 2022-32, NITI Aayog, DAE), committed builds, resource potentials (NISE, NIWE, CEA), costs (IEA WEO 2025, with a CEA cost basis as a sensitivity) and emission factors (CEA CO2 Baseline Database v22.0), each labelled secondary.
Asked whether these may act as limits and prices in the mix, the organisers replied "Use insights from this data source"; a clarification has been requested and no reply has come yet. None of these figures enters the 2047 requirement.
The organiser dataset in [data/raw/competition/](data/raw/competition/) contains installed-capacity columns; only its State, Year, Zone, Population and GSDP columns are read.
The two organiser sources, the National Power Portal (<https://npp.gov.in/landing-home>) and the Renewable India dataset (<https://github.com/chrisjaimyantony/renewable-india-dataset>), are the primary, trusted sources; every other source is secondary and labelled as such.
The headline is estimated on the two datasets. The demand features (official sources, secondary) are a labelled layer outside it; where a source gives capacity or end-use electricity, the feature is converted explicitly to TWh of generation.
Every money figure is in real 2024 USD, converted with official price indices (MoSPI, RBI, BEA; secondary).
See [docs/data_audit.md](docs/data_audit.md).

## Method in one paragraph

Generation per person (NPP) is modelled from real income per person (organiser GSDP and population).
The income elasticity is the only parameter estimated; a moving-block bootstrap over two estimation windows carries its uncertainty.
Urbanisation enters only where it runs ahead of income, with a coefficient taken from published studies (the run without it, gamma = 0, is reported beside it).
Model error is carried as a persistent (AR(1)) residual that starts at the FY2023-24 residual and reverts to the fitted relation.
A Monte Carlo ensemble of 4 million scenarios runs on the GPU (11.2 ms for 1,048,576 scenarios in the latest run, in `outputs/timings.csv`; timings vary by run and machine load); it combines UN population and urbanisation projections, uncertain income growth, the bootstrap elasticities, the model error and state income convergence, and produces national and regional requirements to 2047.
The national total is split across the five grid regions by the population and income of 34 states and union territories in the challenge dataset; `outputs/population_consistency.csv` shows that the population paths are consistent with the dataset's own growth, and `outputs/state_income_2047.csv` shows the split state by state.
Income convergence is estimated from each state's first year that is not a placeholder.
A demand-feature layer adds the 11 quantified drivers from official sources as named scenarios and a joint distribution, outside the headline.
A least-cost linear program (HiGHS) then sites 2047 generation by region and source, starting from the FY2023-24 regional mix (NPP conventional generation by region, primary; CEA renewables by state, secondary, because NPP has none by region).
It works within resource potentials, existing-fleet and committed-build floors, published build limits, firm-generation floors, policy shares and inter-regional corridor limits; an optional CO2 cap traces the cost-CO2 trade-off, and `outputs/sensitivity_mix.csv` re-solves the recommended plan with other published costs, potentials and build cases.
Every cost is in real 2024 USD: the IEA costs already are, and the CEA cost basis is deflated with the MoSPI GDP deflator, converted at the RBI 2024 rate and given a real WACC by the Fisher relation at the RBI inflation target.
A 153-country panel model (Ember and World Bank data) is kept as an independent check used for justification, not for estimation.
Full details are in [docs/methodology.md](docs/methodology.md).

## Documents

| Document | Contents |
| --- | --- |
| [docs/methodology.md](docs/methodology.md) | Models, regional split, Monte Carlo, allocation, price base |
| [docs/results.md](docs/results.md) | Forecast tables, regional mixes, sensitivity, performance |
| [docs/demand_features.md](docs/demand_features.md) | All 44 drivers of the 2047 requirement: mechanism, sign, treatment, official source, size; feature scenarios and the joint distribution |
| [docs/assumptions_and_uncertainty.md](docs/assumptions_and_uncertainty.md) | Every assumption, how uncertainty is handled, limitations |
| [docs/data_audit.md](docs/data_audit.md) | Provenance of every input; generation-only compliance |
| [docs/decisions.md](docs/decisions.md) | Design decisions and rejected alternatives |
| [docs/midpoint_panel_answers.md](docs/midpoint_panel_answers.md) | Answers to the five questions of the midpoint panel, with evidence and sources |
| [docs/midpoint_qa.md](docs/midpoint_qa.md) | Questions and answers for the final evaluation |
| [docs/team_plan.md](docs/team_plan.md) | Work plan to the final evaluation |
| [docs/deck/](docs/deck/) | Final evaluation deck (PowerPoint and PDF), updated to the current run on 21 November after the evaluation |
| [docs/beamer/](docs/beamer/) | Presentation deck (LaTeX Beamer) and reader's edition with notes, updated to the current run on 21 November; the midpoint slide stays as presented on 18 November |
| [docs/deployment.md](docs/deployment.md) | Native and WebAssembly builds, deployment |

## Build

Requirements:

- Windows 10 or 11
- Visual Studio 2022 Build Tools with the C++ workload (MSVC, CMake, Ninja)
- Optional: CUDA Toolkit 12 or newer and an NVIDIA GPU

All other libraries are fetched by CMake: Eigen, HiGHS, GLFW, Dear ImGui and ImPlot.

```bat
build.cmd
```

`build.cmd` loads the MSVC environment, configures with Ninja in `%USERPROFILE%\.e47build` (outside OneDrive), and builds everything.
Useful options:

```bat
build.cmd -DENABLE_CUDA=OFF                      :: CPU only (OpenMP)
build.cmd -DCMAKE_CUDA_ARCHITECTURES=native      :: GPU other than an RTX 30-series
build.cmd -DCMAKE_CUDA_COMPILER="C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v13.5/bin/nvcc.exe"
```

## Run

```bat
:: dashboard
%USERPROFILE%\.e47build\apps\e47_dashboard.exe "<repo folder>"

:: command line: fits, timing table, forecast, strategy; writes outputs\*.csv
%USERPROFILE%\.e47build\apps\energy2047.exe "<repo folder>" [draws]

:: tests (nine: mc_parity including CPU/GPU parity, allocation, storage, pareto, india, regions, price_base, features, site_format)
ctest --test-dir %USERPROFILE%\.e47build --output-on-failure

:: save every dashboard view as an image (for slides)
%USERPROFILE%\.e47build\apps\e47_dashboard.exe "<repo folder>" --shot shots
```

## Repository layout

| Path | Contents |
| --- | --- |
| [src/core/](src/core/) | CSV I/O, Philox random numbers, statistics, price base (real 2024 USD, GSDP price basis) |
| [src/forecast/](src/forecast/) | India-only core (NPP + organiser data), cross-country panel models, bootstrap, backtests, Monte Carlo (CPU and CUDA) |
| [src/regional/](src/regional/) | State income convergence; state population and grid regions from the challenge data |
| [src/allocation/](src/allocation/) | Strategy inputs, least-cost LP, resource-score allocation, policy sweep |
| [src/pipeline/](src/pipeline/) | End-to-end pipeline used by the tools and the dashboard; the demand-feature layer and its scenarios |
| [apps/](apps/) | `energy2047` command-line tool and `e47_dashboard` |
| [tests/](tests/) | Nine tests: CPU/GPU Monte Carlo parity (including the feature draws), allocation, storage, cost-CO2 trade-off, India core (stale GSDP rows, holdout coverage), regional split (state-to-region mapping, shares, independence of the national total, convergence base years), price base (Fisher relation, deflation, GSDP price basis, default plan unchanged), demand features (catalogue validation, scenario totals, joint draws, headline unchanged), landing-page number formatting |
| [data/](data/) | Source data (tidy CSV extracts) with SOURCES.md files |
| [docs/](docs/) | Documentation and dashboard images |
| [.github/workflows/](.github/workflows/) | CI (CPU build, tests and smoke run on Linux; WebAssembly build) and a weekly NPP API monitor |
| [api/](api/) | Postman collection of the public NPP endpoints the project reads, with tests and an offline mock |

The CUDA kernel passes the Compute Sanitizer memcheck, initcheck, racecheck and synccheck tools with no errors.

## Team

See [TEAM.md](TEAM.md).
