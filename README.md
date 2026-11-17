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
| National generation | 1,698 TWh | **4,703 TWh** | 3,497 - 6,400 TWh | 3,217 - 7,002 TWh |
| Increase over FY2023-24 | | +3,005 TWh (2.77 times, 4.3% a year) | | |

- Income elasticity of generation per person, India's own data: **0.83** (FY2011-12 to FY2023-24; 0.78 in first differences).
- Holdout years not used in fitting: FY2024-25 error -3.3%, FY2025-26 error +0.5%; both inside the model's 90% range.
- Income growth dominates the uncertainty: +-1.5 points of growth moves the 2047 median by +31% / -24%.
- Independent cross-country check, used for justification and not for estimation (153 countries, total generation in calendar 2047): 4,528 TWh.

The regional split uses only the challenge data: state populations from the challenge dataset, grown at the UN's India growth plus each state's own 2019-2024 offset, and the grid region NPP reports each state under.

| Region | FY2047-48 requirement (median) | Share of requirement | Share of NPP conventional generation, FY2025-26 |
| --- | --- | --- | --- |
| Northern | 1,405 TWh | 29.9% | 24.9% |
| Western | 1,226 TWh | 26.1% | 36.0% |
| Southern | 1,068 TWh | 22.7% | 20.1% |
| Eastern | 840 TWh | 17.9% | 17.7% |
| North-Eastern | 161 TWh | 3.4% | 1.3% |

Two plans for 2047 (median requirement):

| Plan | Solar | Firmed RE | Coal | Wind (on- and offshore) | Hydro | Nuclear | Gas and bioenergy | Non-fossil | CO2 | Cost | Unmet at median | Unmet at P90 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Least cost, base policy | 36.2% | 0% | 30.0% | 16.7% | 7.4% | 9.7% | 0% | 70.0% | 1,371 Mt | 213 bn USD/yr | 0 TWh | 1,529 TWh |
| **Robust policy (recommended)** | 50.7% | 14.6% | 21.7% | 4.5% | 7.4% | 1.2% | 0% | **78.3%** | **993 Mt** | **176 bn USD/yr** | **0 TWh** | **638 TWh** |

