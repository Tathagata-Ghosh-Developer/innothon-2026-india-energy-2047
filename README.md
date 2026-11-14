# India Energy 2047

Forecasting India's electricity generation requirement for 2047 and a region-specific plan to meet it.
Team submission for ExxonMobil INNOTHON 2026 (Indian Institute of Science, Bengaluru).

## Problem

Build a data-driven framework that:

1. Forecasts India's total energy production requirement for 2047, nationally and for each region, accounting for population growth and urbanization.
2. Proposes a generation mix (coal, hydro, solar, wind, nuclear, natural gas) for each region that reflects local strengths.
3. Justifies the allocation with data, regional analysis and policy considerations.
4. States modeling assumptions, methodological choices and uncertainties.
5. Provides an interactive dashboard of national and region-wise forecasts and the proposed mixes.

The full statement is in [Innothon - Problem Statement.pdf](Innothon%20-%20Problem%20Statement.pdf).

## Compliance rule

Only **energy generated** (GWh or TWh) is used for forecasting and comparison.
Installed capacity and energy consumption are never used as forecasting inputs or comparison metrics.
The organiser dataset in [data/raw/competition/](data/raw/competition/) contains installed capacity columns.
From that file we use only population and GSDP.
See [docs/data_audit.md](docs/data_audit.md) for the column-by-column audit.

## Regions

The five grid regions of India: Northern, Western, Southern, Eastern and North-Eastern.
The state-to-region mapping is in [data/external/](data/external/).

## Repository layout

| Path | Contents |
|------|----------|
| [data/raw/](data/raw/) | Source data as downloaded, plus tidy CSV extracts |
| [data/external/](data/external/) | Population, urbanization, economy and resource-potential tables with sources |
| [docs/](docs/) | Methodology, assumptions, data sources, decisions, uncertainty |
| `src/` | C++ model engine (forecast, allocation, Monte Carlo) |
| `dashboard/` | Interactive dashboard |

## Build

All code is C++20, built with CMake.
CPU parallelism uses OpenMP.
An optional CUDA path accelerates the Monte Carlo and sensitivity ensembles.
Build instructions will be added once the build is in place.

## Team

See [TEAM.md](TEAM.md).
