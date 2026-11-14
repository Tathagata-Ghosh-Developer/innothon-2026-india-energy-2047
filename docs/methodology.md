# Methodology

This document explains how the forecast and the regional strategy are built.

| Topic | Document |
| --- | --- |
| Design choices and rejected alternatives | [decisions.md](decisions.md) |
| Data provenance | [data_audit.md](data_audit.md) |
| Assumptions and limitations | [assumptions_and_uncertainty.md](assumptions_and_uncertainty.md) |
| Numbers | [results.md](results.md) |

```text
data (generation, drivers, resources, costs)
   |
   v
cross-country models  --bootstrap-->  parameter uncertainty
   |                                      |
   v                                      v
Monte Carlo ensemble (CPU OpenMP or GPU CUDA): national + 5 regions, 2026-2047
   |
   v
2047 regional requirement (planning quantile)
   |
   v
least-cost LP (HiGHS)  +  resource-score allocation  +  policy sensitivity sweep
   |
   v
dashboard (Dear ImGui + ImPlot) and output tables
```text

## 1. What is forecast

The target is India's annual electricity generation (TWh) up to 2047, nationally and for the five grid regions.
Only energy generated is used, as the competition requires.
Installed capacity and consumption never enter the forecast.

## 2. National forecast

### 2.1 Cross-country panel

| Item | Source | Coverage |
| --- | --- | --- |
| Total electricity generation (TWh) | Ember yearly electricity data, "Total generation" | 1990 to 2025 |
| Population | World Bank WDI, SP.POP.TOTL | 1990 to 2025 |
| GDP per capita, PPP, constant 2021 international $ | World Bank WDI, NY.GDP.PCAP.PP.KD | 1990 to 2025 |
| Urban population share | World Bank WDI, SP.URB.TOTL.IN.ZS | 1990 to 2025 |

The panel keeps countries with at least 1 million people and at least 10 complete years.
That leaves 153 countries and 4,370 country-years.
The variable modelled is generation per person, e = TWh x 10^9 / population (kWh per person).

### 2.2 Models

All three models predict the change in India's generation from its last observed year.
In 2025 that is 2,080 TWh, or 1,421 kWh per person.
Anchoring on the latest observation means India's own level comes from data, not from the cross-country average.

**FE-Quad.** Within-country fixed-effects regression:

ln e(i,t) = alpha(i) + b1 x + b2 x^2 + g u + d tau

Here x = ln(GDP per capita / 10,000), u = urban share and tau = (year - 2000) / 10.
The income elasticity, b1 + 2 b2 x, falls as income rises.

**FE-Gompertz.** Within-country fixed effects with a saturating income response:

ln e(i,t) = alpha(i) - a exp(-b y) + c u + d tau

Here y = GDP per capita in thousand $.
The model is linear in (a, c, d) for a fixed b.
So b is found by a one-dimensional search (log grid plus golden section), and (a, c, d) by least squares at each b.
This variable-projection fit has no starting-value problem.

**Damped trend.** An additive damped-trend exponential smoothing model on ln(India total generation).
It is fitted by a parallel grid search over its smoothing and damping parameters.
It is a statistical benchmark that ignores the drivers.

Full-sample estimates (bootstrap 90 percent intervals in the dashboard):

| Model | Income | Urban share | Trend per decade | Within R^2 | Elasticity at India 2025 |
| --- | --- | --- | --- | --- | --- |
| FE-Quad | b1 = 0.444, b2 = -0.137 | g = 2.63 | d = 0.023 | 0.465 | 0.44 |
| FE-Gompertz | a = 2.37, b = 0.223 | c = 2.46 | d = 0.032 | 0.481 | 0.57 |
| Damped trend | alpha = 1.00, beta = 0.15, phi = 0.965 | | | | |

### 2.3 Parameter uncertainty

A cluster bootstrap resamples whole countries, with 2,000 replicates per model.

- **FE-Quad:** each country's within-country sufficient statistics are computed once, so a replicate is a sum of 4x4 matrices.
- **FE-Gompertz:** each replicate repeats the variable-projection fit.

Replicates run in parallel with OpenMP.
Both models together take 2.2 s on 16 threads, against an estimated 20 s on one thread.

### 2.4 Backtest

Each model is fitted on data up to an origin year.
It then predicts India's generation for every later year, using the observed drivers.
This measures model error only, not error in projecting the drivers.

| Origin | Horizon | FE-Quad MAPE | FE-Gompertz MAPE | Damped MAPE | FE-Quad 2025 error | FE-Gompertz 2025 error | Damped 2025 error |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 2005 | 20 years | 6.7% | 12.0% | 30.7% | 13.8% | 31.1% | 53.0% |
| 2010 | 15 years | 9.3% | 4.0% | 17.4% | 11.4% | 0.6% | 29.0% |
| 2015 | 10 years | 2.3% | 3.0% | 4.4% | 3.3% | 6.5% | 4.8% |

