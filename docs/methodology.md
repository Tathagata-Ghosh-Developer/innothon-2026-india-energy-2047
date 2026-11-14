# Methodology

This document explains how the forecast and the regional strategy are built.
Design choices and rejected alternatives are in [decisions.md](decisions.md).
Data provenance is in [data_audit.md](data_audit.md).

## 1. What is forecast

The target is India's annual electricity generation (TWh) up to 2047, nationally and for the five grid regions.
Only energy generated is used, as the competition requires.
Installed capacity and consumption never enter the forecast.

## 2. National forecast

### 2.1 Cross-country panel

| Item | Source | Coverage |
|------|--------|----------|
| Total electricity generation (TWh) | Ember yearly electricity data, "Total generation" | 1990 to 2025 |
| Population | World Bank WDI, SP.POP.TOTL | 1990 to 2025 |
| GDP per capita, PPP, constant 2021 international $ | World Bank WDI, NY.GDP.PCAP.PP.KD | 1990 to 2025 |
| Urban population share | World Bank WDI, SP.URB.TOTL.IN.ZS | 1990 to 2025 |

The panel keeps countries with at least 1 million people and at least 10 complete years.
The current panel has 153 countries and 4,370 country-years.
The variable modeled is generation per person, e = TWh x 10^9 / population (kWh per person).

### 2.2 Models

All three models predict the change in India's generation from its last observed year (2025: 2,080 TWh, 1,421 kWh per person).
Anchoring on the latest observation means India's own level is taken from data, not from the cross-country average.

**FE-Quad.** Within-country fixed-effects regression:

ln e(i,t) = alpha(i) + b1 x + b2 x^2 + g u + d tau

Here x = ln(GDP per capita / 10,000), u = urban share and tau = (year - 2000) / 10.
The income elasticity, b1 + 2 b2 x, falls as income rises.
That fall is how saturation enters this model.

**FE-Gompertz.** Within-country fixed effects with a saturating income response:

ln e(i,t) = alpha(i) - a exp(-b y) + c u + d tau

Here y = GDP per capita in thousand $.
The model is linear in (a, c, d) for a fixed b.
So b is found by a one-dimensional search (log grid plus golden section), and (a, c, d) by least squares at each b.
This variable-projection fit has no starting-value problem.

**Damped trend.** An additive damped-trend exponential smoothing model on ln(India total generation).
It is a statistical benchmark that ignores the drivers.

Current full-sample estimates:

| Model | Income | Urban share | Trend per decade | Within R^2 | Elasticity at India 2025 |
|-------|--------|-------------|------------------|------------|--------------------------|
| FE-Quad | b1 = 0.444, b2 = -0.137 | g = 2.63 | d = 0.023 | 0.465 | 0.44 |
| FE-Gompertz | a = 2.37, b = 0.223 | c = 2.46 | d = 0.032 | 0.481 | 0.57 |

### 2.3 Parameter uncertainty

Parameter uncertainty comes from a cluster bootstrap that resamples whole countries, with 2,000 replicates per model.
For FE-Quad, each country's within-country sufficient statistics are computed once, so a replicate is a sum of 4x4 matrices.
For FE-Gompertz, each replicate repeats the variable-projection fit.
Replicates run in parallel with OpenMP.
2,000 replicates of both models take 2.8 s on 16 threads, against an estimated 20 s on one thread.

### 2.4 Backtest

Each model is fitted on data up to an origin year.
It then predicts India's generation for every later year, using the observed drivers.
The error measured is model error only, not error in projecting the drivers.

| Origin | Horizon | FE-Quad MAPE | FE-Gompertz MAPE | Damped trend MAPE | FE-Quad 2025 error | FE-Gompertz 2025 error | Damped 2025 error |
|--------|---------|--------------|------------------|-------------------|--------------------|------------------------|-------------------|
| 2005 | 20 years | 6.7% | 12.0% | 30.7% | 13.8% | 31.1% | 53.0% |
| 2010 | 15 years | 9.3% | 4.0% | 17.4% | 11.4% | 0.6% | 29.0% |
| 2015 | 10 years | 2.3% | 3.0% | 4.4% | 3.3% | 6.5% | 4.8% |

The pass bar is a 2025 error below 10 percent from the 2010 origin.
FE-Gompertz passes (0.6%) and FE-Quad narrowly misses (11.4%).
The damped trend fails over long horizons, which is why it gets a small ensemble weight.
The ensemble weights are set from these errors (see section 4).

## 3. Regional requirement

A region's generation is not its requirement, because power flows between regions.
Each state's requirement weight is:

w(s,t) = population share(s,t) x exp(f(y(s,t), u(s,t)))

Here f is the same per-person response as the national model.
y(s,t) is state GDP per capita and u(s,t) is the state urban share.
Regional requirement = national generation x (sum of w over the region's states) / (sum of all w).
The regions therefore add up exactly to the national total.

State income relative to the national average converges slowly.
From the organiser dataset (state GSDP and population, 2006 to 2024), the relative income gap closes at lambda = 0.021 per year (bootstrap s.e. 0.011, 32 states).
That is a half-life of about 33 years.
Nominal GSDP is acceptable here because a common price level cancels in the ratio.
The installed-capacity columns of that file are never read.

## 4. Monte Carlo ensemble

Each draw is one consistent future:

| Input | Distribution |
|-------|--------------|
| Population path | UN WPP 2024 medium, with a draw-level normal shift scaled to the low/high band |
| Urban share path | Projection plus a draw-level normal shift |
| GDP per capita growth | Scenario mean path, plus a persistent level shift and AR(1) annual shocks |
| Model | FE-Quad, FE-Gompertz or damped trend, by ensemble weight |
| Model parameters | A random bootstrap replicate |
| Income convergence speed | Normal(0.021, 0.011) |
| New loads (optional) | Uniform between dashboard limits, ramped linearly to 2047 |

Random numbers come from the Philox 4x32-10 counter-based generator, keyed by draw index.
A draw therefore has identical inputs on the CPU and the GPU.
Outputs (national and five regions, every year) go into 1,024-bin histograms.
Quantiles come from those histograms.

The CPU path uses OpenMP with thread-private histograms.
The GPU path uses CUDA, with the inputs in constant memory and histogram updates by atomic add.
The model choice is shared by each group of 32 draws, so a GPU warp never diverges on the model branch.

| Path | 1,048,576 draws (parity fixture) |
|------|----------------------------------|
| CPU, 16 threads, double | 1,197 ms |
| CPU, 16 threads, float | 859 ms |
| GPU, RTX 3070 Ti Laptop, float | 5.4 ms |

A parity test checks that CPU double, CPU float and GPU float agree within 0.2 percent at the 5th, 50th and 95th percentiles of every series.

## 5. Regional generation strategy

(In progress.) Resource-based scoring table plus a least-cost linear program cross-check; see [decisions.md](decisions.md), D4 and D5.
