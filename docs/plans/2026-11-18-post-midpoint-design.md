# Post-midpoint upgrade: design

Date: 18 November 2026. Final evaluation: 21-22 November 2026.

## Why

The midpoint panel asked five questions:

1. Is income growth the only factor for the 2047 requirement? If not, what else should be accounted for?
2. Is income data used state-wise, region-wise or for India as a whole? (Use state-wise data from trusted sources, name them, and add it if absent.)
3. Renewable energy data is missing or mostly empty. How did you manage?
4. How does the elasticity affect the result, and was it estimated state-wise, region-wise or for India as a whole?
5. What hurdles did you face, and how will you mitigate them?

The team also asked for two blocks to be added, following the kickoff guidance:

- Use publicly available literature for the growth rates and constraints of each energy source.
- Use published figures to decide the regional mix: MNRE/NIWE resource potentials, IEA and CEA costs, CEA emission factors.

The source hierarchy is fixed (`docs/data_audit.md`). NPP and the Renewable India dataset are primary. Every other source is secondary and labelled.

## What the code does today (recon)

- **State income is already used state-wise in the regional split.** For each of the 34 states, the Monte Carlo kernel (`src/forecast/mc.hpp`) applies the national income response to that state's own income per person. That income is the state's relative income from the Renewable India GSDP and population, converging at a rate estimated from the same dataset (`src/regional/convergence.cpp`). The kernel weights each state by its population share and sums the states into NPP's five regions.
- **The elasticity is estimated on India as a whole.** The national generation per person (NPP) is regressed on the national income per person, chained from the dataset's states.
- **Build-rate limits in the mix come from Ember national history.** Ember is secondary. Each source's limit is its record annual increase times `build_accel`. There is no published per-source build limit.
- **Emission factors come from CEA CO2 Baseline Database v21.0** (FY2024-25). Version 22.0 (FY2025-26) is out.
- **Costs come from IEA WEO 2025 LCOE projections**, plus Indian tariff ratios. There is no CEA cost cross-check.
- **Potentials:** NISE 2025 ground-mounted solar and NIWE 150 m wind are already the defaults.
- **The regional base mix comes from Ember state data** (CY2024, utilities, secondary). NPP's regional conventional generation is not used for it.

## Goals

1. Answer the five questions with numbers and named sources (`docs/midpoint_panel_answers.md`).
2. Show the factors behind the 2047 total, beyond income:
   - a growth decomposition (population, income, urbanisation, model error);
   - elasticity sensitivities;
   - a labelled new-loads scenario from published estimates, outside the headline.
3. Show the elasticity at all three levels:
   - national estimate (unchanged);
   - state-wise application (a state table);
   - a state panel check that shows why plant location makes a state regression misleading.
4. Add published per-source growth rates and build limits as constraints in the mix.
5. Make the published figures that decide the mix explicit and current:
   - CEA v22 emission factors;
   - IEA costs with a CEA-based cost sensitivity;
   - MNRE/NISE/NIWE potentials, with a potential sensitivity.
6. Primary first: the regional base mix uses NPP conventional generation by region (FY2023-24). CEA renewable generation by region (secondary) fills only the gap NPP leaves.
7. Refresh the docs, decks, dashboard text, landing page and live site.

## Non-goals

- No change to the two-dataset core estimation. The headline stays NPP plus Renewable India.
- No new external data inside Part 1. The new-loads layer is a separate, labelled scenario.
- No hourly capacity-expansion model.

## Anti-goals

- Any installed-capacity or consumption figure used as a forecast metric.
- An unlabelled secondary source.
- A result change that is not explained in `docs/decisions.md`.

## Approaches considered

- **A (chosen): extend the existing C++ pipeline.** Add new outputs, tests and data files, and re-run. This keeps one engine for the CLI, the dashboard and the site.
- **B: offline analysis notebooks.** Rejected: the project's code must be C++, and the results would not reach the dashboard.
- **C: rebuild the mix as an hourly model.** Rejected: too large for three days, and it would be a non-goal.

## Design

### Part 1 additions (`src/pipeline`, `apps/energy2047.cpp`, `tests/`)

