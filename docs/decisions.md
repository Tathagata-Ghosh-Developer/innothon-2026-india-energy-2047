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
Since 22 November 2026 (D15) the 100 GW nuclear target enters one optional policy, "Nuclear mission", as a floor on generation, never as capacity: the mission's utility reactors in CEA's road map, 90.588 GW x CUF 0.68 x 8.76 = 539.6 TWh. The recommended plan has no such floor.

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

## D13. Midpoint panel follow-up: requirement evidence, published figures in the mix (2026-11-19)

**Trigger.** At the midpoint review (18 November 2026) the panel asked five questions:

1. Is income growth the only factor for the 2047 requirement? If not, what else should be accounted for?
2. Is income data used state-wise, region-wise or for India as a whole? (Use state-wise data from trusted sources, name them, and add it if absent.)
3. Renewable energy data is missing or mostly empty. How did you manage?
4. How does the elasticity affect the result, and was it estimated state-wise, region-wise or for India as a whole?
5. What hurdles did you face, and how will you mitigate them?

Following the kickoff guidance, the team also decided that publicly available literature should set the growth rate and constraints of each energy source, and that published figures (MNRE/NISE/NIWE potentials, IEA and CEA costs, CEA emission factors) should decide the regional mix.
The official challenge page (unchanged since 4 November) frames the strategy around six sources, with natural gas as the Western Region's transition fuel and nuclear in the South.
The plan is `docs/plans/2026-11-18-post-midpoint-design.md` and `docs/plans/2026-11-18-post-midpoint-plan.md`.
The source hierarchy is unchanged: NPP and the Renewable India dataset are primary, and every other source is secondary and labelled.

**Choice.** What changed, task by task (commits 26e91e7 to this entry).

- **Elasticity sensitivities** (Task 3; questions 1 and 4). `ScenarioControls` gains `beta_shift` and `beta_window`. Five rows are added to `outputs/sensitivity_2047.csv`, each run on a copy of the default controls. The 2047 median moves as follows:
  - the elasticity one OLS standard error (0.063) lower or higher: 4,400 or 5,027 TWh (-6.4 or +6.9 percent);
  - the FY2011-12 or FY2006-07 window alone: 4,583 or 4,829 TWh;
  - the first-difference estimate: 4,392 TWh.
- **Growth decomposition** (Task 4; question 1). `outputs/growth_decomposition_2047.csv` splits the central path's growth from FY2023-24 to FY2047-48 (1,698 to 4,600 TWh, 0.997 log points). Income accounts for 87.5 percent of it, population 14.8 percent and urbanisation beyond income 4.8 percent. The fading of the high base-year residual takes back 7.1 percent. The parts add up to the total exactly (test `decomposition_adds_up`).
- **State table and state panel check** (Task 5; questions 2 and 4).
  - `outputs/state_income_2047.csv` shows the national elasticity applied to each of the 34 states' own income: relative income, convergence and the 2047 requirement share. Its regional sums are within 0.3 points of the Monte Carlo regional medians.
  - `outputs/state_panel_check.csv` is a diagnostic only. It regresses ln(NPP state generation per person) on ln(GSDP per person) with state fixed effects over FY2018-19 to FY2023-24. The slope is 0.434 (s.e. 0.242, 188 state-years, 32 states, Ladakh skipped). NPP records generation where the plant stands, so state generation per person ranges from 0.08 times (Nagaland) to 15.9 times (Sikkim) the national 979 kWh, and 97 percent of its variance lies between states. This is why the elasticity is estimated nationally and applied state by state.
- **New-loads scenario** (Task 6; question 1; ruling R7).
  - `data/external/new_loads.csv` holds published 2050 estimates (secondary). `outputs/new_loads_2047.csv` reports them.
  - Only green hydrogen, the one load absent from the historical series, is added: 418 to 1,237 TWh in 2047, scaled by 24/27.
  - The sensitivity row `new_loads_published` gives a median of 5,537 TWh (+17.7 percent). It is never in the headline.
  - Retired on 21 November 2026 and replaced by the demand-feature layer from official sources (D14, R27).
- **Regional base mix from NPP** (Task 7; question 3; ruling R9). The FY2023-24 base mix (1,734.4 TWh) is built from two parts:
  - NPP regional conventional generation (1,508.5 TWh, primary), with thermal split into coal and gas by NPP's own station sums;
  - CEA General Review Table 4.7 state renewables (225.8 TWh, secondary), which fill the gap NPP leaves.

  It replaces Ember's CY2024 state data (1,801.2 TWh, secondary), which is now a cross-check only. The base year moves to FY2023-24, so the coal floors fall slightly, for example 796.8 to 776.9 TWh at the default 0.60.
- **Published build limits** (Task 8; rulings R10, R11, R12).
  - Each source's 2047 generation is capped at its starting level plus `build_accel` times the published additions (GW per year x CUF x 8.76, summed from FY2024-25). The additions come from CEA NEP 2022-32, the CEA input slides (Dec 2024), NITI Aayog (2026), the DAE nuclear target and the MoP Annual Report hydro schedule, in low, central and high cases.
  - The starting level is the larger of FY2023-24 and the existing fleet's best NPP year from FY2018-19 to FY2025-26. These fleet floors are coal 1,331.9, gas 51.1, nuclear 56.7 and hydro 176.7 TWh.
  - This replaces the record annual increase (Ember) times an acceleration factor. NPP national records remain a fallback and cross-check only.
  - The Robust preset moves from 2.5 times the record to 1.0 times the published high case.
  - The dashboard multiplier now starts at 0.75, and an infeasible plan shows a message instead of numbers.
- **CEA v22 emission factors, CEA cost basis, mix sensitivities** (Task 9; rulings R8, R15, R16, R17).
  - The emission factors are CEA CO2 Baseline Database v22.0 (FY2025-26): coal 0.971 and gas 0.470 tCO2/MWh, against 0.969 and 0.452 in v21.
  - A CEA-basis LCOE is computed from NEP capital costs, the CERC RE tariff order's pre-tax WACC (12.585 percent), CEA and CERC fuel costs and an RBI rate, all secondary.
  - `outputs/sensitivity_mix.csv` runs copies of the Robust preset with one change each: the CEA cost basis, the NISE 2014 solar potential, NIWE 120 m wind, and the low and central build cases. Each case is solved at the median and at the P90 requirement, and the file records the median shortfall as well.
- **Committed builds and gas for peaking** (Task 9a; rulings R13, R18).
  - **Committed nuclear.** Units under construction or sanctioned, plus RAPP-7 (commissioned in FY2025-26), give 14,300 MW. At CUF 0.68 they add 85.2 TWh, so the nuclear floor is 133.1 TWh.
  - **Committed hydro.** Large hydro under construction gives 13,023.5 MW. At CUF 0.36 it adds 41.1 TWh, so the hydro floor is 184.6 TWh.
  - Both floors apply in every policy, because committed capacity is exogenous to a capacity-expansion plan.
  - **Gas for peaking.** The Robust preset keeps the gas fleet at its best NPP year (51.07 TWh, FY2020-21), for peaking and as a bridge for variable renewables.
  - **Regional gas bounds.** Each region's gas is bounded by the gas cap times its FY2023-24 share of gas, in every policy (R18). Without these bounds the LP sited 46 TWh of gas in the Eastern Region, which has almost none today.
  - The hourly check finds that gas runs 2,698 hours a year, up to 21.4 GW, with 49 percent of its energy between 18:00 and 24:00 IST.
- **Landing page** (Task 11b; ruling R14). The page now follows the structure of the official challenge page, with a six-source regional map.
- **This task.**
  - `outputs/site_summary.json` gains:
    - `plan.vectors`, the Robust plan by the six sources and region (`plan_vectors` in `src/allocation/allocate.hpp`), so the map shows plan numbers;
    - `plan.vectors_other` and `plan.committed`;
    - `regions.state_count`;
    - `growth_decomposition`, `state_panel`, `new_loads` and `sensitivity_mix`;
    - the gas peaking figures under `storage`.
  - The `no_urban_term` row now reads "gamma = 0: no urbanisation effect beyond income (UN population path still used)". The run without the urban term still uses the UN population path.
  - `outputs/presets.csv` adds a P05 stress test.
  - The dashboard's data tab adds two views. "Requirement evidence" shows the growth decomposition, the state table, the state panel check and the new loads. "Mix evidence" shows the mix sensitivities, the committed builds and the gas peaking hours.

