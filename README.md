# India Energy 2047

Forecasting India's electricity generation requirement for 2047 and a region-specific plan to meet it.
Team submission for ExxonMobil INNOTHON 2026 (Indian Institute of Science, Bengaluru).

![Dashboard](docs/img/1_national_forecast.png)

## Headline results (default scenario)

| | 2025 (observed) | 2047 median | 2047 90% range |
| --- | --- | --- | --- |
| National generation | 2,080 TWh | **4,536 TWh** | 3,351 - 5,606 TWh |
| Per person | 1,421 kWh | about 2,720 kWh | |

| Region | 2047 requirement (median) | Share | Least-cost 2047 role (base policy) |
| --- | --- | --- | --- |
| Northern | 1,327 TWh | 29% | Rajasthan solar, Himalayan hydro, coal fleet, nuclear; about self-sufficient |
| Western | 1,120 TWh | 25% | Solar, wind, Gujarat offshore wind, coal, nuclear; largest exporter (+241 TWh) |
| Southern | 983 TWh | 22% | Best wind, Tamil Nadu offshore wind, nuclear; exporter (+113 TWh) |
| Eastern | 932 TWh | 21% | Coal fleet and nuclear; imports 340 TWh, so corridors into the East must grow |
| North-Eastern | 173 TWh | 4% | Arunachal hydro supplies 81% of the region |

Two plans for 2047:

| Plan | Solar | Firmed RE | Coal | Wind (on- and offshore) | Hydro | Nuclear | Non-fossil | CO2 | Cost | Unmet if requirement reaches P90 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Least cost, base policy | 37.5% | 0% | 30.0% | 17.3% | 7.7% | 7.5% | 70.0% | 1,323 Mt | 202 bn USD/yr | 447 TWh |
| **Robust policy (recommended)** | 52.5% | 15.1% | 20.8% | 2.7% | 7.7% | 1.2% | **79.2%** | **920 Mt** | **166 bn USD/yr** | **0 TWh** |

- Firmed RE means solar and wind backed by storage, sold round the clock.
- The Robust plan opens more land for solar (rooftop and agri-PV), triples inter-regional corridors and builds storage-firmed renewables.
- It is cheaper, cleaner and still adequate if the requirement reaches the 90th percentile.
- Today's generation emits about 1,480 Mt CO2.
- Details are in [docs/results.md](docs/results.md).

## Compliance rule

Only **energy generated** (GWh or TWh) is used for forecasting and comparison.
Installed capacity and energy consumption are never forecasting inputs or comparison metrics.
The organiser dataset in [data/raw/competition/](data/raw/competition/) contains installed-capacity columns; only its population and GSDP columns are read.
See [docs/data_audit.md](docs/data_audit.md).

## Method in one paragraph

Generation per person is modelled from income (GDP per capita, PPP) and urban share.
Two within-country fixed-effects models do this: a quadratic and a saturating Gompertz form.
They are fitted on 153 countries (Ember generation, World Bank drivers, 1990-2025).
A damped-trend model of India's own series is the statistical benchmark.
Models are weighted by an out-of-sample backtest; the Gompertz model is 0.6 percent off in 2025 when fitted only to 2010.

A Monte Carlo ensemble of 4 million scenarios runs on the GPU in about 30 ms.
It combines UN population and urbanisation projections, uncertain income growth, bootstrap model parameters and state income convergence.
It produces national and regional requirements to 2047.

A least-cost linear program (HiGHS) then sites 2047 generation by region and source.
It works within resource potentials, firm-generation floors, build-rate limits, policy shares and inter-regional corridor limits.
A transparent resource-score allocation and a policy sensitivity sweep sit alongside it.
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
| [docs/beamer/](docs/beamer/) | Presentation deck (LaTeX Beamer) and reader's edition with notes |

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

:: tests
%USERPROFILE%\.e47build\tests\test_mc_parity.exe
%USERPROFILE%\.e47build\tests\test_allocation.exe "<repo folder>"

:: save every dashboard view as an image (for slides)
%USERPROFILE%\.e47build\apps\e47_dashboard.exe "<repo folder>" --shot shots
```

## Repository layout

| Path | Contents |
| --- | --- |
| [src/core/](src/core/) | CSV I/O, Philox random numbers, statistics |
| [src/forecast/](src/forecast/) | Panel loader, models, bootstrap, backtest, Monte Carlo (CPU and CUDA) |
| [src/regional/](src/regional/) | State income convergence |
| [src/allocation/](src/allocation/) | Strategy inputs, least-cost LP, resource-score allocation, policy sweep |
| [src/pipeline/](src/pipeline/) | End-to-end pipeline used by the tools and the dashboard |
| [apps/](apps/) | `energy2047` command-line tool and `e47_dashboard` |
| [tests/](tests/) | CPU/GPU parity test and allocation test |
| [data/](data/) | Source data (tidy CSV extracts) with SOURCES.md files |
| [docs/](docs/) | Documentation and dashboard images |

## Team

See [TEAM.md](TEAM.md).