- Firmed RE means solar and wind backed by storage, sold round the clock.
- The Robust plan opens more land for solar (rooftop and agri-PV), triples inter-regional corridors and builds storage-firmed renewables; both plans meet the median, and the Robust plan is cheaper and cleaner and leaves less than half the base policy's shortfall at P90.
- Meeting the P90 requirement (6,400 TWh) additionally needs about 45 percent of the solar potential usable; land, not money or build rate, is the binding constraint (building four times faster than India's record year without more land still leaves 240 TWh unmet).
- Deeper CO2 cuts are cheap: a CO2 cap takes the Robust plan from 993 to 643 Mt a year (-35%) for 0.65 bn USD/yr more (marginal cost at most 6.3 USD/t), until the coal fleet floor binds.
- Details are in [docs/results.md](docs/results.md).

## Compliance rule

Only **energy generated** (GWh or TWh) is used for forecasting and comparison.
Installed capacity and energy consumption are never forecasting inputs or comparison metrics.
The core 2047 prediction uses only the two challenge datasets: NPP generation and the organiser state population and GSDP.
The organisers' written ruling of 16 November 2026 is applied: NPP generation only (no captive generation), the regional split from the challenge data, and the cross-country model and resource potentials for justification only ([docs/decisions.md](docs/decisions.md), D12).
The organiser dataset in [data/raw/competition/](data/raw/competition/) contains installed-capacity columns; only its State, Year, Zone, Population and GSDP columns are read.
The two organiser sources, the National Power Portal (https://npp.gov.in/landing-home) and the Renewable India dataset (https://github.com/chrisjaimyantony/renewable-india-dataset), are the primary, trusted sources; every other source is secondary and labelled as such.
See [docs/data_audit.md](docs/data_audit.md).

## Method in one paragraph

Generation per person (NPP) is modelled from real income per person (organiser GSDP and population).
The income elasticity is the only parameter estimated; a moving-block bootstrap over two estimation windows carries its uncertainty.
Urbanisation enters only where it runs ahead of income, with a coefficient taken from published studies (the two-dataset-only run is reported beside it).
Model error is carried as a persistent (AR(1)) residual that starts at the FY2023-24 residual and reverts to the fitted relation.
A Monte Carlo ensemble of 4 million scenarios runs on the GPU in about 35 ms; it combines UN population and urbanisation projections, uncertain income growth, the bootstrap elasticities, the model error and state income convergence, and produces national and regional requirements to 2047.
The national total is split across the five grid regions by the population and income of 34 states and union territories in the challenge dataset; `outputs/population_consistency.csv` shows that the population paths are consistent with the dataset's own growth.
A least-cost linear program (HiGHS) then sites 2047 generation by region and source within resource potentials, firm-generation floors, build-rate limits, policy shares and inter-regional corridor limits; an optional CO2 cap traces the cost-CO2 trade-off.
A 153-country panel model (Ember and World Bank data) is kept as an independent check used for justification, not for estimation.
Full details are in [docs/methodology.md](docs/methodology.md).

## Documents

| Document | Contents |
| --- | --- |
| [docs/methodology.md](docs/methodology.md) | Models, regional split, Monte Carlo, allocation |
| [docs/results.md](docs/results.md) | Forecast tables, regional mixes, sensitivity, performance |
| [docs/assumptions_and_uncertainty.md](docs/assumptions_and_uncertainty.md) | Every assumption, how uncertainty is handled, limitations |
| [docs/data_audit.md](docs/data_audit.md) | Provenance of every input; generation-only compliance |
| [docs/decisions.md](docs/decisions.md) | Design decisions and rejected alternatives |
| [docs/team_plan.md](docs/team_plan.md) | Work plan to the final evaluation |
| [docs/beamer/](docs/beamer/) | Presentation deck (LaTeX Beamer), reader's edition with notes, midpoint slide |
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

:: tests (seven: mc_parity including CPU/GPU parity, allocation, storage, pareto, india, regions, site_format)
ctest --test-dir %USERPROFILE%\.e47build --output-on-failure

:: save every dashboard view as an image (for slides)
%USERPROFILE%\.e47build\apps\e47_dashboard.exe "<repo folder>" --shot shots
```

## Repository layout

| Path | Contents |
| --- | --- |
| [src/core/](src/core/) | CSV I/O, Philox random numbers, statistics |
| [src/forecast/](src/forecast/) | India-only core (NPP + organiser data), cross-country panel models, bootstrap, backtests, Monte Carlo (CPU and CUDA) |
| [src/regional/](src/regional/) | State income convergence; state population and grid regions from the challenge data |
| [src/allocation/](src/allocation/) | Strategy inputs, least-cost LP, resource-score allocation, policy sweep |
| [src/pipeline/](src/pipeline/) | End-to-end pipeline used by the tools and the dashboard |
| [apps/](apps/) | `energy2047` command-line tool and `e47_dashboard` |
| [tests/](tests/) | Seven tests: CPU/GPU Monte Carlo parity, allocation, storage, cost-CO2 trade-off, India core (stale GSDP rows, holdout coverage), regional split (state-to-region mapping, shares, independence of the national total), landing-page number formatting |
| [data/](data/) | Source data (tidy CSV extracts) with SOURCES.md files |
| [docs/](docs/) | Documentation and dashboard images |
| [.github/workflows/](.github/workflows/) | CI (CPU build, tests and smoke run on Linux; WebAssembly build) and a weekly NPP API monitor |
| [api/](api/) | Postman collection of the public NPP endpoints the project reads, with tests and an offline mock |

The CUDA kernel passes the Compute Sanitizer memcheck, initcheck, racecheck and synccheck tools with no errors.

## Team

See [TEAM.md](TEAM.md).