**Rulings that shape the results** (the ledger of the plan).

| Ruling | Decision |
| --- | --- |
| R7 | The new-loads add-on takes only loads absent from the history, scaled 24/27 to 2047. They are end-use electricity with no loss gross-up, labelled "if supplied from the grid". Partly overlapping loads are reported, never added. |
| R8, R16 | CEA-basis costs use the CERC RE tariff order's WACC, CEA and CERC fuel costs and a dated RBI rate (all secondary). Nuclear and gas keep their IEA cost. |
| R9 | NPP regional thermal is split into coal and gas by NPP's own station sums (primary). The CEA Table 4.4 split is a cross-check (within 0.65 TWh). |
| R10 | Published limits govern every source that has rows. NPP national records are the fallback and a cross-check only. |
| R11 | Existing-fleet floors start each conventional cap at the best NPP year from FY2018-19 to FY2025-26. Generation only; no installed capacity. |
| R12 | The dashboard build multiplier starts at 0.75. An infeasible LP shows a message instead of numbers. |
| R13 | Committed nuclear and hydro builds are floors. The Robust plan keeps the gas fleet for peaking. |
| R14 | The landing page was rebuilt in parallel; this task adds its data keys. |
| R15 | The `build_high` mix sensitivity duplicated Robust, so it was replaced by `build_central`. |
| R17 | `sensitivity_mix.csv` keeps the median shortfall column. Without it, cases that cannot meet the median would look cleaner and cheaper. |
| R18 | Regional gas is bounded by the gas cap times the FY2023-24 regional share, in every policy. |

**Effect on results** (before: the copies in `docs/beamer/data/` committed with 88a6626, the midpoint run; after: `outputs/` of this run; Base scenario unless stated).

| Quantity | Before (midpoint) | After |
| --- | --- | --- |
| FY2047-48 national median (P5 to P95), TWh | 4,703 (3,217 to 7,002) | unchanged (the forecast files are byte-identical) |
| Regional medians: Northern, Western, Southern, Eastern, North-Eastern, TWh | 1,405; 1,226; 1,068; 840; 161 | unchanged |
| Base mix | Ember CY2024, 1,801.2 TWh (secondary) | NPP FY2023-24 conventional 1,508.5 + CEA renewables 225.8 = 1,734.4 TWh |
| Robust plan: non-fossil, coal, gas, nuclear | 78.3%, 21.7%, 0%, 1.2% | 79.5%, 19.4%, 1.1%, 2.8% |
| Robust plan: solar, firmed renewables, hydro, wind (onshore + offshore) | 50.7%, 14.6%, 7.4%, 4.5% | 50.7%, 14.2%, 7.3%, 4.5% |
| Robust plan: CO2; cost | 992.6 Mt; 175.92 bn USD/yr | 911.7 Mt; 177.51 bn USD/yr |
| Robust plan: unmet at the median; at P90 (6,400 TWh) | 0; 638.2 TWh | 0; 632.9 TWh |
| Base policy: non-fossil, CO2, cost | 70.0%, 1,370.6 Mt, 213.31 bn USD/yr | 70.0%, 1,373.5 Mt, 215.29 bn USD/yr |
| Base policy: unmet at the median; at P90 | 0; 1,529 TWh | 0; 1,463 TWh |
| Low carbon policy: non-fossil, CO2 | 80.0%, 823.4 Mt | 81.7%, 832.4 Mt |
| P90 levers (Robust), 35% usable solar land | 2.5x record: 638 TWh unmet; 4x record: 240 TWh | 1.0x published high case: 633 TWh; 2.0x: 219 TWh |
| P90 levers (Robust), 45% usable solar land | 0 unmet at 2.5x record (256.8 bn USD/yr) | 0 unmet at 1.0x the published high case (257.7 bn USD/yr) |
| Cost-CO2 trade-off (Robust): end point | 992.6 to 643.4 Mt (-35.2%) for +0.65 bn USD/yr; highest MAC 6.32 USD/t | 911.7 to 652.7 Mt (-28.4%) for +0.46 bn USD/yr; highest MAC 4.04 USD/t |
| Robust storage (median): power, daily energy, duration | 447 GW, 3,042 GWh, 6.8 h | 448 GW, 3,044 GWh, 6.8 h |
| Robust storage: shortfall left after daily storage | 242 TWh | 226 TWh |
| Robust land: solar and wind footprint | 19,990 km² (0.61% of India) | unchanged |
| P90-levers plan: storage; land | 580 GW, 4,069 GWh; 25,402 km² | 577 GW, 4,051 GWh; 25,402 km² |

How the Robust numbers moved, step by step (median requirement):

| Step | Non-fossil | CO2, Mt | Cost, bn USD/yr | Unmet at P90, TWh | Why |
| --- | --- | --- | --- | --- | --- |
| Midpoint | 78.28% | 992.6 | 175.92 | 638.2 | |
| NPP base mix (Task 7, R9) | 78.04% | 1,003.4 | 175.93 | 641.6 | The nuclear lower bound falls from 54.7 to 47.9 TWh, and hydro's bound falls where today's output exceeds 70% of potential; coal fills the gap |
| Published build limits, high case (Task 8) | 77.70% | 1,018.9 | 175.98 | 648.4 | Hydro and offshore wind become capped by the published additions |
| Existing-fleet floors (Task 8, R11) | 77.77% | 1,015.8 | 175.96 | 632.1 | Caps start from the best NPP year, not FY2023-24 |
| CEA v22 emission factors (Task 9) | 77.77% | 1,017.9 | 175.96 | 632.1 | Coal factor 0.969 to 0.971; the mix is unchanged |
| Committed builds, gas for peaking, regional gas bounds (Task 9a, R13, R18) | 79.54% | 911.7 | 177.51 | 632.9 | 85 TWh of committed nuclear and 51 TWh of gas push coal out at the margin |

The Base policy's non-fossil minimum (70 percent) binds throughout, so its mix hardly moves. Its P90 shortfall falls by 66 TWh, mostly because the conventional caps now start from the best NPP year (Task 8, R11).
Hydro is now limited in every Base and High growth plan by its usable potential (70 percent of identified potential, 346 TWh), not by the published hydro additions.
The Low growth plans need less hydro: 5.4 to 6.5 percent of generation (`outputs/presets.csv`).

Mix sensitivity of the recommended plan (`outputs/sensitivity_mix.csv`):

| Case | Non-fossil | CO2, Mt | Cost, bn USD/yr | Unmet at median / P90, TWh |
| --- | --- | --- | --- | --- |
| Robust as given | 79.5% | 912 | 177.5 | 0 / 633 |
| CEA cost basis (own basis) | 75.0% | 1,118 | 250.3 | 0 / 633 |
| NISE 2014 solar potential | 75.0% | 772 | 181.3 | 1,418 / 3,114 |
| NIWE 120 m wind potential | 75.0% | 1,118 | 183.7 | 0 / 1,044 |
| Low-case build limits | 75.0% | 739 | 152.7 | 1,555 / 3,252 |
| Central-case build limits | 79.4% | 918 | 177.5 | 0 / 703 |

The NISE 2014 and low-case rows cannot meet the median requirement. Their lower CO2 and cost describe less generation, not a better plan.

**Rejected.**

- Ember as the base mix or the build-rate evidence: it is secondary. NPP covers the conventional sources by region, and CEA is used only where NPP has no regional or type split (renewables).
- Freezing existing fleets at FY2023-24 output: a published addition of zero (gas) would otherwise cap the gas fleet below what it generated in FY2020-21.
- A state-level elasticity: the panel check shows that state generation measures plant location, not demand.
- Adding the partly overlapping new loads (vehicles, cooling, data centres, cooking) to the headline: the income elasticity already carries part of them.
- Changing the LP so that low-demand cases stay feasible (for example by letting the coal floor or the gas minimum bend): out of scope before the final evaluation. The limit is reported below instead.

**Known limits.**