- **Growth decomposition**, `outputs/growth_decomposition_2047.csv`. It splits ln(2047 / base) on the central path into population, income (beta times the income change), urbanisation (gamma times the excess urbanisation) and model error. The parts must add up to the total; a test checks this.
- **Elasticity sensitivities**, new rows in `outputs/sensitivity_2047.csv`:
  - beta minus one s.e. and plus one s.e.;
  - S1 window only;
  - S3 window only;
  - the first-difference beta.

  These need a `beta_shift` control in `ScenarioControls`, applied in `boot_for`. A test checks that the 2047 median rises with beta.
- **State-wise table**, `outputs/state_income_2047.csv`: for each state, base relative income, convergence rate, 2047 per-person index, 2047 requirement share and region. It is evidence for questions 2 and 4.
- **State panel check**, `outputs/state_panel_check.csv`:
  - The regression is ln(NPP state generation per person) on ln(dataset GSDP per person), with state fixed effects, over FY2018-19 to FY2023-24.
  - It reports the coefficient, its s.e., and the spread of state generation per person against the national average.
  - It is a diagnostic only, never used.
  - A test runs it on synthetic data with a known slope.
- **New-loads scenario**, `data/external/new_loads.csv` (secondary):
  - Published 2047 estimates for electric vehicles, green hydrogen, data centres and cooling, each with a source.
  - Exposed as a scenario preset through the existing add-on range, and reported separately in `outputs/new_loads_2047.csv`.
  - Never in the headline.

### Part 2 additions (`src/allocation`, `data/external/techno_economic/`, `tests/`)

- **Published build limits**, `build_limits.csv`:
  - Contents: GW per year by source and period, from CEA NEP 2022-32, the CEA input slides (Dec 2024), NITI Aayog 2026 (Table 3.6), the DAE nuclear target (100 GW by 2047) and CEA hydro pipeline timelines.
  - Conversion: each limit becomes TWh with the stated CUF, and the LP caps each source's growth from its base.
  - Record history: the observed record annual increase now comes from NPP national generation by source (primary) instead of Ember.
  - Lever: `build_accel` becomes a multiplier on the published limits, default 1. Presets are re-tuned and the change is recorded in `docs/decisions.md`.
  - Tests: the LP never exceeds a limit, and a binding limit shows a positive dual.
- **Emission factors:** CEA CO2 Baseline Database v22.0 (FY2025-26), coal and gas station-weighted rates. Version 21 stays as a history row.
- **Costs:**
  - IEA WEO 2025 stays the default.
  - A CEA-based LCOE is computed from NEP 2022-32 capital costs, with CEA's stated CUF, O&M and fuel assumptions, as a labelled sensitivity ("cost basis: CEA") in `outputs/sensitivity_mix.csv`.
- **Potentials:** NISE 2025 and NIWE 150 m remain the defaults. A sensitivity uses NISE 2014 (748 GW) and NIWE 120 m.
- **Regional base mix:** NPP regional conventional generation for FY2023-24 (thermal split into coal and gas by NPP or CEA shares, plus nuclear and hydro), plus CEA state renewable generation for FY2023-24 aggregated to regions (secondary). This replaces Ember for the base. Ember remains only as a cross-check.

### Outputs, docs and site

- Re-run `energy2047`, then refresh `docs/beamer/data`, the decks, results, methodology, assumptions, data audit, decisions (new D13), team plan, README, dashboard text and the landing page. Redeploy to Vercel.
- `docs/midpoint_panel_answers.md`: one section per question, with numbers from the new outputs and named primary sources.

## Order and parallelism

1. Data, in parallel:
   - build limits, CEA v22 factors and CEA costs;
   - new-loads estimates.
2. Implementation, with tests written first:
   - Part 1 additions;
   - Part 2 additions.

   These touch different files except `apps/energy2047.cpp`, so they run one after the other.
3. Re-run, review, docs and site.

## Left to the builder

- Exact CSV column names.
- CUF values used to convert GW per year into TWh: take them from the same source as each limit, or from CEA NEP regional CUFs.
- How the dashboard shows the new tables (a table in the existing data tab is enough).
