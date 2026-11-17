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

  The base-year split uses population and income only.
  Urbanisation enters through each state's projected change in urban share.
  We first applied the urban coefficient to cross-state urban levels, which gave the Southern region 33 percent of the 2026 requirement.
  That is implausible, because the coefficient is estimated from changes within a country over time, not from differences between regions.
  It was revised on 2026-11-15.
  Since 2026-11-16 the split has no state urban term and uses only the challenge data (D12).
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

**Constraints added after first runs.**

- **Build-rate limits.** Without them the optimiser chose extreme corner solutions.
  The limits are based on India's record annual generation increase for each source.
- **National coal ceiling.** A national ceiling, with a looser regional limit, replaces per-region caps.
  This lets coal run where the fleet and reserves are.
- **Unserved-energy slack.** A high-cost slack keeps sensitivity runs feasible.
- **Independent check.** Every solution is checked against the constraint rows.
  That check found a matrix-start indexing bug in the first version.

## D5. Policy targets enter as generation shares, never as capacity

**Choice.** Policy settings are expressed as shares of 2047 generation: the non-fossil share, the nuclear share and the coal ceiling.
The dashboard exposes them as sliders.
Capacity targets (500 GW non-fossil by 2030, 100 GW nuclear by 2047) appear only as labelled context and never enter a calculation.

Resource potentials published in GW are converted to annual generation potential (TWh per year) with sourced capacity factors.
That conversion is labelled wherever it appears.
It is used only as an upper bound on generation.
Their ruling of 16 November allows resource potentials for justification only, not estimation. Asked whether they may bound the mix optimisation, the organisers replied "Use insights from this data source" (D12). That answer is not explicit, and a clarification has been requested; until then they stay as limits in the supply plan and never enter the requirement.

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

## D6b. Measured performance

| Run | 1,048,576 scenarios |
| --- | --- |
| 1 CPU thread | 20.0 s |
| 16 threads | 2.21 s |
| RTX 3070 Ti Laptop GPU | 7.7 ms |

Run of 16 November 2026 (`outputs/timings.csv`; the single-thread time is scaled from a 1/16 run; CPU timings vary with background load).

The GPU kernel builds machine code for sm_86 directly (`86-real`).
The toolkit (CUDA 13.5) is newer than the driver (CUDA 13.3), so the driver could not JIT-compile its PTX.

## D7. Dashboard: Dear ImGui and ImPlot

**Choice.** A native C++ dashboard (Dear ImGui, ImPlot, GLFW, OpenGL 3) for the midpoint review.
For the final, the same code is compiled with Emscripten to a browser build that runs the CPU path and loads precomputed GPU results.
A recorded walkthrough video is kept as a fallback for the live demo.

**Rejected.** A separate web front end in another language, because the project is C++ only.

## D8. Every number has a provenance row

**Choice.** [data_audit.md](data_audit.md) lists every dataset and column, its unit, source URL, and whether it is a generation quantity.
No column enters the model without a row in that table.

## D9. Independent review and fixes (2026-11-15)

An independent read-only review of the numerical core found these problems, all now fixed.

| Severity | Finding | Fix |
| --- | --- | --- |
| High | Hydro capacity factors were never read: the CSV label is `hydro_all`. Every region used a 100 percent capacity factor, so usable hydro was 948 TWh instead of about 350 TWh. | Match `hydro_all`. Fail loudly if no regional capacity factor is read. |
| Low-medium | Share rows on gross generation let the solver circulate flow round a corridor, burning energy as losses near infeasibility. | Any circulating flow is now reported as infeasible. |
| Low | Relative state income is from 2024 but was treated as the 2025 base year. | One extra year of convergence is applied. |
| Low | The score allocation ignored the shared solar and wind resource for firmed RE, and ran even when the least-cost plan was infeasible. | Firmed RE is capped by the remaining resource; the score allocation is skipped when the plan is infeasible. |
| Low | Convergence speed returned 0 for overshooting bootstrap replicates; float-to-int histogram binning was undefined for out-of-range values. | Return NaN and drop the replicate; clamp before the cast. |

**Effect on results.**

- The requirement forecast did not change.
- The base 2047 mix changed: hydro went from 17.7 to 7.7 percent and coal from 25.3 to 30.0 percent.
- The non-fossil minimum now binds, and the base plan has no headroom at the P90 requirement.

This led to the **Robust** policy preset, which is the recommended strategy.