- **Infeasible at low demand.** The Robust preset has no feasible plan at the Low growth scenario's P05 requirement (2,332 TWh, `outputs/presets.csv`, columns `unserved_at_p05_twh` and `cost_at_p05_busd`).
  - The coal floor (50 percent of each region's FY2023-24 coal, 647 TWh) and the gas kept for peaking (51 TWh) are fixed in TWh, while the non-fossil minimum (75 percent) is a share of generation.
  - Below about (647 + 51) / 0.25 = 2,794 TWh of generation, the two floors exceed the fossil allowance. The lowest feasible requirement found by bisection is 2,780 TWh (test `robust_low_requirement_limit`).
  - The Base policy (coal floor 60 percent, bound about 2,590 TWh) and the Energy security policy (70 percent, about 3,020 TWh) are also infeasible at that requirement. Low carbon (35 percent) is feasible.
  - Every policy is feasible at the Base scenario's P05 (3,217 TWh) and at every scenario's median.
  - By the same arithmetic the midpoint model had this limit too (coal floor 664 TWh / 0.25, about 2,660 TWh); it was not tested then. The gas minimum adds about 200 TWh to the bound.
  - The dashboard explains the infeasible state and names the levers: the coal floor, the gas kept for peaking and the non-fossil minimum.
- High growth with the Energy security policy is infeasible at P90, as at the midpoint.
- The growth decomposition splits the central path (4,600 TWh), not the median (4,703 TWh), and the central path ignores the `beta_shift` and `beta_window` controls.
- The state panel check uses a conventional standard error that is not clustered by state, and it covers conventional stations only. Ladakh is skipped, not merged into another state.
- The new-loads add-on is drawn uniformly between its low and high values, so the scenario's median rises by about the midpoint (+834 TWh). Green hydrogen may be supplied off-grid, outside NPP utility generation.
- The base mix total (1,734 TWh) exceeds NPP's national FY2023-24 total (1,698 TWh), because CEA's renewables are 36 TWh above NPP's renewable total. NPP gives no regional or type split of renewables.
- The existing-fleet floors are best-year maxima: hydro in the wet FY2025-26, gas in FY2020-21.
- FY2024-25 and FY2025-26 have no rows in the build-limit file, so they take the first period's rate.
- Published coal additions are gross of retirements. The 1.10 times base national coal ceiling binds before them.
- The CEA cost basis applies nominal CERC rates to real 2021-22 NEP capital costs, at the FY2021-22 rate. Its cost level is not comparable with the IEA basis; it shows only the direction of the mix. Resolved on 21 November 2026: the CEA basis is now in real 2024 USD (D14).
- The committed floors ignore commissioning dates, and the hydro CUF (0.36) is the lowest published value.
- At 100 percent gas kept for peaking, the gas minimum equals the gas cap, so the regional gas bounds fix where gas runs. The Western Region has the largest share (38 percent), not a majority.
- A gas maximum below about 1.1 percent with a non-zero gas minimum makes the LP infeasible. The dashboard says so.

**Checks.**

- The suite still has seven tests, all passing. New checks were added inside them:
  - `test_india`: `elasticity_shift_monotone`, `decomposition_adds_up`.
  - `test_regions`: `fe_slope_recovers_known`, `fe_slope_skips_unmatched`, `state_table_shares_sum`, `new_loads_excludes_overlap`.
  - `test_allocation`, from earlier tasks: `base_mix_matches_npp`, the published-limit and fleet-floor tests, `emission_factor_latest_period`, `cea_lcoe_formula`, `sensitivity_from_copy`, the committed-floor and gas tests, `presets_feasible`.
  - `test_allocation`, added in this task: `plan_vectors_add_up` (regions sum to each source's total, regional shares and plan shares to 100 percent) and `robust_low_requirement_limit`.
- The forecast outputs are byte-identical to the midpoint run: the national P50 and the regional medians are unchanged.

## D14. Inflation and the full set of demand drivers (2026-11-21)

**Trigger.** After the final presentation (21 November 2026) the team set two goals. The design is `docs/plans/2026-11-21-inflation-features-design.md`.

1. **Inflation.** Every money figure must sit on one stated real price base, converted with official price indices.
2. **Demand drivers.** Every factor that can change India's 2047 generation requirement must be listed, and quantified wherever an official source allows. The scenario set must span all of them.

The rules do not change:

- The headline is estimated on the two challenge datasets. NPP and the Renewable India dataset stay the only primary sources.
- Every new figure comes from an official source (a ministry, a statutory body, NITI Aayog, a central bank or a national statistics office) and is labelled secondary.
- Installed capacity and consumption are never forecast metrics. A capacity or end-use figure appears only after an explicit conversion to TWh of generation, as the size of a feature.
- Generation figures appear only in comparisons; consumption-side figures are context.

**Why.**

- **Mixed price bases in the CEA cost basis.** The CEA basis applied the nominal CERC WACC (12.585 percent) to real 2021-22 NEP capital costs. It priced fuel in 2021-22 and FY2026-27 rupees and converted at the FY2021-22 exchange rate. D13 listed this as a known limit: its cost level could not be compared with the IEA basis.
- **One state's income at current prices.** A check of the organiser GSDP against MoSPI found that every state is at constant 2011-12 prices from 2011 except Jammu and Kashmir. Its values from 2012 equal MoSPI current prices (2016: 124,848 crore, against 100,203 at constant prices). Its inflation was counted as real income growth, both in the national income series and in the relative incomes of the regional split.
- **Placeholder base years in the convergence estimate.** Once Jammu and Kashmir was deflated, its 2006 relative income still rested on the 2006-2010 placeholder run (D11). Telangana, Andaman and Nicobar Islands and Lakshadweep had the same problem. The old convergence speed was driven by these four placeholder states (2006 relative incomes of +1.40, +1.16, +0.93 and +0.50 log points for Telangana, Andaman and Nicobar Islands, Jammu and Kashmir and Lakshadweep).
- **Demand drivers beyond income.** The midpoint panel asked whether income was the only factor (D13, question 1). The answer listed four drivers and one new-loads scenario that added green hydrogen only, with a central value from a think tank (TERI); the other loads were reported, not added, and several of their figures were not official. A full catalogue from official sources only was missing.

**Choice.** What changed (commits 8c4dd8a to 62c347f; ruling R30 in 6f19e6e):

- **One real price base: real 2024 USD**, the price year of IEA WEO 2025 (`src/core/price_base.hpp`).
  - The official indices are in `data/external/macro/price_indices.csv` (secondary): the MoSPI GDP deflator (2011-12 series), the OEA WPI, the MoSPI CPI, the RBI rupee-dollar reference rates, the RBI inflation target (4 percent, band 2 to 6 percent, notified to FY2030-31), the BEA GDP deflator, the BLS CPI-U and the FOMC longer-run goal (2 percent).
  - A cost given in a currency and price year is first deflated within its own currency with the GDP deflator, then converted at the RBI calendar-2024 rate (Rs 83.6693 per USD). A year after the last published index is extended at the target (India 4 percent, United States 2 percent) and labelled as not official.
  - **The CEA cost basis is fully real.** NEP capital cost, fixed O&M and CEA coal fuel (2021-22 prices) are multiplied by 1.11991, the MoSPI deflator of FY2024-25 over FY2021-22. CERC biomass fuel (FY2026-27 prices) is multiplied by 0.95628. Both are then divided by 83.6693. The CERC WACC is made real with the Fisher relation at the RBI target: (1.12585 / 1.04) - 1 = 8.255 percent. Two new mix-sensitivity rows take the RBI band: 2 percent inflation (real WACC 10.377 percent) and 6 percent (6.212 percent). The default inflation is read from `price_indices.csv`.
  - **The default IEA plan does not change.** Every IEA cost is already in USD(2024)/MWh. Hydro, bioenergy and the firmed-RE premium are rupee tariff ratios, which need no conversion. With the price base alone, every output of the default plan was byte-identical.
  - **Nominal 2047 outlook**, for information only (`site_summary.json`, `plan.nominal_outlook`). The Robust plan's cost, 176.16 bn USD a year in real 2024 USD, is Rs 36.33 lakh crore a year in 2047 rupees at the RBI target, or 277.8 bn USD a year in 2047 dollars at the FOMC goal. Every headline cost stays real.
- **Jammu and Kashmir GSDP deflated** (R21; `data/external/macro/gsdp_price_basis.csv`).
  - Its values from 2012 are divided by the MoSPI national GDP deflator of their fiscal year: 13 state-years in all.
  - Every reader of the dataset's GSDP must pass the price basis: national income growth, the relative incomes and convergence speed of the regional split, and the state panel check.
  - The GSDP values stay the dataset's own; only their price basis is made consistent. This is an inflation correction of a mis-based series, not new estimation data.
  - A sensitivity row, `jk_excluded`, leaves Jammu and Kashmir out of national income growth instead.
- **Convergence base year** (R25, R28).
  - Each state's long difference starts at its first year from 2006 whose GSDP, as read, equals neither the year before nor the year after.
  - Jammu and Kashmir, Telangana, Andaman and Nicobar Islands and Lakshadweep start in 2011, and Nagaland in 2008. Delhi (2009) and Chandigarh (2013) start in their first years. All other states start in 2006. So 34 states enter instead of 32, 7 of them with a later base year.
  - lambda = -ln(1 + slope x S) / S, where S is the mean span (17.06 years).
  - lambda falls from 0.0208 to 0.0021 (s.e. 0.0051): convergence is essentially absent. The national forecast does not depend on it.
- **Demand-feature layer** (R22, R23, R24; `src/pipeline/demand_features.hpp`; the catalogue is [demand_features.md](demand_features.md)).
  - `data/external/demand_features.csv` lists 49 drivers (44 before R30); every quantified size comes from an official source, and six unquantified rows have no external source (three where no official figure was found, three that point to the model's own runs): 7 estimated in the core forecast, 3 existing drivers or settings, 11 quantified feature rows (the layer), 1 excluded by rule (captive generation) and 27 context rows (22 before R30).
  - The 11 layer rows are signed TWh adjustments to FY2047-48 utility generation. They are already reduced to their part beyond the historical trend and grossed up to generation.
  - Seven named scenarios: Core, Features central, Electrification high, Efficiency and distributed, Climate stress, All upside and All downside. A joint Monte Carlo adds independent feature draws to each core draw. A P90 plan test solves the Robust plan at the joint P50 and P90.
  - Each adjustment is ramped linearly from the base year, like the existing add-on. Feature draws use their own Philox stream, so the headline draws are bit-identical.
  - New outputs: `outputs/demand_features_catalogue.csv`, `outputs/feature_scenarios_2047.csv` and `features.*` in `site_summary.json`. The dashboard gets a "Demand features (official, outside headline)" selector and a catalogue view.
  - The headline stays the two-dataset forecast (4,688 TWh). Every feature scenario is labelled as outside the headline.
- **AI and digital loads** (R30, commit 6f19e6e, later on 21 Nov).
  - The data-centre high takes the Ministry of Power's 26.3 GW of AI data-centre load by 2031-32 (Rajya Sabha Unstarred Question 956, answered 27.07.2026), read as average load (x 8.76 = 230.4 TWh) and continued to FY2047-48: 697.8 TWh beyond the trend, from 397.1. Low and central stay NITI Aayog's.
  - Five context rows are added with their official sources and not quantified: AI compute (GPUs), telecom networks, consumer electronics and devices, semiconductor and electronics manufacturing, and digital public infrastructure.
  - The layer stays 11 rows and the headline does not move. Electrification high, All upside, the one-at-a-time data-centre row, the joint distribution and the P90 plan test change (tables below).
- **Retired.**
  - The new-loads scenario (`new_loads_published`: green hydrogen only, 418 to 1,237 TWh, with a think-tank central value) and its code (R27). `data/external/new_loads.csv` is kept as an archive and is no longer read.
  - The High growth preset's fixed 100 to 300 TWh add-on, which had no official source (R26).

**Rulings** (the ledger of this work).

| Ruling | Decision | What it costs if wrong |
| --- | --- | --- |
| R21 | Jammu and Kashmir GSDP from 2012 (current prices) is deflated with the MoSPI national GDP deflator, with `jk_excluded` as the sensitivity. It is an inflation correction of a mis-based series | A small headline shift, disclosed: `jk_excluded` gives 4,683 TWh against 4,688 (-0.1 percent). A Jammu and Kashmir deflator would differ from the national one: the deflated values lie -2.0 to +4.2 percent from MoSPI's constant-price values over 2012-2021 |
| R22 | Layer rows apply as the CSV gives them (already cut to the part beyond the trend), except the one row that overlaps the history, T&D losses: 0 at central, uniform(0, high) in the joint draws, and its high value only in the downside scenarios | If T&D losses are in fact beyond the trend, Features central overstates the requirement by up to about 465 TWh (the listed central value, 464.6) |
| R23 | Features are split across regions in proportion to each region's share of the requirement; where a feature sits is context only | Regional feature effects are approximate. No official regional split exists, and data centres, hydrogen and rooftop solar need not be spread like the requirement |
| R24 | Seven named scenarios, a joint Monte Carlo (core draws plus independent triangular feature draws) and a P90 plan test on core plus features | None: computation only |
| R25 | Each state's convergence base year is its first non-repeated GSDP year, matching the existing repeat-skip rule | The regional split moved without cause: lambda went from 0.0249 (after R21) to 0.0021, and the regional medians moved by -125 (Eastern) to +137 TWh (Southern) |
| R26 | The High growth preset's unofficial 100 to 300 TWh add-on is removed | The High growth rows drop by about 200 TWh (P50 6,780 to 6,580) |
| R27 | The landing page reads `features.*` instead of the new-loads row, and the new-loads code is retired | None |
| R28 | The base year is the first year that differs from both neighbours, not the literal "differs from the next year", which still lands on the Jammu and Kashmir 2010 placeholder | Under the literal reading lambda would be 0.0166 instead of 0.0021, and the regional shift would be smaller |
| R29 | Revised later on 21 Nov: the final deck (pptx and PDF), the Beamer decks and the Q&A notes are updated to the current run, each with a dated line, and the landing page says what changed after the evaluation; the talk script and the midpoint slide keep the presented numbers | Jurors comparing the site with the deck they saw find different regional numbers; the dated lines and the landing note explain why |
| R30 | Added later on 21 Nov (6f19e6e): AI and digital loads. The data-centre high takes the Ministry of Power's 26.3 GW of AI data-centre load by 2031-32 (Rajya Sabha USQ 956, 27.07.2026), read as average load by NITI Aayog's flat-load convention (x 8.76 = 230.4 TWh), and continues the FY2023-24 to FY2031-32 path linearly to FY2047-48, as rooftop solar's high does: 666.6 TWh, still below NITI's 700 TWh for 2070. Less the trend and x1.1029 this is 697.8 TWh (397.1 before); low 356.2 and central 397.1 stay NITI Aayog's. Five rows are listed as context, not quantified: AI compute (GPUs) and digital public infrastructure sit inside data centres and would double count; telecom networks and consumer electronics are inside the trend, and no official projection sizes growth beyond it (telecom is about 22 TWh a year today at TRAI's 2011 load per tower, for scale only); semiconductor manufacturing has no official MW or TWh. Industry, press and think-tank figures are not used | If the 26.3 GW is sanctioned capacity at NITI's 50 percent utilisation, the continued path gives 316.6 TWh, below the central, so the high stays 397.1 and every feature result returns to its value before R30 (joint P50 5,668 instead of 5,769; Electrification high 6,528 instead of 6,828). CEA's 17 GW (via MeitY; 148.9 TWh as average load) on the same path gives a milder high of 428.2 TWh, 269.6 lower. If the 26.3 GW is in addition to CEA's 17 GW, the high is too low. The continuation runs an 8-year official path 16 more years |

**Effect on results** (before: the 20 Nov run, as presented on 21 Nov; after: `outputs/` of commit 62c347f, and of 6f19e6e for the feature rows that R30 moves; GPU, 4,194,304 draws, default seed).

| Quantity | 20 Nov, as presented | Now |
| --- | --- | --- |
| FY2047-48 national P5 / P50 / P95, TWh | 3,217 / 4,703 / 7,002 | 3,190 / 4,688 / 7,021 |
| National P90 (the plan's stress test), TWh | 6,400 | 6,408 |
| Increase over FY2023-24 (1,698 TWh) | +3,005 TWh (2.77 times) | +2,990 TWh (2.76 times) |
| Elasticity beta, FY2011-12 window (s.e.) | 0.828 (0.063) | 0.835 (0.065) |
| Elasticity beta, FY2006-07 window (s.e.) | 0.866 (0.031) | 0.872 (0.032) |
| Elasticity used: bootstrap median (90 percent interval) | 0.850 (0.731 to 0.939) | 0.857 (0.737 to 0.947) |
| Mean growth of income per person (organiser data) | 4.95 percent a year | 4.90 percent a year |
| Convergence lambda (s.e.; states) | 0.0208 (0.0106; 32) | 0.0021 (0.0051; 34) |
| Regional medians: Northern / Western / Southern / Eastern / North-Eastern, TWh | 1,405 / 1,226 / 1,068 / 840 / 161 | 1,342 / 1,276 / 1,188 / 730 / 151 |
| Regional shares of the requirement | 29.9 / 26.1 / 22.7 / 17.9 / 3.4% | 28.6 / 27.2 / 25.3 / 15.6 / 3.2% |
| Robust plan: non-fossil; coal | 79.5%; 19.4% | 79.9%; 19.0% |
| Robust plan: CO2 | 911.7 Mt | 891.3 Mt |
| Robust plan: cost at the median (real 2024 USD, IEA basis) | 177.51 bn USD/yr | 176.16 bn USD/yr |
| Robust plan: unmet at the median; at P90 | 0; 632.9 TWh | 0; 639.5 TWh |
| Base policy: CO2; cost; unmet at P90 | 1,373.5 Mt; 215.29 bn USD/yr; 1,463 TWh | 1,367.2 Mt; 213.05 bn USD/yr; 1,471 TWh |
| CEA cost basis (copy of the Robust plan) | 250.29 bn USD/yr and 1,118.3 Mt, on a mixed price base that could not be compared with IEA | 202.58 bn USD/yr in real 2024 USD at 4 percent inflation (225.22 at 2 percent, 182.13 at 6 percent); 1,113.8 Mt; non-fossil 75.0% in all three |
| Nominal 2047 outlook (information only) | none | Rs 36.33 lakh crore or 277.8 bn USD a year |
| Scenarios outside the headline, FY2047-48 P50 | New loads (green hydrogen only): 5,537 TWh (+17.7%) | Features central 5,810; Electrification high 6,828; Efficiency and distributed 5,076; Climate stress 6,063; All upside 6,855; All downside 4,865 TWh. Before R30: Electrification high 6,528, All upside 6,555 |
| Joint distribution (core plus features) | none | P5 4,225 / P50 5,769 (+23.1%) / P95 8,121 TWh. Before R30: 4,130 / 5,668 (+20.9%) / 8,017 |
| Robust plan: unmet at the joint P50; at the joint P90 | none | 8.9 TWh at 5,769; 1,738 TWh at 7,506. Before R30: 0 at 5,668; 1,635 TWh at 7,404 |
| High growth preset, P50 | 6,785 TWh (with the 100 to 300 TWh add-on) | 6,580 TWh |
| Central path; its log growth from FY2023-24 | 4,600 TWh; 0.997 | 4,589 TWh; 0.994 |
| State panel check: slope (s.e.) | 0.434 (0.242) | 0.522 (0.258) |
| Cost-CO2 trade-off of the Robust plan | 911.7 to 652.7 Mt for +0.46 bn USD/yr | 891.3 to 652.7 Mt for +0.45 bn USD/yr |
| Robust storage at the median: power; daily energy | 448 GW; 3,044 GWh | 448 GW; 3,048 GWh |
| Tests | 7 | 9 |

How the numbers moved, step by step:

| Step (commit) | National P50 (P5 to P95), TWh | lambda | Regional medians N / W / S / E / NE, TWh | Robust plan: non-fossil, CO2, cost, unmet at P90 |
| --- | --- | --- | --- | --- |
| 20 Nov, as presented | 4,703 (3,217 to 7,002) | 0.0208 | 1,405 / 1,226 / 1,068 / 840 / 161 | 79.54%, 911.7 Mt, 177.51 bn USD, 632.9 TWh |
| Price base (8c4dd8a) | unchanged | unchanged | unchanged | unchanged; only the CEA rows move (250.29 to 203.44 bn USD) |
| Jammu and Kashmir deflated (5f7857b) | 4,688 (3,190 to 7,021) | 0.0249 | 1,399 / 1,217 / 1,051 / 854 / 162 | 79.78%, 897.7 Mt, 176.57 bn USD, 641.3 TWh |
| Feature layer (c957a6e, 0aded2d, 62c0916) | unchanged | unchanged | unchanged | unchanged (every output byte-identical except `timings.csv`) |
| Convergence base year, R25 (c41d93d) | unchanged | 0.0021 | 1,342 / 1,276 / 1,188 / 730 / 151 | 79.90%, 891.3 Mt, 176.16 bn USD, 639.5 TWh |
| R26 and R27 (00e873d, 62c347f) | unchanged | unchanged | unchanged | unchanged; the High growth P50 falls from 6,780 to 6,580 TWh |
| AI and digital loads, R30 (6f19e6e) | unchanged | unchanged | unchanged | unchanged at the median and P90; data-centre high 397.1 to 697.8 TWh: Electrification high 6,528 to 6,828, All upside 6,555 to 6,855, joint P50 5,668 to 5,769 TWh; unmet at the joint P50 0 to 8.9 TWh, at the joint P90 1,635 to 1,738 TWh |

- The national median falls by 15 TWh (-0.3 percent) because the deflation removes Jammu and Kashmir's inflated growth: mean income growth per person falls from 4.95 to 4.90 percent a year. The elasticity rises (0.828 to 0.835), which partly offsets it.
- The regional split changes a lot. Under the old lambda, poorer states' relative incomes rose towards the national average by 2047; now they barely move. The Southern and Western regions gain; the Eastern, Northern and North-Eastern regions lose.
- The Robust plan moves only through the regional requirements: a smaller Eastern requirement means less import over the West-East corridor.

**What stays the same.** The holdout errors (-3.3 and +0.5 percent, both inside the 90 percent range), the backtest errors (2.0 percent one year ahead, 6.4 percent five years ahead), the cross-country check (4,528 TWh), the FY2023-24 base mix, the build limits, the committed floors, the emission factors, the IEA costs, and the end point of the cost-CO2 trade-off (652.7 Mt, highest marginal abatement cost 4.04 USD/t).

**Rejected.**

- A Jammu and Kashmir state deflator: it is not among the official indices fetched, and the national deflator is the official index (R21).
- Dropping Jammu and Kashmir from the estimation: it would remove a state from a primary source. It is kept as the `jk_excluded` sensitivity instead.
- Adding any feature to the headline: the headline must stay estimated on the two challenge datasets.
- Figures from TERI, CEEW, BloombergNEF and the IEA as feature sizes: they are not official Indian sources. Official figures replace them where one exists, and electric cooking, which has none, is a context row.
- Correlated feature draws: no official source gives the correlations, so the draws are independent and say so.
- The literal reading of R25 (R28).
- Keeping the presented numbers in the decks: the deck, the site and the repository would disagree (R29).
- AI and data-centre figures that are not official or cannot be converted, as sizes (R30): the Economic Survey 2025-26's "8 GW by 2030", which it attributes to industry; a PIB data-centre capacity figure of 8 GW with no utilisation; CEA figures quoted only in the press; S&P, JLL, Nasscom and similar industry estimates.
- Adding the Ministry of Power's 26.3 GW (230.4 TWh as average load) on top of CEA's 17 GW (148.9 TWh): the reply does not say the two are additive, so the 26.3 GW is read as the whole data-centre level in 2031-32 (R30).
- A TWh size for telecom from TRAI's 2011 load per tower: the figure is fifteen years old, part of tower energy is diesel or on-site renewables, and no official projection separates growth beyond the trend (R30).

**Known limits.**

- **Placeholder end years.** Andaman and Nicobar Islands and Lakshadweep repeat their 2023 GSDP in 2024, so their 2024 relative income, the end point of the convergence estimate and the start of the split, rests on a placeholder. Only the base year was in scope. In the convergence estimate, the national GSDP per person that relative incomes are measured against still includes the 2006-2010 placeholder values of those years.
- **Convergence is essentially absent** (lambda 0.0021, s.e. 0.0051). It depends on how placeholders are read: the literal R25 reading gives 0.0166, and leaving out Delhi and Chandigarh, which the new rule brings in, gives -0.0005.
- **Jammu and Kashmir 2022-2024** are predicted values (the upstream README says some values were predicted), treated as current prices. After deflation they differ from MoSPI constant prices by +7.9, +5.8 and -3.5 percent. The price basis of its 2006-2010 placeholder value was not checked.
- **Overlap caveats in the feature rows.**
  - T&D losses are an upper bound: the 2006-07 to 2023-24 fall from 28.65 to 17.63 percent is inside the estimated elasticity.
  - New loads are grossed up at FY2023-24 loss rates, so with T&D losses active they are overstated by up to about 10 percent.
  - Storage round-trip losses would be counted twice if the Part 2 storage back-calculation, which reports its own losses (136 TWh in the Robust plan), were added on top.
  - Distributed renewables under the renewable consumption obligation overlap rooftop solar and PM-KUSUM.
  - AI compute (GPUs) and digital public infrastructure are inside data centres, and telecom networks and consumer electronics are inside the trend; none of them is added (R30).
  - Rows are summed independently, exactly as the CSV gives them.
- **NPP scope of feeder-level solar.** PM-KUSUM C feeder-level plants feed 11 kV feeders. If CEA counts them as utility generation, their effect on NPP generation is zero. Whether Component A plants are inside NPP's series is not stated in the sources, and rooftop solar assumes NPP excludes rooftop output.
- **Conversion factors.**
  - The layer's unit factors are official but fixed: 1 Mtoe = 11.63 TWh; 50 kWh per kg of hydrogen with a 50 percent grid share; rooftop CUF 17 percent with 75 percent self-consumed; 7.5 HP pumps at 17 percent CUF; hydro import PLF 32 to 40 percent; storage round trip 88 percent (batteries) and 80 percent (pumped storage) at one cycle a day.
  - 2050 values are scaled linearly from the base year (24/27), and values for earlier years are held flat to 2047.
  - The trend rule carries each existing load x2.7697 to FY2047-48, the multiple of the 20 Nov headline (4,703 TWh). On the current headline it is x2.7607. The layer values were not rescaled; the difference is under 2 TWh for any row.
- **Tariff ratios across years.** Hydro (2020), new-coal bids (2025), biomass (FY2026-27), round-the-clock bids (2025-26) and solar (2025) are used as ratios to coal, which are treated as inflation-neutral. A ratio across different years is not strictly inflation-neutral.
- **Price base proxies.** FY2024-25 stands for 2024, because MoSPI publishes the deflator by fiscal year only, and it is paired with the calendar-2024 exchange rate. The FY2026-27 deflator for biomass is extended at the RBI target and is not an official figure. The CEA band rows (2 and 6 percent) are fixed in the code; only the default reads `price_indices.csv`.
- **Nominal outlook.** The RBI target is notified only to FY2030-31; the outlook carries it to 2047. The FOMC goal is stated for the PCE price index, not the GDP deflator.
- **The data-centre high is an extrapolation** (R30). The Ministry of Power's 2031-32 figure is continued linearly for 16 more years and read as average load (26.3 GW x 8.76 = 230.4 TWh); the reply does not say whether it is load or capacity. No official captive share exists for data centres, so none is removed.
- **Every feature scenario raises the requirement.** Even All downside adds 177 TWh, because transport (at least 512 TWh), data centres (at least 356) and hydrogen (at least 138) outweigh every row that lowers generation.
- **The talk script and the midpoint slide keep the presented numbers** (R29); the decks and Q&A notes match the outputs.

**Checks.**

- The suite has nine tests, all passing (ctest 9 of 9 at 62c347f).
  - New `price_base`: the Fisher relation, deflation, the rupee-dollar conversion, extension beyond the data at the target, the nominal outlook, the Jammu and Kashmir deflation (within 1 percent of MoSPI's 2016 constant-price value; -0.37 percent observed), every GSDP reader applying the basis on synthetic data, and the default plan unchanged by the price base (cost and CO2 reproduced to 1e-6).
  - New `features`: catalogue validation, the scenario totals, the T&D treatment, the joint quantiles within the All downside to All upside envelope, and the headline unchanged with the layer at Core. R30 changed its expected counts to 49 rows (27 context).
  - `mc_parity` checks the feature draws on the CPU and the GPU; `regions` adds `convergence_base_skips_repeats`; `allocation` checks the CEA tables (2 percent above 4 percent above 6 percent) and the inflation default.
- With the price base alone, every default-plan output was byte-identical. With the feature layer, every output except `timings.csv` was byte-identical. R25 left the national forecast rows byte-identical.

## D15. Official conferences and the Nuclear Energy Mission (2026-11-22)

**Why.**

- **India's own nuclear target was never tested in the plan.** The Union Budget 2025-26 (paras 61-62) commits to "at least 100 GW of nuclear energy by 2047", and CEA's road map to that goal (released on 11 October 2025) lists the units. The Robust plan keeps only the committed reactors, 133.1 TWh (2.8 percent of generation), about a quarter of the mission's utility reactors. The Base and Energy security policies cap nuclear at 10 percent of the requirement, 468.8 TWh at the median, below the mission. Nothing in the model asked what building the announced fleet would do to cost and CO2.
- **The catalogue was built from ministry documents, not from the conferences where targets are set.** The D14 catalogue drew on reports and replies in Parliament. Ministers announce and reaffirm targets at conferences (the Ministry of Power's conferences with State ministers, the Bharat Electricity Summit, India Energy Week, COP and G20 meetings), so a review of the official outcome texts from 2023 to 2026 checked whether any factor was missing.
- **CEA has now published 2047 estimates.** The road map is the only official document found with CEA estimates dated 2047 for both the requirement and the mix: an energy requirement for 2046-47, a capacity mix and a non-fossil share of generation. The Q&A notes still said that CEA had published no 2047 figure.

The rules do not change. The headline is estimated on the two challenge datasets; every new figure is official and labelled secondary; installed capacity and consumption are never forecast metrics, and a GW figure appears only with its explicit conversion to TWh; requirement (consumption-side) figures are context.

**What changed** (commit 8800881).

- **A fifth policy preset, "Nuclear mission"** (`src/pipeline/presets.hpp`; `PolicySettings::nuclear_mission` and `nuclear_floor_twh` in `src/allocation/allocate.hpp`).
  - It is the Robust policy plus one LP row: national nuclear generation at least a floor. The LP sites the floor within each region's nuclear bound (the siting key of reactor units).
  - The floor is the mission's utility reactors in 2047 from CEA's road map, Table 6.3 (100,648 MW), less the captive Bharat Small Reactors (5,060 MW) and BSMR units (5,000 MW) that the organisers' ruling excludes: 90.588 GW x CUF 0.68 x 8.76 = 539.6 TWh (R31, R32).
  - Low case: NPCIL's 54 GW (DAE, Lok Sabha USQ 2783, 17.12.2025) x 0.68 x 8.76 = 321.7 TWh. High case: the 90.588 GW at the road map's achievable PLF of 0.80 = 634.8 TWh, above the national nuclear build cap of 627.1 TWh, so it is reported as infeasible (R33).
  - The inputs are `data/external/techno_economic/nuclear_mission.csv` (secondary): one row per case with its MW, the captive MW left out, the CUF, the source and its page. The CEA URL returned 404 on 22 Nov 2026, and the Internet Archive copy was read.
  - The Robust policy's 15 percent nuclear share limit never cuts the floor; the build cap does (R34).
  - `outputs/presets.csv` now holds 3 growth scenarios x 5 policies. `outputs/sensitivity_mix.csv` gains `nuclear_mission_low` and `nuclear_mission_high`: 10 cases, 20 solves. `site_summary.json` gains `plan.nuclear_mission`. The dashboard's policy buttons wrap to fit five presets.
  - Apart from the new rows and the feature outputs below, every output is byte-identical to the previous run, except `timings.csv`: the default plans do not change.
- **Demand features: 73 rows** (`data/external/demand_features.csv`; `data/external/SOURCES.md`, section 11; [demand_features.md](demand_features.md)).
  - 24 new rows: 23 context rows from official conferences, missions and texts, and the captive small reactors (`captive_nuclear_bsr_bsmr`), excluded by the organisers' ruling. Counts: core 7, driver 3, layer 11, excluded by rule 2, context 50 (R36).
  - Two layer highs are raised from Indian official sources (R35): storage round-trip losses 150.1 to 173.6 TWh (CEA's 2047 storage in the road map) and cross-border imports 15.3 to 49.3 TWh (the Prime Minister's target of importing 10,000 MW from Nepal).
  - Three rows gain revised text (`national_efficiency_targets`, `cross_border_exports`, `data_centres`), and 15 rows gain conference citations with no value change. The data-centre row now says plainly that its high (697.8 TWh) is our extrapolation of an official 2031-32 figure (the Ministry of Power's 26.3 GW), not an official 2047 figure.
  - The layer stays 11 rows, and the headline does not move.
- **`data/external/policy_targets.csv`** gains the CEA road map's 2047 estimates (13 rows): the 2046-47 and 2049-50 requirement and peak as context (R37), the non-fossil share of generation ("about 2/3rd"), and the capacity mix with each source's TWh conversion. It also gains CEA's National Generation Adequacy Plan (NGAP) total, 3,596 BU of gross generation in FY2035-36, which the Q&A notes had attributed to press reports.
- **Docs.** "CEA has published no 2047 figure" is replaced: CEA's road map gives a 2046-47 energy requirement of 4,582 BU, which is consumption-side and so not compared as a benchmark, and a 2047 capacity mix; there is still no CEA 2047 generation total.

**The Nuclear mission is a policy option, not the recommendation.** Robust stays the recommended plan, because the numbers do not argue otherwise:

- **The trade.** At the median requirement the Nuclear mission plan costs 1.49 bn USD a year more than Robust (177.65 against 176.16, real 2024 USD) for 238.7 Mt less CO2 (652.7 against 891.3 Mt): about 6.2 USD per tonne on average.
- **A CO2 cap gets the same cut for less.** The cost-CO2 trade-off of the Robust plan (`outputs/pareto_2047.csv`) reaches the same 652.7 Mt, and the same 85.1 percent non-fossil, for 0.45 bn USD a year (highest marginal abatement cost 4.04 USD per tonne). The mission therefore costs 1.04 bn USD a year more than the least-cost route to the same emissions. Both stop at 652.7 Mt because coal reaches its floor (647.4 TWh).
- **Nothing at P90.** Planned for the P90 requirement, Robust already runs nuclear at the build cap (627.1 TWh), so both policies leave 639.5 TWh unmet at 252.96 bn USD a year.
- **Most costly where demand is lowest.** In the Low growth scenario the floor costs 9.06 bn USD a year more (133.76 against 124.70) and cuts no CO2 (652.7 Mt in both, because coal is already at its floor). In the High growth scenario the floor does not bind, and the two plans are the same.
- What the annual least-cost plan does not price is the mission's case: firm output around the clock and fuel diversity. The storage the mission plan needs was not back-calculated (limits below).

**Rulings** (R31 onwards).

| Ruling | Decision | What it costs if wrong |
| --- | --- | --- |
| R31 | Captive reactors are left out of the floor. The road map's 100,648 MW includes 5,060 MW of Bharat Small Reactors and 5,000 MW of BSMR "for captive uses" (Table 5.3 note; DAE, Lok Sabha USQ 2783, answers (c) and (d)); captive generation is outside NPP utility generation by the organisers' ruling. The units are listed as the excluded row `captive_nuclear_bsr_bsmr` | If the small reactors sell to the grid, the floor is 100.648 GW x 0.68 x 8.76 = 599.5 TWh, 59.9 TWh more and still under the 627.1 TWh build cap; that case was not solved. If they replace industries' grid purchases, NPP generation falls by 59.9 to 70.5 TWh (CUF 0.68 to 0.80), which no scenario includes |
| R32 | The floor's CUF is 0.68, CEA NEP 2022-32's nuclear availability (Annexure 5.1), the same as the nuclear build limits and the committed-build floor | At the normative PLF of new PHWRs (72.5 percent) the floor would be 575.3 TWh; at the fleet's PLF of about 79 percent in 2024-25, 626.9 TWh, just under the cap; at the road map's achievable 80 percent, 634.8 TWh (R33). The central case can understate the mission's energy by up to 95.2 TWh. Only 0.68 and 0.80 were solved |
| R33 | The high case (634.8 TWh) is reported as infeasible against the national nuclear build cap (627.1 TWh: the starting level plus the published high-case additions at CUF 0.68), not cut to fit | The cap credits each published GW at 0.68, while the high case runs the same reactors at 0.80. If the fleet does reach 0.80, the cap understates what it can generate, and a feasible plan 95.2 TWh above the central case goes unreported. The gap is only 7.7 TWh |
| R34 | The floor overrides the nuclear share limit: where the floor is above nuclear_max x the requirement (a low requirement), the floor is the limit. The build cap is never overridden | In the Low growth scenario (3,377 TWh) 15 percent is 506.6 TWh, below the floor. With the ruling, nuclear is 15.9 percent of generation and the plan costs 9.06 bn USD a year more than Robust for no CO2 cut. If the share limit should bind, that plan is either infeasible or holds 33 TWh less nuclear |
| R35 | Two layer highs are raised from Indian official sources: storage round-trip losses to 173.6 TWh (the road map's 2047 storage, 702.9 GWh of pumped storage and 1,983.9 GWh of energy storage read as batteries, one cycle a day) and cross-border imports to 49.3 TWh (Bhutan's 4,956 MW from the NEP plus the Prime Minister's 10,000 MW Nepal target, at PLF 0.40) | Storage: partial cycling, or storage built for reserves, would lower the high; at 150.1 Climate stress and All upside fall back by 23.5 TWh (to 6,063 and 6,855). Imports: the Nepal target has no schedule or contract; at 15.3 All downside and Efficiency and distributed rise back by 34.0 TWh (to 4,865 and 5,076). The joint P50 would be 5,769 instead of 5,765 TWh, and the Robust plan's unmet energy at the joint P50 8.9 instead of 5.5 TWh |
| R36 | Factors from official conferences and texts are context rows only: none gives an India figure in TWh beyond the trend. Global figures (COP28's 11,000 GW of renewables, COP29's 1,500 GW of storage, the IEA's 945 TWh of data centres in 2030) are not scaled to India | A real effect may be missing from the layer, so the All downside to All upside envelope may be too narrow. The largest candidates move the mix, not the requirement: biomass co-firing at 5 percent of coal output would count about 45 TWh of the Robust plan's coal (893 TWh) as non-fossil and cut about 43 Mt; the Brahmaputra plan's 65 GW is 205.0 TWh of North-Eastern hydro against about 120 in the model; NGAP's firm share is 63.7 percent against the model's 45 percent. On the requirement side the EU CBAM and fossil-fuel subsidy reform would raise it and Mission LiFE would lower it, none by an official amount |
| R37 | CEA's requirement figures (4,582 BU for 2046-47, 5,173 BU for 2049-50, peaks of 697 and 777 GW) are context, not a benchmark: they are measured ex-bus, without the stations' own auxiliary use, while the forecast is gross generation. The road map's capacity mix is converted to TWh at the model's CUFs as context for the mix | If the 4,582 BU were the benchmark, CEA's figure after the gross-up (below) would be 6 to 8 percent above our median for the same year, inside our P10-P90 range. On the road map's own 2024-25 base (1,573 BU, below the actual 1,694 BU) CEA's path may be low; rebased on the actual, it gives about 5,475 TWh for FY2047-48, 16.8 percent above our median and below our P75 (5,516) |

**Effect on results** (before: the run of 6f19e6e, whose copies are in `docs/beamer/data/`; after: `outputs/` of 8800881; Base growth scenario, median requirement, real 2024 USD).

| Quantity | Before D15 | After |
| --- | --- | --- |
| FY2047-48 national P5 / P50 / P95 | 3,190 / 4,688 / 7,021 TWh | unchanged |
| Policy presets; `presets.csv` rows | 4; 12 | 5 (Nuclear mission added); 15 |
| Robust plan (recommended): non-fossil, CO2, cost, unmet at the median and at P90 | 79.9%, 891.3 Mt, 176.16 bn USD/yr, 0 and 639.5 TWh | unchanged |
| Nuclear mission plan: non-fossil, CO2, cost, unmet at the median and at P90 | none | 85.1%, 652.7 Mt, 177.65 bn USD/yr, 0 and 639.5 TWh |
| Nuclear in the plan; share of generation | 133.1 TWh; 2.8% (Robust) | Nuclear mission: 539.6 TWh; 11.5% |
| Nuclear by region, N / W / S / E / NE, TWh | Robust: 51.6 / 24.0 / 57.5 / 0 / 0 | Nuclear mission: 179.5 / 173.4 / 114.0 / 72.7 / 0 |
| Change against Robust by source, TWh (median) | | nuclear +406.5; coal -245.8 (to its floor, 647.4); offshore wind -113.4 (to 0); hydro -44.2; firmed RE -116.5; onshore wind +116.5; solar and gas unchanged |
| Eastern Region net imports | 155.3 TWh (Robust) | 328.3 TWh (Nuclear mission): its coal falls to its floor |
| Mission low case (54 GW, 321.7 TWh): non-fossil, CO2, cost | none | 83.9%, 709.1 Mt, 176.41 bn USD/yr; unmet 0 and 639.5 TWh |
| Mission high case (634.8 TWh) | none | infeasible: above the 627.1 TWh nuclear build cap |
| Mix sensitivity cases; solves | 8; 16 | 10; 20 |
| Demand-feature rows: core / driver / layer / excluded / context | 49: 7 / 3 / 11 / 1 / 27 | 73: 7 / 3 / 11 / 2 / 50 |
| Layer highs: storage round-trip losses; cross-border imports | 150.1; 15.3 TWh | 173.6; 49.3 TWh |
| Named scenarios, FY2047-48 P50 | Features central 5,810; Electrification high 6,828; Efficiency and distributed 5,076; Climate stress 6,063; All upside 6,855; All downside 4,865 TWh | 5,810; 6,828; 5,042; 6,086; 6,879; 4,831 TWh |
| Joint distribution P5 / P50 / P90 / P95 | 4,225 / 5,769 (+23.1%) / 7,506 / 8,121 TWh | 4,222 / 5,765 (+23.0%) / 7,503 / 8,118 TWh |
| Robust plan unmet at the joint P50; at the joint P90 | 8.9 TWh; 1,738 TWh | 5.5 TWh; 1,734 TWh |
| CEA 2047 figures in the docs | "CEA has published no 2047 figure" | the road map's 2046-47 requirement (context) and 2047 capacity mix (converted to TWh, context) |

**CEA's 2047 requirement, if a juror asks** (context, not a benchmark).
CEA's 2047 road map (October 2025) estimates a 2046-47 energy requirement of 4,582 BU (1 BU = 1 TWh). It is a requirement figure, measured ex-bus, without the stations' own auxiliary use, so it is not a like-for-like generation benchmark.
Grossed up for auxiliary use it is about 4,790 to 4,880 TWh for 2046-47: 4,582 / (1 - 0.0615), the FY2023-24 auxiliary rate in CEA's General Review 2025, gives 4,882; 4,582 / (1 - 0.0428), the 2047 rate of the `auxiliary_consumption` row, gives 4,787.
That is 6 to 8 percent above our median for the same year (4,524 TWh) and well inside our P10-P90 range for it (3,384 to 6,119).
Carried one more year at CEA's own growth from 2046-47 to 2049-50 ((5,173 / 4,582)^(1/3), 4.1 percent a year), it is about 4,980 to 5,080 TWh for FY2047-48, 6 to 8 percent above our 4,688 TWh median and inside our P10-P90 range (3,471 to 6,408).
The road map's only statement about 2047 generation is a share: "about 2/3rd" non-fossil, below every policy preset's floor (70 to 80 percent).

**Conferences and texts reviewed** (official outcome texts only; the press and think tanks only to locate documents; full list in [demand_features.md](demand_features.md), section 9).

- India: the National Conference of Power Ministers of States and UTs (12 Nov 2024); the five Regional Power Conferences of 2025 (Gangtok, Mumbai, Bengaluru, Chandigarh, Patna); the Bharat Electricity Summit 2026 and its ministerial meeting (19-22 Mar 2026); the CEA and MoP brainstorming session on the power sector in 2047 (14-15 Oct 2024); CEA's 52nd Foundation Day (11 Oct 2025), where the road map and the Brahmaputra hydro master plan were released; the Ministry of Power's press conference of 10 Jun 2025; India Energy Week 2025 and 2026; the International Solar Alliance's eighth Assembly (Oct 2025); the 4th RE-INVEST (Sep 2024); the Union Budgets 2025-26 and 2026-27; and replies in Parliament (DAE, Ministry of Heavy Industries, Ministry of Power).
- International: COP28 (decision 1/CMA.5 and the presidency pledges), COP29 (the declarations and pledges letter), COP30 (decisions 1/CMA.7 to 3/CMA.7 and the Belém 4X pledge) and COP31 (no adopted decision as of 21 Nov 2026); the G20 in New Delhi (2023), Brazil (2024), South Africa (2025) and the United States (2026); the G7 Critical Minerals Action Plan (2025); the Clean Energy Ministerial and Mission Innovation (2024 to 2026); the IEA ministerials of 2024 and 2026, its Global Conferences on Energy Efficiency (2023 to 2026) and its Energy and AI report (intergovernmental, secondary); the IRENA Assemblies of 2025 and 2026; the EU CBAM (Regulation (EU) 2023/956 and the 2025 proposal) and the India-EU FTA (Jan 2026); India's NDC 2031-35.

**Rejected.**

- The Nuclear mission as the recommended plan: a CO2 cap reaches the same CO2 for 1.04 bn USD a year less, and the mission adds nothing at P90.
- Counting the captive small reactors in the floor (R31).
- Cutting the high case to fit under the build cap: it would hide that the case exceeds the published build cap at the planning CUF (R33).
- Scaling global pledges to India (R36), and the interim nuclear milestones (22 GW by 2032, 47 GW by 2037, 67 GW by 2042) that appear only in ministerial remarks reported by the press, not in a DAE answer.
- CEA's 2046-47 requirement as a benchmark (R37).

**Limits.**

- The storage and land of the Nuclear mission plan were not back-calculated: `outputs/storage_land_2047.csv` covers the Base and Robust plans only.
- The floor is for 2047 only and ignores the road map's commissioning path; the model solves 2047, not the years before it.
- Siting follows the model's nuclear siting key. The road map leaves 52,540 MW (313.0 TWh at CUF 0.68) without an identified site, and the Ministry of Power's calls for a nuclear plant in every State and at retired coal sites are not modelled; the North-Eastern Region gets none.
- Nuclear's station auxiliary use (8.78 percent) is above solar's (1.94 percent). Moving about 406 TWh to nuclear adds about 28 TWh of auxiliary energy, which the annual balance does not see.
- The extra nuclear is priced at the IEA WEO 2025 cost interpolated to 2047, a single published path; the CEA cost basis also keeps the IEA nuclear cost, because the NEP gives none.
- No "CEA road map mix" policy (about two-thirds non-fossil, 1,210 to 1,340 TWh of coal, at least 916 TWh of wind, at most 14 TWh of gas) was built; the road map's mix is context only.
- Some texts could not be read: CEA's transmission plan to 2035-36 and the Brahmaputra master plan (404 or a stub page; only the PIB totals are used), part of PIB (automated access refused; ministry copies were read), IRENA (indexed pages only) and the CEM17 outcomes (none found). COP31's decisions were not yet posted.
- The road map's 2024-25 requirement (1,573 BU) is below the Ministry of Power's actual 1,694 BU, so its 2046-47 figure may rest on a low base (R37).
- The data-centre high remains our extrapolation of an official 2031-32 figure (the Ministry of Power's 26.3 GW), not an official 2047 figure (R30).
- **The road map is read from an archived copy.** CEA's own link (`cea.nic.in/wp-content/uploads/notification/2025/10/Roadmap_Final_30062025.pdf`) returned "not found" on 22 November 2026, and no live official copy was found; the figures come from the Internet Archive copy cited in `nuclear_mission.csv`, and PIB Release ID 2177769 records the road map's release on 11 October 2025. Every value used was re-checked page by page against that copy on 22 November 2026: Table 6.3 (PDF p. 50: cumulative 1,00,648 MW, BSR 5,060 MW, BSMR 5,000 MW), Table 5.3 and its note "Includes BSR/BSMR/SMR for captive uses" (PDF p. 41), Table 5.2 (PDF p. 40: 4,582 BU in 2046-47, 5,173 BU in 2049-50) and paragraph 11.1.3.1 (c) on the 80 percent PLF (PDF p. 77). If CEA republishes the road map, the link should be replaced and the values checked again.

**Checks** (`tests/`; the suite still has nine tests).

- `test_allocation` gains checks for the floor: the three cases load from the file with their TWh, and a missing, duplicate or unknown case, a captive MW not below the total, a CUF above 1 or a low case above the central throws; the four existing presets have no floor and reproduce their cost and CO2; a 450 TWh floor is met and sited within the regional bounds; the mission's floor holds at a requirement where it exceeds the share limit; a floor 5 TWh above the build cap is flagged infeasible and one 5 TWh below is solved; a negative or non-numeric floor throws; the preset is listed; the mix sensitivity has ten cases in order.
- `test_features` expects 73 rows, 50 of them context and 2 excluded by rule.
