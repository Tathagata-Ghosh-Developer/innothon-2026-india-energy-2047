# Design decisions

Recorded 2026-11-15. Each decision lists the choice, the reason, and the rejected alternatives.

## D1. Forecast the national requirement with a cross-country saturation model

**Choice.** Per-capita electricity generation is modeled as a saturating function of GDP per capita (PPP, constant prices) and the urban population share.
The model is fitted on a panel of about 150 countries, 1990 to 2024, using generation data only (Ember via Our World in Data).
National generation = projected population x projected per-capita generation.

Two further models run beside it:

- A within-country fixed-effects regression with a declining income elasticity.
- A damped-trend time-series model of India's own generation, as a sanity baseline.

**Reason.** India's own history (about 35 years) cannot show where growth slows.
Other countries that passed through India's projected 2047 income level can show it.
The saturating form limits the growth rate as income rises.
The time-series model checks that the structural models do not drift far from recent momentum.

**Rejected.** A state-level panel regression, because state generation series start around 2015 and give fewer than 10 points per state.

## D2. Validate with a backtest before trusting any 2047 number

**Choice.** Fit every model on data up to 2010 and predict 2011 to 2024 for India, using the observed population, GDP and urban share.
Report the absolute percentage error for 2024.
The pass bar is 10 percent.
The ensemble weights are inversely proportional to backtest error.

## D3. Separate regional requirement from regional production

**Choice.** Each region gets three numbers per year.

- **Requirement.** The national generation forecast times the region's share.
  The share comes from state population, GSDP per capita and urban share, run through the same per-capita response as the national model.
  No consumption data is used.
- **Production.** Where generation is sited, chosen by the allocation step according to regional resource strengths.
- **Net transfer.** Production minus requirement.
  It sums to zero nationally by construction.
  It is an output, capped by a stated inter-regional transfer limit.

Historical regional generation shares are used only to sanity-check the driver-based shares.

**Reason.** A region's generation is not its need.
The Eastern region exports coal power and the Northern region imports.
Splitting the two concepts keeps the forecast free of consumption data and makes the transfers explicit.

## D4. Allocation: transparent scoring first, least-cost LP as a cross-check

**Choice.**

1. A scoring table rates each region and source on sourced resource indicators (solar irradiance and potential, wind potential, hydro potential, coal reserves, nuclear sites, gas infrastructure).
   It distributes the national generation of each source across regions in proportion to those scores, within each region's resource limit.
2. A linear program (HiGHS solver) minimizes system cost subject to resource limits, a regional firm-generation floor, an emissions cap, and inter-regional transfer limits.
   It runs as a cross-check, and its dual values show which constraint binds in each region.

**Reason.** A scoring table can be checked by hand during questioning.
The linear program shows whether a least-cost system agrees.

## D5. Policy targets enter as generation shares, never as capacity

**Choice.** Policy settings are expressed as shares of 2047 generation: the non-fossil share, the nuclear share and the coal ceiling.
The dashboard exposes them as sliders.
Capacity targets (500 GW non-fossil by 2030, 100 GW nuclear by 2047) appear only as labelled context and never enter a calculation.

Resource potentials published in GW are converted to annual generation potential (TWh per year) with sourced capacity factors.
That conversion is labelled wherever it appears.
It is used only as an upper bound on generation.
We have asked the organisers to confirm that this conversion is acceptable.

## D6. Compute: C++20, OpenMP by default, optional CUDA

**Choice.** All code is C++20, built with CMake and MSVC 2022.
OpenMP parallelizes the bootstrap fits, the Monte Carlo ensemble and the sensitivity sweeps on the CPU.
A CUDA module, enabled with the CMake option `ENABLE_CUDA`, runs the same Monte Carlo and Sobol kernels on the GPU.

Both paths use one counter-based random number generator (Philox 4x32-10), so a draw with a given index gets the same inputs on CPU and GPU.
A parity test checks that CPU and GPU quantiles agree within tolerance.
We publish timings for 1 thread, 16 threads and the GPU.

**Reason.** The input data is small.
The parallel work is the uncertainty analysis: millions of scenario draws and the sensitivity sweeps over the allocation.
Those draws are independent, so they parallelize well, and the GPU lets the dashboard recompute the full ensemble while a slider moves.

## D7. Dashboard: Dear ImGui and ImPlot

**Choice.** A native C++ dashboard (Dear ImGui, ImPlot, GLFW, OpenGL 3) for the midpoint review.
For the final, the same code is compiled with Emscripten to a browser build that runs the CPU path and loads precomputed GPU results.
A recorded walkthrough video is kept as a fallback for the live demo.

**Rejected.** A separate web front end in another language, because the project is C++ only.

## D8. Every number has a provenance row

**Choice.** [data_audit.md](data_audit.md) lists every dataset and column, its unit, source URL, and whether it is a generation quantity.
No column enters the model without a row in that table.