## D10. India-only primary core; cross-country model becomes a check (2026-11-16)

**Trigger.** At the kickoff (13 Nov 2026) the organisers said the core analysis of the 2047 energy prediction must use only the two challenge datasets (National Power Portal and the Renewable India dataset); literature may supply growth rates and constraints.
D1 used Ember and World Bank data in the core, so it no longer qualifies as the primary forecast.

**Choice.** The primary forecast is an India-only driver model:

- ln(NPP generation per person) on ln(real GSDP per person), with a FY2020-21 dummy. The income elasticity is the only parameter estimated, from FY2011-12 to FY2023-24 (one price base) and FY2006-07 to FY2023-24 (spliced); the bootstrap mixes both windows equally.
- Urbanisation enters only as its excess over the income-implied path, gamma (du - kappa dx), with gamma from the literature (normal 1.0, s.d. 0.5). gamma = 0 is the pure two-dataset ablation, shown in every output.
- Moving-block (3-year) residual bootstrap, because the residuals are serially correlated.
- Base FY2023-24, the last year of the NPP all-India series; FY2024-25 and FY2025-26 serve as a holdout.
- Scope is NPP utility generation; captive plants are a separately labelled layer. (The layer was removed after the organisers' ruling of 16 November; see D12.)
- The damped trend is a benchmark, not an ensemble member, and backtest errors are not used as weights.
- The regional split stays driver-based (population, income convergence, urbanisation); NPP regional generation is compared with it as a transfer gap and is never fitted. (Since D12 the split uses challenge-dataset population and income only, with NPP's grid regions and no state urban term.)
- The cross-country model stays in the code and the dashboard as an independent check on non-core data. (Since D12 it is used for justification only, not for estimation.)

**Reason.** The decision went through a structured review with five independent critiques, anonymous peer review and a synthesis.
All reviewers agreed on the points above.
Key arguments:

- With 18 annual points only one parameter is identified. The elasticity is stable across windows (0.78 to 0.90 including first differences), while the income path moves the 2047 figure by a quarter to a third, so effort belongs in driver scenarios, not in more model forms.
- Urban share and income moved together in India; a free urban coefficient is unidentified, and the post-2011 urban series is an interpolation. Taking gamma from the team's own cross-country regression would bring non-core data back into the core, so it comes from published studies instead.
- NPP regional generation records where plants are (the West and East export), so a regional model fitted to it would be confounded.

**Rejected.**

- A hierarchical regional panel on NPP regional generation: conventional sources only, from FY2018-19, and confounded by inter-regional trade.
- Nowcasting the base to FY2025-26: NPP has no renewable generation for those years; they are more useful as a holdout.
- Correcting for past energy shortages with CEA power-supply-position data: those are requirement (consumption-side) quantities, which the rules exclude from forecasting and comparison. The direction of the bias is stated instead, and the FY2011-12 window and the elasticity-decline setting bound it.
- Inverse-MAPE weights from the India backtest: seven short origins give noisy weights.

**Open.** A written query to the organisers asks whether external population and urban projections, a literature urbanisation coefficient, the captive layer and the cross-country check are acceptable as stated.
The organisers answered in writing on 16 November 2026; the ruling and how it is applied are in D12.

## D11. Independent code review and fixes (2026-11-16)

An independent review of the India-only core, the Monte Carlo and the allocation found these problems, all now fixed.

| Severity | Finding | Fix |
| --- | --- | --- |
| Medium | The base-year anchor was a threshold rule: when the FY2023-24 residual (+0.070 log points) exceeded one residual s.d., the anchor moved to the fitted value plus the mean residual of the last three years. A threshold switch makes the forecast jump with a small change in the data, and the ensemble carried no model error, so its near-term band was too narrow: the FY2024-25 holdout fell outside the 90 percent range. | The Monte Carlo carries an AR(1) residual of ln(kWh per person) that starts at the FY2023-24 residual and reverts to the fitted relation, with persistence rho = 1 - DW / 2 = 0.51 and innovation s.d. = residual s.d. x sqrt(1 - rho^2) = 0.032. Central paths use the expected residual path. The anchor rule is removed (`anchor_shift` = 0 in `outputs/india_fit.csv`). |
| Medium | Several organiser GSDP values are copied unchanged from the previous year (Telangana, Jammu and Kashmir, Andaman and Nicobar Islands and Lakshadweep for 2006 to 2010, Nagaland for 2007). Chaining national income growth counted them as zero growth. | Copied values are left out of national income growth; population growth still uses every matched state. The 2011 price-base splice now requires both neighbouring years. |
| Low | The bootstrap resampled every residual, including the pandemic year whose residual is zero by construction, without a degrees-of-freedom correction, which understates the spread. | It resamples the residuals of the years without the dummy, rescaled by the square root of (pooled residuals) / (years minus regressors). |
| Low | The first-difference elasticity of the spliced window used the imputed 2011 change, which is not observed. | That change is left out. |
| Low | The urbanisation prior was censored at zero (negative draws set to zero), which puts a point mass at gamma = 0. | Negative draws are redrawn (a truncated normal). |
| Low | The persistence of income-growth shocks paired years across the gap left by the pandemic years. | Only consecutive retained years are paired. |
| Low | The cost-CO2 trade-off sweep inherited the policy's carbon price, so the cap's dual was net of that price. | The sweep sets the carbon price to zero; the cap's dual is the marginal abatement cost of resource cost alone. |
| Low | If the `std::thread` fallback of the CPU Monte Carlo (builds without OpenMP) could not start a thread, the program terminated; the web build used fewer India bootstrap replicates than the command line, so the browser and the command line could show different parameters. | Workers that cannot be started run on the calling thread; the India bootstrap keeps 2,000 replicates in every build. |
| Low | The dashboard switched to a requested forecast core before it had loaded, so a failed load could leave the page without controls; the national chart reset its year axis on every frame, so it could not be zoomed; the elasticity-decline control went up to 0.5. | The current core stays on screen until the new one has loaded; the year axis is set once; the decline control is capped at 0.3. |

**Checks added.**

- `tests/test_india.cpp`: copied GSDP rows are ignored, and both holdout years fall inside the model's 90 percent range.
- `tests/test_pareto.cpp` (new with the CO2 cap): slack and binding caps, the cap's dual against the finite-difference slope, a monotone frontier.
- `tests/test_mc_parity.cpp` now exercises the model-error path on the CPU and the GPU.
- The test suite has five tests (mc_parity including the GPU, allocation, storage, pareto, india).
- The CUDA kernel passes the Compute Sanitizer memcheck, initcheck, racecheck and synccheck tools with no errors.

**Effect on results** (`outputs/` of 16 Nov 2026; the earlier values are the copies committed in `docs/beamer/data/` with b3d6754).

| Quantity | Before | After |
| --- | --- | --- |
| FY2047-48 median (P5 to P95), TWh | 4,894 (3,248 to 7,526) | 4,703 (3,217 to 7,002) |
| Elasticity, FY2006-07 window; first differences | 0.896; 0.869 | 0.866; 0.787 |
| Elasticity used (bootstrap mixture), median and 90 percent interval | 0.870 (0.747 to 0.950) | 0.850 (0.731 to 0.939) |
| Holdout errors FY2024-25 / FY2025-26 | -4.0% / +1.6%; FY2024-25 outside the 90 percent range | -3.3% / +0.5%; both inside |
| Base policy, unmet at the median | 28 TWh (Eastern region) | 0 |
| Robust policy: cost, CO2, unmet at P90 | 188 bn USD, 1,114 Mt, 1,067 TWh | 176 bn USD, 993 Mt, 638 TWh |

The FY2011-12 elasticity (0.828, s.e. 0.063) does not change.
The base policy now meets the median requirement, so the case for the Robust policy rests on cost (37 bn USD a year less), CO2 (377 Mt less) and the shortfall at P90 (638 instead of 1,529 TWh), not on adequacy at the median.

**New capability.** The least-cost LP accepts an optional national CO2 cap whose dual is the marginal abatement cost, and an epsilon-constraint sweep traces the cost-CO2 trade-off ([methodology.md](methodology.md), section 6.5; [results.md](results.md), section 3.5).

## D12. Organisers' data ruling and how it is applied (2026-11-16)

**Trigger.** On 16 November 2026 the organisers answered the written query of D10 in writing:

1. External population and urbanisation projections may be used if they are reasonably consistent with the state-level population growth in the challenge (GitHub) dataset; UN India-level growth is a reference.
2. The urbanisation coefficient is the team's own judgment.
3. Captive generation is not allowed: use generation from the NPP dataset only, and no external data for estimation.
4. The cross-country model may be used for justification only, not for estimation.
5. The regional split must use the challenge dataset.
6. Resource potentials may be used for justification only, not for estimation.

The headline must also be the country's total in TWh, not generation per person.

**Choice.** What changed in the code (commit 6c7eb56):

- **Regional split from the challenge data** (`src/regional/states.cpp`, `Pipeline` constructor in `src/pipeline/pipeline.cpp`).
  State populations start from the dataset's 2024 values and grow at the UN WPP 2024 medium growth of India plus the state's own 2019-2024 offset from the dataset's national growth, g_s = g_UN + (g_s,2019-24 - g_India,2019-24); the shares are renormalised every year.
  Each state takes the grid region NPP reports it under (`data/raw/npp/npp_state_generation_fy.csv`), and the dataset Zone (Central as Western) only where NPP lists no state.
  NPP prevails where the two disagree: Andaman and Nicobar Islands are Eastern (Zone South), Rajasthan Northern (Zone West) and Sikkim Eastern (Zone North-East); Chandigarh, which NPP does not list, is Northern by its Zone.
  Ladakh and Dadra and Nagar Haveli and Daman and Diu have no rows in the dataset and are left out, so the split has 34 states and union territories instead of 36.
  The Technical Group projections and `data/external/state_region_map.csv` no longer feed the split.
- **No state urbanisation term.** The dataset has no state urban share, so the term is removed from the regional weights (`state_log_index` in `src/forecast/mc.hpp`). The national urban effect (gamma) stays in the national total that is split.
- **Population consistency check.** The new `outputs/population_consistency.csv` compares the dataset's growth, the model's paths and UN WPP 2024 medium for India and every state; the Technical Group appears in it as a reference column only.
  India grew 1.12 percent a year in the dataset over 2011-2024 and 0.97 percent over 2019-2024; the model path grows 0.83 percent a year over 2024-2030 and 0.60 percent over 2024-2047, equal to UN WPP 2024 medium.
  Every state's 2024-2030 path is within 0.14 points a year of its own 2019-2024 growth in the dataset.
  The dataset's 2024 total is 1.399 billion, against 1.403 billion in the Technical Group projection for 1 July 2024.
- **Captive layer removed** from the code, the outputs (`india_fit.csv`, `core_comparison.csv`, `site_summary.json`), the dashboard and the landing page.
- **Cross-country model for justification only.** Its label (`core_name()` in `src/pipeline/pipeline.hpp`), the dashboard and the landing page say so; no number from it enters the forecast, the split or the plan.
- **Headline in total TWh.** The dashboard's per-person card is replaced by "Increase over FY2023-24" (+3,005 TWh), and `site_summary.json` carries `increase_twh`. Generation per person remains the quantity the regression explains, inside the model.
- **Checks.** `tests/test_regions.cpp` (new) checks the state-to-region mapping against the two files, that the shares sum to 1 in every year, that the first-year shares equal the dataset's 2024 shares and that the national forecast does not depend on the split. The suite has seven tests (mc_parity, allocation, storage, pareto, india, regions, site_format).

**What stays the same, and why.**

- **The national forecast**: 4,703 TWh median in FY2047-48 (P5 3,217, P95 7,002), from 1,698 TWh in FY2023-24. Its estimation already used only NPP generation and the organiser population and GSDP. India's population and urban paths are UN projections, which point 1 allows and which the consistency check supports; the urbanisation coefficient is the team's judgment (point 2), kept as the prior gamma ~ N(1.0, 0.5), truncated at zero, from `data/external/literature_priors.csv`. The split divides the national total and does not change it.
- **The recommended plan**: the Robust policy keeps 78.3 percent non-fossil, 993 Mt CO2, about 176 bn USD a year, no unmet energy at the median and 638 TWh unmet at P90 (none with 45 percent usable solar land). Its national mix hardly moves because the national requirement is unchanged; the regional requirements, and with them regional siting and corridor flows, do change.
- **The trade-off**: a CO2 cap still takes the Robust plan from 993 to 643 Mt a year (-35 percent) until the coal fleet floor binds; it now costs 0.65 bn USD a year more (0.67 before) and the highest marginal abatement cost is 6.32 USD per tonne (3.97 before).
- **Storage and land** of the Robust plan: 447 GW, 3,042 GWh and 6.8 hours; 19,990 km² (0.61 percent of India).
- **Resource potentials** (point 6) never entered the forecast or the split; they bound the supply plan only, which the organisers' answer to question b below allows.

**Effect on results** (before: the copies in `docs/beamer/data/` committed with 07f8b3d; after: `outputs/` of this run).

| Quantity | Before | After |
| --- | --- | --- |
| FY2047-48 national median (P5 to P95), TWh | 4,703 (3,217 to 7,002) | unchanged |
| States and union territories in the split | 36 (Technical Group) | 34 (challenge dataset) |
| Northern median, TWh (share of requirement) | 1,341 (28.5%) | 1,405 (29.9%) |
| Western | 1,248 (26.6%) | 1,226 (26.1%) |
| Southern | 1,144 (24.3%) | 1,068 (22.7%) |
| Eastern | 805 (17.1%) | 840 (17.9%) |
| North-Eastern | 161 (3.4%) | 161 (3.4%) |
| Captive layer | about 5,320 TWh including captive plants | removed |
| Robust plan: non-fossil, CO2, cost, unmet at P90 | 78.3%, 993 Mt, 176 bn USD, 638 TWh | unchanged |
| Robust plan: Western exports; Eastern imports | 258 TWh; about 210 TWh | 280 TWh; 246 TWh |
| Base plan: CO2; regional marginal costs | 1,370 Mt; 70 to 76 USD/MWh | 1,371 Mt; 101 to 109 USD/MWh |
| CO2 cap to 643 Mt: extra cost; highest marginal abatement cost | 0.67 bn USD/yr; 3.97 USD/t | 0.65 bn USD/yr; 6.32 USD/t |
| Robust plan storage, daily cycle | 447 GW, 3,039 GWh | 447 GW, 3,042 GWh |

**Rejected.**

- Keeping the Technical Group state projections: they are not the challenge dataset (point 5).
- Carrying each state's own 2019-2024 growth to 2047: India would then grow at 0.97 percent a year to 2047, far above the UN reference (0.60 percent). The offset form keeps the dataset's differences between states and UN's national level.
- Using the dataset Zone for every state: its six zones do not match NPP's five grid regions, and it places Rajasthan in the West, Sikkim in the North-East and the Andaman and Nicobar Islands in the South, while NPP reports their generation in the Northern and Eastern regions. The Zone is kept as the fallback for a state NPP does not list.
- A state urban term from external state urban projections: point 5 requires the split to use the challenge dataset, which has no state urban share.

**Follow-up questions and answers.** Three follow-up questions were sent to the organisers on 16 November 2026, and they answered in writing before the midpoint review (answers quoted verbatim). Answers (a) and (c) are clear; answer (b) is not:

- (a) Generation only: the kickoff guidance asked for energy generated only, not consumption, and we continue to use NPP generation only. We asked the organisers to confirm this is correct. Answer: "Generation data".
- (b) Supply mix: published resource potentials (solar, wind and hydro, converted from GW to TWh a year with capacity factors), costs and emission factors from public literature (CEA, IEA, MNRE) act only as limits and prices inside the least-cost mix; the 2047 total requirement uses none of them. We asked whether this is acceptable, or whether the potentials should only be shown beside the mix as a justification. Answer: "Use insights from this data source". We read this as allowing the published data to inform the mix as limits and prices, which is how we use them. The answer is not explicit, and the organisers' ruling of 16 November allows potentials only to justify results, not for estimation. A clarification has been requested: whether the two-dataset rule covers only the 2047 requirement (Part 1) or also the mix (Part 2), and whether published figures may decide the mix or only justify it.
- (c) Regions: we use the five grid regions reported by NPP (Northern, Western, Southern, Eastern, North-Eastern). The challenge dataset's Zone column has six zones, including Central, and the kickoff mentioned four regional blocks. We asked whether the NPP five-region split is acceptable. Answer: "Four, five, or six regions - either is fine".

Answers (a) and (c) need no change: the forecast already uses NPP generation only, and the results stay on NPP's five grid regions. Answer (b) awaits the clarification. Until then the published potentials, costs and emission factors stay as limits and prices in the mix and never enter the 2047 requirement. The kickoff guidance supports this reading: publicly available literature may be used for the growth rates and constraints of each energy source, and only the core 2047 energy prediction must use the two datasets. The two challenge datasets contain no technology costs, emission factors or resource potentials, and their installed capacity may not be used. If the organisers rule that published figures may only justify the mix, Part 1 (the 2047 requirement, national and regional) is unaffected, and only the way the mix is chosen changes.