The pass bar is a 2025 error below 10 percent from the 2010 origin.
FE-Gompertz passes and FE-Quad narrowly misses.
Ensemble weights are proportional to 1 / MAPE from the 2010 origin: 0.26, 0.60 and 0.14.

## 3. Regional requirement

A region's generation is not its requirement, because power flows between regions.
Each of the 36 states and union territories gets a weight:

w(s,t) = population share(s,t) x exp( f(y(s,t)) + coefficient x [u(s,t) - u(s,base)] )

| Term | Meaning |
| --- | --- |
| f | The same income response as the national model in that scenario (FE-Quad or FE-Gompertz) |
| y(s,t) | State GDP per capita, PPP |
| u(s,t) - u(s,base) | The state's projected change in urban share |
| coefficient | The model's urban-share coefficient |

Regional requirement = national generation x (sum of w over the region) / (sum of all w).
The regions add up exactly to the national total.

**State income.** In the base year, state income relative to the national average comes from the organiser dataset (state GSDP and population, 2024).
Only these two columns of that file are read.
Relative income gaps then close at a speed estimated from the same data (2006 to 2024), by long-difference beta-convergence over 32 states:

| Estimate | Value |
| --- | --- |
| lambda | 0.021 per year |
| Bootstrap s.e. | 0.011 |
| Half-life | about 33 years |

Nominal GSDP is acceptable because a common price level cancels in the ratio.

**Urbanisation.** Base-year urban levels do not enter the split.
The urban coefficient is estimated from changes within countries over time, not from cross-sectional differences.
Applying it to cross-state levels exaggerated regional differences.
State urban paths come from the Technical Group projections (to 2036) and are then extended by their 2031-2036 trend.

**Population.** State population shares come from the Technical Group on Population Projections (1 July series, 2011-2036).
After 2036 each state's 2031-2036 log-share trend continues, and the shares are renormalised every year.

## 4. Monte Carlo ensemble

Each draw is one consistent future:

| Input | Distribution |
| --- | --- |
| Population path | UN WPP 2024 medium anchored on 2025; normal shift scaled so low/high is a 90 percent band |
| Urban share path | UN WUP 2018 change anchored on 2025; normal shift with s.d. growing to 2 points by 2047 |
| GDP per capita growth | Mean path (4.90 percent to 2030, then slowing by 1.5 points by 2047), persistent shift (s.d. 0.75 points) and AR(1) annual shocks (s.d. 3.18 percent) |
| Model | FE-Quad, FE-Gompertz or damped trend, by backtest weight |
| Model parameters | One of 2,000 bootstrap replicates |
| Income convergence speed | Normal(0.021, 0.011) |
| New loads | Uniform between dashboard limits (0 by default), ramped to 2047 |

Random numbers come from the Philox 4x32-10 counter-based generator, keyed by draw index.
A draw therefore has identical inputs on the CPU and the GPU.
Outputs (national and five regions, 22 years) go into 1,024-bin histograms.
Quantiles are read from those histograms.

**Optimisation:**

- **CPU path:** OpenMP over draws, thread-private histograms, and a parallel merge.
- **GPU path:**
  - Inputs live in constant memory, which broadcasts to every thread of a warp.
  - Histograms are updated by atomic add.
  - The model choice is shared by each group of 32 draws, so a warp never diverges on the model branch.
  - The kernel is compiled to machine code for the RTX 30 series (sm_86).

A parity test checks that CPU double, CPU float and GPU float agree within 0.2 percent at the 5th, 50th and 95th percentiles of every series.

| Run | 1,048,576 scenarios |
| --- | --- |
| CPU, 1 thread | 22.7 s |
| CPU, 16 threads | 2.07 s |
| GPU, RTX 3070 Ti Laptop | 7.5 ms |

## 5. Regional generation strategy for 2047

The 2047 requirement is taken at a planning quantile (the median by default) and split by the regional medians.
Two allocation methods then meet it.

### 5.1 Least-cost linear program (HiGHS)

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

### 5.2 Resource-score allocation

The national total of each source is shared across regions in proportion to a score, within the same bounds, by water-filling:

| Sources | Score |
| --- | --- |
| Solar, wind, offshore wind, hydro, bioenergy | Usable generation potential |
| Coal, gas | 2024 generation (existing fleet) |
| Nuclear | Reactor units |

Every score can be checked by hand.
Comparing it with the least-cost plan shows where transmission limits and costs move generation.

### 5.3 Policy sensitivity

Thirteen settings (policy shares, land use, corridors, build rates, carbon price, costs and the requirement quantile) are each moved to a low and a high value.
The 26 resulting least-cost problems are solved in parallel with OpenMP in about 7 ms.
