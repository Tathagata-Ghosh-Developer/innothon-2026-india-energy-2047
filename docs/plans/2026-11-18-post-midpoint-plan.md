# Post-midpoint upgrade: implementation plan

> **Workflow:** one implementer per task, then a separate reviewer, then the next task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Answer the five midpoint-panel questions with evidence. Add published per-source growth limits and published figures (potentials, costs, emission factors) to the regional generation mix. Keep the two-dataset core estimate unchanged.

**Architecture:** Extend the existing C++20 pipeline:
- Part 1 (requirement): new diagnostics and sensitivities in `src/pipeline` and `src/forecast`.
- Part 2 (mix): new published inputs and LP constraints in `src/allocation`.
- `apps/energy2047.cpp` writes the new outputs.
- The docs, decks and site are refreshed from those outputs.

**Tech stack:** C++20, CMake/Ninja/MSVC, HiGHS, Eigen, CUDA (optional), ctest.

**Spec:** [2026-11-18-post-midpoint-design.md](2026-11-18-post-midpoint-design.md)

## Global constraints

- All repository code is C++. Use Python only for throwaway scripts outside the repo (`python -I`).
- Files and commit messages carry no tool attribution.
- Primary sources are NPP (https://npp.gov.in/landing-home) and the Renewable India dataset (https://github.com/chrisjaimyantony/renewable-india-dataset). Every other source is secondary. Give it a row in the matching `SOURCES.md` with URL, page or table, and access date.
- Installed capacity and consumption are never forecast metrics. GW figures are used only as limits on generation, after an explicit CUF conversion.
- The Part 1 estimation (`fit_india`, the bootstrap, the national headline) must not change. The default `outputs/forecast_national_regional.csv` national P50 stays 4,703 TWh, within ±0.5 TWh.
- Build:

  ```
  set CUDACXX=C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v13.5\bin\nvcc.exe
  set E47_BUILD_DIR=%USERPROFILE%\.e47build_core
  build.cmd
  ```

- Tests:

  ```
  "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir %USERPROFILE%\.e47build_core --output-on-failure
  ```

- Fiscal-year convention: start years, so 2047 = FY2047-48.
- Every result change gets an entry in `docs/decisions.md`, D13.

## Review focus

1. **A published build limit below the base year's generation.** The LP must stay feasible: cap = max(base, base + limit).
2. **Target years beyond the last period in `build_limits.csv`.** Hold the last period's annual rate. Do not drop to zero.
3. **State names in the NPP state file or the CEA renewable file that don't match the dataset or region map.** Skip the row, count it, and report the count. Never crash or silently misassign.
4. **Overlap in `new_loads.csv` with demand already in the historical trend (for example, cooling).** Report each load separately and flag the overlap. Never add it to the headline.
5. **A sensitivity run that changes a shared `Pipeline` or `PolicySettings` default.** Every sensitivity must start from a copy, so later rows and the main outputs are unaffected.

---

### Task 1: Published build limits, CEA v22 emission factors and CEA cost inputs (data only)

**Files:**
- Create: `data/external/techno_economic/build_limits.csv`
- Create: `data/external/techno_economic/cea_costs.csv`
- Modify: `data/external/techno_economic/emission_factors.csv`
- Modify: `data/external/techno_economic/SOURCES.md`

**Interfaces:**
- **Produces `build_limits.csv`** with columns `tech, period_start, period_end, gw_per_year_low, gw_per_year_central, gw_per_year_high, cuf, source_name, source_url, source_location, notes`.
  - `tech` is one of `solar, wind, offshore_wind, nuclear, hydro, bioenergy, gas, coal, pumped_storage`.
  - Periods are fiscal start years and cover 2026 to 2047.
  - Sources, in order of preference: CEA NEP 2022-32, the CEA input slides (Dec 2024), NITI Aayog 2026 Table 3.6, the DAE/NPCIL nuclear roadmap (100 GW by 2047), the CEA hydro pipeline.
- **Produces `cea_costs.csv`** with columns `tech, capex_cr_per_mw, fixed_om_pct, life_years, cuf, fuel_cost_rs_per_kwh, heat_rate, wacc_pct, year_basis, source_name, source_url, source_location`, all from CEA NEP 2022-32 Annexure 5.1 and Chapter 5.
- **Adds `emission_factors.csv` rows** for coal and gas with `period=2025-26`, from CEA CO2 Baseline Database v22.0 (Aug 2026), station-weighted rates. Keep the v21 rows.

- [ ] **Step 1:** Collect the values from the primary documents. If a CEA link is dead, use a mirror and record both URLs.
- [ ] **Step 2:** Write the three files and the `SOURCES.md` rows, each source marked "secondary".
- [ ] **Step 3:** Run this check (stdlib only, outside the repo): every CSV row has the same field count; the periods for each `tech` cover 2026-2047 with no gaps. Expected: "ok".
- [ ] **Step 4:** Commit with the message "Add published build limits, CEA v22 emission factors and CEA cost inputs".

### Task 2: Published new-load estimates (data only)

**Files:**
- Create: `data/external/new_loads.csv`
- Modify: `data/external/SOURCES.md`

**Interfaces:**
- **Produces `new_loads.csv`** with columns `load, year, twh_low, twh_central, twh_high, basis, overlaps_history, source_name, source_url, source_location, notes`.
  - `load` is one of `electric_vehicles, green_hydrogen, data_centres, space_cooling`, plus others found.
  - `year` is 2047, or 2050 with `basis` saying so.
  - `overlaps_history` is `yes`, `partial` or `no`.

- [ ] **Step 1:** Find published 2047 or 2050 electricity estimates for India. Candidates: CEA, NITI Aayog, IEA WEO 2025, MNRE National Green Hydrogen Mission, published data-centre studies, India Cooling Action Plan.
- [ ] **Step 2:** Write the file and its `SOURCES.md` rows, marked "secondary".
- [ ] **Step 3:** Commit with the message "Add published new-load estimates for a separate scenario".

### Task 3: Elasticity controls and sensitivities

**Files:**
- Modify: `src/pipeline/pipeline.hpp` (`ScenarioControls`)
- Modify: `src/pipeline/pipeline.cpp` (`boot_for`)
- Modify: `apps/energy2047.cpp` (sensitivity rows)
- Test: `tests/test_india.cpp`

**Interfaces:**
- **Produces** `ScenarioControls::beta_shift` (`double`, default 0), which is added to beta in every replicate.
- **Produces** `ScenarioControls::beta_window` (`int`, default 0). 0 uses the mixture, 1 uses S1 replicates only, 2 uses S3 replicates only.
- **Produces** new rows in `outputs/sensitivity_2047.csv`: `beta_minus_1se`, `beta_plus_1se`, `beta_window_s1`, `beta_window_s3`, `beta_first_difference`. The last uses `beta_shift = fd_beta - mean(beta_boot)`.

- [ ] **Step 1:** Write the failing test `elasticity_shift_monotone`. With `draws = 1<<14`, assert median 2047 at `beta_shift = -0.063` < the median at 0 < the median at `+0.063`. Also assert `beta_window = 1` and `beta_window = 2` both give finite medians, and that window 2's median is ≥ window 1's (S3 beta 0.866 > S1 0.828).
- [ ] **Step 2:** Build, then run `ctest -R india`. Expected: compile failure, because `beta_shift` is not a member.
- [ ] **Step 3:** Implement the two fields and apply them in `boot_for` (b1 += beta_shift; replicate selection by window).
- [ ] **Step 4:** Run `ctest -R india`. Expected: PASS. Run all ctest. Expected: 7/7.
- [ ] **Step 5:** Add the five sensitivity rows in `energy2047.cpp`, each run from a copy of the default controls. Run `energy2047` and check the `base` row is unchanged.
- [ ] **Step 6:** Commit.

### Task 4: Growth decomposition

**Files:**
- Modify: `src/pipeline/pipeline.hpp` and `pipeline.cpp`
- Modify: `apps/energy2047.cpp`
- Test: `tests/test_india.cpp`

**Interfaces:**
- **Produces** `struct GrowthDecomposition { int base_year, target_year; double total, population, income, urbanisation, model_error, gen_base_twh, gen_target_twh; };`. All parts are log points.
- **Produces** `GrowthDecomposition Pipeline::decompose_central(const ScenarioControls& c, int target_year) const`. It follows the central path of the India core, with zero add-on:
  - population = Δln pop;
  - income = (b1 terms of income) = Δ of the income part of the per-person index;
  - urbanisation = γ̄ (Δu − κΔx);
  - model_error = E[resid_target] − resid0.
- **Produces** `outputs/growth_decomposition_2047.csv`. Each part is written in log points and as a percentage share of the total.

- [ ] **Step 1:** Write the failing test `decomposition_adds_up`. Assert that |population + income + urbanisation + model_error − total| < 1e-9, that total = ln(gen_target/gen_base) on `forecast(c).central[0]` within 1e-6, and that population > 0 and income > 0.
- [ ] **Step 2:** Run it. Expected: FAIL (undefined).
- [ ] **Step 3:** Implement the decomposition, reusing the same driver paths as `build_inputs` and the central path.
- [ ] **Step 4:** Run it. Expected: PASS. Then write the CSV from `energy2047.cpp`.
- [ ] **Step 5:** Commit.

### Task 5: State-wise table and state panel check

**Files:**
- Create: `src/forecast/state_panel.hpp`, `src/forecast/state_panel.cpp` (and add to `src/CMakeLists.txt`)
- Modify: `src/pipeline/pipeline.hpp` and `pipeline.cpp`
- Modify: `apps/energy2047.cpp`
- Test: `tests/test_regions.cpp`

**Interfaces:**
- **Produces** `struct PanelObs { std::string state; int year; double ln_gen_pc, ln_income_pc; };`
- **Produces** `struct PanelFit { double slope, se; int n_obs, n_states, n_skipped; };`
- **Produces** `PanelFit fe_slope(const std::vector<PanelObs>&)`, a within (state fixed effects) OLS.
- **Produces** `PanelFit state_panel_check(const std::string& repo_root)`:
  - Reads NPP `data/raw/npp/npp_state_generation_fy.csv` and the dataset's population and GSDP.
  - Covers FY2018-19 to FY2023-24.
  - Skips unmatched names and counts them in `n_skipped`.
- **Produces** `struct StateRow { std::string state, region, region_from; double pop_share_base, ln_rel_income_base, ln_rel_income_target, per_person_index_target, requirement_share_target; };`
- **Produces** `std::vector<StateRow> Pipeline::state_table(const ScenarioControls& c, int target_year) const`, using the mean convergence rate and the central income path.
- **Produces** `outputs/state_income_2047.csv` (one row per state), and `outputs/state_panel_check.csv`, which also records the spread of state generation per person against the national figure.

- [ ] **Step 1:** Write the failing tests:
  - `fe_slope_recovers_known`: 5 synthetic states, 6 years, slope 0.8, state effects; assert |slope − 0.8| < 1e-9.
  - `fe_slope_skips_unmatched`: one unknown name, so `n_skipped == 1`.
  - `state_table_shares_sum`: the shares sum to 1 within 1e-9, there are 34 rows, and the sum of shares by region matches the regional medians' shares within 0.02.
- [ ] **Step 2:** Run them. Expected: FAIL.
- [ ] **Step 3:** Implement. Name matching uses the existing state-name normaliser in `src/regional/states.*`.
- [ ] **Step 4:** Run them. Expected: PASS. Then write both CSVs.
- [ ] **Step 5:** Commit.

### Task 6: New-loads scenario

**Files:**
- Modify: `src/pipeline/pipeline.hpp` and `pipeline.cpp`
- Modify: `apps/energy2047.cpp`
- Modify: `apps/dashboard/app.cpp` (one preset)
- Test: `tests/test_regions.cpp`

**Interfaces:**
- **Consumes** `data/external/new_loads.csv` (Task 2).
- **Produces** `struct NewLoad { std::string load, basis, overlaps; double low, central, high; };`
- **Produces** `std::vector<NewLoad> Pipeline::new_loads() const`.
- **Produces** `outputs/new_loads_2047.csv` (each load, and totals with and without overlapping loads).
- **Produces** the sensitivity row `new_loads_published`: `addon_lo` = sum of non-overlapping lows, `addon_hi` = sum of non-overlapping highs.
- **Produces** a dashboard scenario preset "New loads (published, outside headline)".

- [ ] **Step 1:** Write the failing test `new_loads_excludes_overlap`. Using a temp CSV with one `yes` overlap load, the addon totals exclude it, and the headline P50 with zero addon is unchanged.
- [ ] **Step 2:** Run it. Expected: FAIL.
- [ ] **Step 3:** Implement it.
- [ ] **Step 4:** Run it. Expected: PASS. Then write the output and add the preset.
- [ ] **Step 5:** Commit.

### Task 7: Regional base mix from NPP (primary) plus CEA regional renewables (secondary)

**Files:**
- Modify: `src/allocation/inputs.cpp` (section 1, base generation)
- Test: `tests/test_allocation.cpp`

**Interfaces:**
- **Produces** `AllocationInputs::base_twh` for FY2023-24, built as follows:
  - **Conventional:** NPP `data/raw/npp/npp_region_generation_fy.csv` (thermal, nuclear, hydro by region).
  - **Thermal split:** coal and gas shares by region from `data/raw/generation/region/cea_region_generation_fy.csv`. If that file has no split, use the NPP national split and record that in `notes`.
  - **Renewables:** solar, wind, small hydro and biomass by state from `data/raw/generation/state/cea_state_res_by_type_fy.csv`, aggregated with the NPP region map.
- **Produces** `AllocationInputs::base_source_notes`, the provenance lines, each primary or secondary.

- [ ] **Step 1:** Write the failing test `base_mix_matches_npp`:
  - For each region, (coal + gas + nuclear + hydro) equals the NPP FY2023-24 regional conventional total within 0.5%.
  - Renewables are positive in every region except where CEA shows none.
  - The unmatched-state count is reported and is 0 for the current files.
- [ ] **Step 2:** Run it. Expected: FAIL.
- [ ] **Step 3:** Implement it. Keep Ember only as a cross-check note.
- [ ] **Step 4:** Run all allocation, storage and pareto tests. Expected: PASS. Update any test value that legitimately moves, with the reason in the commit body.
- [ ] **Step 5:** Commit.

### Task 8: Published per-source build limits in the LP, NPP record history

**Files:**
- Modify: `src/allocation/inputs.hpp` and `inputs.cpp`
- Modify: `src/allocation/allocate.hpp` and `allocate.cpp` (the `rate_cap` block)
- Modify: `src/pipeline/presets.hpp`
- Test: `tests/test_allocation.cpp`

**Interfaces:**
- **Consumes** `build_limits.csv` (Task 1).
- **Produces** `std::array<std::array<double, kTechs>, 3> AllocationInputs::published_build_twh`. This is the cumulative TWh a source can add between `rate_last_year` and 2047, for the low, central and high cases. Each year's GW per year is multiplied by its CUF × 8.76. The last period's rate is held past its end.
- **Produces** `AllocationInputs::national_last_twh`, `max_annual_increase` and `rate_last_year`, now taken from NPP `data/raw/npp/npp_national_generation_fy.csv` (primary) instead of Ember.
- **Produces** `PolicySettings::build_case` (`int` 0, 1 or 2; default 1) and `PolicySettings::build_accel` (now a multiplier on the published limit; default 1.0).
- **Produces** the cap for each tech group: national_last + build_accel × published_build_twh[build_case]. It falls back to the record-based bound when a tech has no published row. The cap is never below base.
- **Produces** retuned presets: Base keeps the central case and multiplier 1, Robust uses the high case. The tornado "Build-rate acceleration" lever runs 0.75 to 1.5.

- [ ] **Step 1:** Write the failing tests:
  - `build_limit_binds`: a low solar limit makes total solar equal the cap, and the LP stays feasible.
  - `build_limit_below_base_feasible`: a limit of 0 still allows base generation.
  - `build_limit_holds_last_period`: a period ending in 2040 still allows growth in 2041 to 2047.
- [ ] **Step 2:** Run them. Expected: FAIL.
- [ ] **Step 3:** Implement it.
- [ ] **Step 4:** Run all ctest. Expected: PASS.
- [ ] **Step 5:** Commit.

### Task 9: CEA v22 emission factors, CEA cost basis and potential sensitivities

**Files:**
- Modify: `src/allocation/inputs.hpp` and `inputs.cpp`
- Modify: `src/allocation/allocate.hpp` and `allocate.cpp`
- Modify: `apps/energy2047.cpp`
- Test: `tests/test_allocation.cpp`

**Interfaces:**
- **Produces** `emission_t_per_mwh`, taken from the latest `period` in the file (v22, FY2025-26).
- **Produces** `RegionTech AllocationInputs::cost_cea`, the levelised cost from `cea_costs.csv`:
  - CRF(wacc, life) × capex / (CUF × 8,760) + fixed O&M + fuel.
  - Converted at the INR/USD rate already used in `inputs.cpp`.
  - Regional resource quality is applied the same way as for the IEA costs.
- **Produces** `PolicySettings::cost_basis` (0 = IEA default, 1 = CEA).
- **Produces** `PolicySettings::solar_potential_set` (0 = NISE 2025 default, 1 = NISE 2014).
- **Produces** `PolicySettings::wind_height` (150 default, or 120).
- **Produces** `outputs/sensitivity_mix.csv` with columns `case, non_fossil_pct, coal_pct, co2_mt, cost_busd, unserved_p90_twh`, for these cases:
  - `base`
  - `cost_basis_cea`
  - `solar_potential_nise2014`
  - `wind_120m`
  - `build_low`
  - `build_central` (R15: the Robust preset already uses the high case)

- [ ] **Step 1:** Write the failing tests:
  - `emission_factor_latest_period`: picks the 2025-26 rows.
  - `cea_lcoe_formula`: a known case gives the expected value within 1e-9.
  - `sensitivity_from_copy`: running the cases leaves the default `PolicySettings` unchanged.
- [ ] **Step 2:** Run them. Expected: FAIL.
- [ ] **Step 3:** Implement it.
- [ ] **Step 4:** Run all ctest. Expected: PASS.
- [ ] **Step 5:** Commit.

### Task 10: Re-run, check results, record D13

**Files:**
- Modify: `outputs/*` (regenerated)
- Modify: `docs/decisions.md` (D13)
- Modify: `apps/dashboard/app.cpp` (labels and tables for the new outputs)

- [ ] **Step 1:** Do a full build, run all ctest (expected all pass), run `energy2047`, then check:
  - the national P50 is unchanged;
  - the regional medians are unchanged (the base mix does not touch the split);
  - every new file exists.
- [ ] **Step 2:** Compare the Robust and Base plan numbers before and after, and explain every change in D13: the base mix source, published build limits, v22 factors.
- [ ] **Step 3:** Add the new tables to the dashboard's data tab, then run the dashboard `--shot` to check.
- [ ] **Step 4:** Commit.

### Task 11: Panel answers, docs, decks, landing page, deploy

**Files:**
- Create: `docs/midpoint_panel_answers.md`
- Modify: README; `docs/methodology.md`, `results.md`, `assumptions_and_uncertainty.md`, `data_audit.md`, `team_plan.md`; `docs/beamer/*.tex` and `docs/beamer/data/*.csv`; `docs/img/*`; `web/landing.html`
- Modify: the `site_summary` keys in `energy2047.cpp`, if needed

- [ ] **Step 1:** Write `midpoint_panel_answers.md`. Give one section per question, with numbers from the new outputs and named primary sources (NPP; the Renewable India dataset), and any secondary sources labelled.
- [ ] **Step 2:** Refresh the docs and decks:
  - Part 2 uses published figures, per the kickoff guidance;
  - the source hierarchy;
  - growth decomposition, elasticity, state table and panel check, new loads, build limits.
- [ ] **Step 3:** Rebuild the PDFs (`latexmk -pdf`) and check them visually. Then build the site (`build_web.cmd`), deploy with `vercel deploy --prod --yes` from `web/dist`, and check the live `/summary.json`.
- [ ] **Step 4:** Commit and push. Check that CI passes.

### Task 12: Whole-branch review

- [ ] **Step 1:** Run an independent code review of all commits since this plan (correctness, rules compliance, source labelling).
- [ ] **Step 2:** Fix the findings, re-run ctest and CI, and push.

---

## Addendum (19 Nov 2026): alignment with the official challenge page

The official page (https://em-innothon-challenge.web.app/, last modified 4 Nov 2026) has not changed since the kickoff. It frames the strategy around six energy vectors (coal, hydro, solar, wind, nuclear, natural gas). Its regional cards name solar in the Northern Region, wind and natural gas in the Western Region ("Transition Fuel: flexible peaking capacity, LNG terminal infrastructure, reliable bridge fuel for intermittent renewables"), hydro in the North-Eastern Region and nuclear in the Southern Region. Its key evidence sources are historical generation, population and demographics, the existing regional generation mix, geographic and climatic indicators, land availability, and economic development and urbanisation.

### Task 9a: Committed builds and the role of gas

**Files:** `src/allocation/inputs.*`, `src/allocation/allocate.*`, `src/pipeline/presets.hpp`, `apps/energy2047.cpp`, `tests/test_allocation.cpp`

- **Committed-build floors.** Nuclear units under construction or sanctioned (`nuclear_pipeline.csv`) and large hydro under construction (`resource_potential.csv`, `under_construction_mw`) set lower bounds on 2047 generation in their regions. Each bound is GW × the CUF from `build_limits.csv` × 8.76. Plants already being built generate in 2047 whatever the LP's cost ranking says. This is standard practice: existing and committed capacity is exogenous in capacity-expansion models.
- **Gas as a peaking and transition fuel.** Add `PolicySettings::gas_min_share` (default 0 in Base). The Robust preset keeps the existing gas fleet in service for peaking: gas generation of at least the NPP fleet floor (R11), sited by the regional base shares, with most in the Western Region. The hourly storage check reports how many evening-peak hours gas covers.
- **Tests:** `committed_nuclear_floor_binds`, `gas_min_share_respected`, and the LP stays feasible at the defaults.

### Task 11b: Landing page in the structure of the official challenge page

**Files:** `web/landing.html` (and `cmake/Site.cmake` only if a new template feature is needed)

- Follow the official page's information architecture and visual language: white background, navy headings, a red accent for the call to action, a large two-line headline, stat cards, numbered steps and deliverable cards, a timeline, and an FAQ accordion.
- Map our evidence onto it:
  - the hero, with 4,703 TWh and the three headline cards;
  - "at a glance" (2047, national + regional, energy generated);
  - an interactive regional map with six vector chips, using our numbers and each region's strength;
  - Steps 01 Forecast, 02 Strategy, 03 Explore;
  - key evidence sources, each labelled primary or secondary;
  - Deliverables 01-05, each linked to its evidence;
  - our timeline against the official one;
  - an FAQ built from the midpoint panel's questions;
  - the existing charts (fan, holdout, drivers, regions, mix, trade-off, land and storage).
- **Do not copy** the ExxonMobil logo, wordmark or any brand asset, and do not present the page as the official site. The header names the team and says "Submission to ExxonMobil INNOTHON 2026".
- Keep the accessibility standard: WCAG 2.2 AA, keyboard-operable accordion and map chips, light and dark themes, no layout shift, and a table view for every chart.
