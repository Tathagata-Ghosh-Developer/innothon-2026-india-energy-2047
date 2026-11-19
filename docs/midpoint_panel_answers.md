# Midpoint panel: answers to the five questions

At the midpoint review (18 November 2026) the panel asked five questions.
This file answers each one in three parts: the short answer, the evidence and the sources.

- Every number comes from the current run (`outputs/`, regenerated with `energy2047 <repo_root>`) or from [decisions.md](decisions.md), D13. The file behind each number is named next to it.
- Energy is energy generated, in TWh. Installed capacity and consumption are never forecast metrics.
- Years are fiscal years labelled by their start year: 2023 is FY2023-24 and 2047 is FY2047-48.
- The work done since the midpoint to answer these questions is recorded in [decisions.md](decisions.md), D13. The 2047 requirement did not change: the forecast files are byte-identical to the midpoint run.

**Source hierarchy.** Two sources are primary, because the organisers provided them:

1. **National Power Portal (NPP)**, <https://npp.gov.in/landing-home>, including the CEA generation reports it hosts (18-column monthly reports, daily generation reports, Merit real-time data).
2. **Renewable India dataset**, <https://github.com/chrisjaimyantony/renewable-india-dataset> (file `data/renewable_energy_dataset.csv`; our copy matches it byte for byte).

Every other source is secondary and is labelled so below.
Provenance of every input is in [data_audit.md](data_audit.md).

## 1. Is income growth the only factor for the 2047 requirement?

**Short answer.**
No. Income is the largest factor, but the forecast also carries population growth, urbanisation beyond what income implies, and the model's own error.
On the central path, income accounts for 87 percent of the growth from FY2023-24 to FY2047-48, population for 15 percent and urbanisation for 5 percent, while the fading of the unusually high FY2023-24 level takes back 7 percent.
Genuinely new loads, such as green hydrogen, are not in the history the model learns from, so they are shown as a separate scenario outside the headline.

**Evidence.**

Growth decomposition of the central path, 1,698 TWh in FY2023-24 to 4,600 TWh in FY2047-48 (`outputs/growth_decomposition_2047.csv`):

| Part | What it measures | Log points | Share of growth |
| --- | --- | --- | --- |
| Population | Population growth (UN WPP 2024 medium path) | 0.148 | 14.8% |
| Income | Elasticity 0.828 x the change in ln real GSDP per person | 0.872 | 87.5% |
| Urbanisation | gamma 1.0 x urbanisation beyond the path income implies | 0.048 | 4.8% |
| Model error | Expected FY2047-48 residual minus the FY2023-24 residual (+0.070) | -0.070 | -7.1% |
| **Total** | ln(4,600 / 1,698) | **0.997** | **100%** |

- The parts add up to the total exactly (test `decomposition_adds_up`).
- The decomposition is of the central path (medium drivers, mean income growth, point estimates), which is 4,600 TWh. The headline median of the Monte Carlo ensemble is 4,703 TWh.

How each driver moves the FY2047-48 median (`outputs/sensitivity_2047.csv`; base 4,703 TWh):

| Case | Median, TWh | Change |
| --- | --- | --- |
| Income growth 1.5 points a year above history | 6,169 | +31.2% |
| Income growth 1.5 points a year below history | 3,588 | -23.7% |
| Elasticity falls 0.2 per unit of ln income | 4,197 | -10.7% |
| Urban share 5 points above the UN path by 2047 | 4,954 | +5.3% |
| No urbanisation term (gamma = 0, the two datasets only) | 4,453 | -5.3% |
| Population uncertainty doubled | 4,694 | -0.2% |
| Damped trend without drivers (benchmark) | 3,604 | -23.4% |

The elasticity cases are under question 4.

New loads (`outputs/new_loads_2047.csv`). The published values are for 2050 and are scaled to 2047 by 24/27 (0.889). They are end-use electricity, not grossed up for losses.

| Load | Low | Central | High | In the scenario? |
| --- | --- | --- | --- | --- |
| Green hydrogen | 418 | 796 | 1,237 | Added: absent from the historical series |
| Electric vehicles | 373 | 405 | 687 | Reported only: the income elasticity already carries part of it |
| Data centres | 356 | 356 | 565 | Reported only, for the same reason |
| Space cooling | 517 | 697 | 1,200 | Reported only, for the same reason |
| Electric cooking | 117 | 117 | 135 | Reported only, for the same reason |
| All five (reference, never added) | 1,781 | 2,372 | 3,825 | Would double count |

TWh in FY2047-48.

- With green hydrogen added, if it is supplied from the grid, the FY2047-48 median is 5,537 TWh, 17.7 percent above the headline (`outputs/sensitivity_2047.csv`, row `new_loads_published`). Electrolysers on captive or off-grid renewables would be outside NPP utility generation.
- This scenario is never in the headline. The headline P90 (6,400 TWh) is already 1,697 TWh above the median, and the recommended plan is tested against it (question 5).

**Sources.**

- Primary: NPP all-India generation, FY2006-07 to FY2023-24 (`data/raw/npp/npp_national_generation_fy.csv`). Renewable India dataset: state population and GSDP, the only columns read.
- Secondary: UN WPP 2024 (India's population path) and UN WUP 2018 (India's urban share path), which the organisers' ruling of 16 November 2026 allows if consistent with the dataset's state growth (`outputs/population_consistency.csv`).
- Secondary: Liddle 2013, Shahbaz et al. 2016 and Holtedahl and Joutz 2004, behind the urbanisation prior; the organisers ruled that the coefficient is the team's own judgment.
- Secondary, new loads only: NITI Aayog (Feb 2026), TERI (Feb 2024), CEEW (June 2025), BloombergNEF (Oct 2026) and IEA (2018), in `data/external/new_loads.csv` (`data/external/SOURCES.md`, section 10).

## 2. Was income data used state-wise, region-wise or for India as a whole?

**Short answer.**
State-wise. The income data are the GSDP and population of 34 states and union territories in the Renewable India dataset, a primary source provided by the organisers.
National income growth is chained over the states.
The regional split then applies the income response to each state's own income per person, with the states' incomes converging over time, and sums the states into NPP's five grid regions.

**Evidence.**

- **National income per person.** State GSDP and population are summed over the states reported in both of two consecutive years, and the national series is chained from those growth rates. GSDP values copied unchanged from the previous year are left out, the 2011 price-base change is spliced, and the incomplete 2024 values are not used ([methodology.md](methodology.md), section 2.1). Mean growth of income per person, FY2012-13 to FY2023-24 without the pandemic years: 4.95 percent a year (`outputs/india_fit.csv`, row `g_hist_mean`).
- **Relative income and convergence.** Each state's 2024 GSDP per person relative to India's comes from the dataset. The gaps close at 2.1 percent a year (lambda 0.0208), estimated from the same dataset over 2006-2024, a half-life of about 33 years (`outputs/state_income_2047.csv`, column `convergence_lambda_per_year`).
- **Regional split.** Each state's weight is its population share times its generation per person relative to a person at India's income. The regional requirement is the national total times the region's share of the weights. The split covers 34 states and union territories (`outputs/site_summary.json`, `regions.state_count`).

Four examples from `outputs/state_income_2047.csv`:

| State | Grid region | Population share, 2024 | Income per person relative to India, 2024 to 2047 | Generation per person relative to India's, 2047 | Population share, 2047 | Share of the 2047 requirement |
| --- | --- | --- | --- | --- | --- | --- |
| Uttar Pradesh | Northern | 17.0% | 0.42 to 0.58 | 0.67 | 17.6% | 12.4% |
| Bihar | Eastern | 9.2% | 0.32 to 0.49 | 0.59 | 10.3% | 6.4% |
| Maharashtra | Western | 9.1% | 1.36 to 1.21 | 1.15 | 8.8% | 10.6% |
| Delhi | Northern | 1.6% | 2.75 to 1.87 | 1.59 | 1.9% | 3.2% |

- Bihar holds 10.3 percent of India's population in 2047 but 6.4 percent of the requirement; Delhi holds 1.9 percent of the population and 3.2 percent of the requirement.
- Generation per person relative to India's runs from 0.59 (Bihar) to 1.77 (Goa) in 2047.
- Summed by region, the state table is within 0.3 points of the regional medians of the Monte Carlo ensemble (`outputs/site_summary.json`, `regions.list[].share_2047_pct`):

| Region | Sum of the states (state table) | Monte Carlo regional median |
| --- | --- | --- |
| Northern | 30.0% | 29.9% |
| Western | 26.0% | 26.1% |
| Southern | 22.5% | 22.7% |
| Eastern | 18.1% | 17.9% |
| North-Eastern | 3.4% | 3.4% |

**Sources.**

- Primary: Renewable India dataset, State, Year, Zone, Population and GSDP columns only (its installed-capacity columns are never read). NPP state generation reports (CEA 18-column Sub-report 16, `data/raw/npp/npp_state_generation_fy.csv`), used here only for the grid region each state's generation is reported under.
- Secondary: UN WPP 2024 medium growth for India, on which each state's population path is built; each state keeps its own 2019-2024 offset from the dataset's national growth (`outputs/population_consistency.csv`).
- No other state income source is used: the organisers ruled on 16 November 2026 that the regional split must use the challenge dataset.

## 3. Renewable energy data is missing or mostly empty. How did you manage?

**Short answer.**
For the 2047 requirement nothing is missing: NPP's all-India generation, which the model explains, includes renewables up to FY2023-24, the base year.
After FY2023-24 NPP publishes no renewable generation, so the two holdout years extend renewables at their recent growth.
NPP has no renewable generation by region or state at all (we checked every NPP service), so the FY2023-24 regional base mix of the generation plan takes regional renewables from the CEA General Review, a secondary source, labelled as such.
The Renewable India dataset holds installed capacity, not generation, so it is only a qualitative check.

**Evidence.**

- **Part 1, the requirement.** NPP all-India generation includes renewables: 189.5 TWh of the 1,698 TWh of FY2023-24 (`data/raw/npp/npp_national_generation_fy.csv`).
- **Holdout** (`outputs/holdout.csv`). NPP publishes conventional generation for FY2024-25 and FY2025-26 but no renewables, so renewables are extended at their FY2018-19 to FY2023-24 growth (14.1 percent a year):

| Year | NPP conventional | Renewables (extended) | Total | Model median (P5 to P95) | Error |
| --- | --- | --- | --- | --- | --- |
| FY2024-25 | 1,569 | 216 | 1,785 | 1,727 (1,632 to 1,827) | -3.3% |
| FY2025-26 | 1,530 | 247 | 1,776 | 1,786 (1,661 to 1,922) | +0.5% |

Both years fall inside the model's 90 percent range.

- **The NPP check** (`api/NPP_SERVICES.md`, section "Renewable generation by region or state: not on NPP", re-checked on 17 November 2026):
  - the 18-column and daily reports cover conventional stations above 25 MW and print "It excludes generation from RES";
  - no dashboard service returns renewable generation by region or state; the services that carry renewables by state give installed capacity, which the rules exclude;
  - Merit real-time solar and wind are all-India only and start on 18 September 2025.
- **Part 2, the regional base mix for FY2023-24** (D13; by region and source in `outputs/strategy_2047.csv`, column `base_fy2023_24_twh`): 1,734.4 TWh in total.
  - NPP regional conventional generation, 1,508.5 TWh (primary). Thermal is split into coal (1,294.9 TWh) and gas (31.7 TWh) by NPP's own station sums.
  - CEA General Review Table 4.7 state renewables, 225.8 TWh (secondary): solar 116.0, wind 83.4, small hydro 9.5 and bioenergy 17.0 TWh, each state placed in its NPP grid region.
  - The total is 36 TWh above NPP's 1,698 TWh because CEA's renewables total is 36.3 TWh above NPP's (189.5 TWh). NPP gives no regional or type split, so CEA's is used.
  - The midpoint base mix was Ember's CY2024 state data (1,801.2 TWh, secondary). It is now a cross-check only.
- **Renewables in 2047.** Solar and wind are bounded by published potentials (NISE 2025 ground-mounted solar, NIWE 150 m wind; secondary), and their hourly shapes for the storage check come from NPP Merit (332 complete days, 19 September 2025 to 13 November 2026; `outputs/storage_land_2047.csv`).
- **Installed capacity.** The Renewable India dataset's capacity columns never enter a calculation. They repeat their 2021 values in most states from 2022, and their 2024 solar total is about 1.3 times the MNRE figure ([data_audit.md](data_audit.md), known caveats). They serve only as a qualitative check on regional strengths.

**Sources.**

- Primary: NPP all-India generation chart (renewables to FY2023-24); NPP 18-column and daily reports (conventional generation by region and state); NPP Merit real-time data (hourly all-India solar and wind). Renewable India dataset (installed capacity, qualitative check only).
- Secondary: CEA General Review 2025, Table 4.7 (FY2023-24 renewable generation by state and type); Ember India state data (cross-check only); NISE, NIWE and CEA resource potentials (limits in the mix only).

## 4. How does the elasticity affect the result, and was it estimated state-wise, region-wise or for India as a whole?

**Short answer.**
The elasticity is estimated for India as a whole: 0.83 (s.e. 0.063), from NPP generation per person on real GSDP per person, FY2011-12 to FY2023-24.
It is then applied state by state, to each state's own income (question 2).
Each 0.01 of elasticity moves the FY2047-48 median by about 1 percent, so one standard error either way moves it by -6.4 or +6.9 percent.
A state-level regression on NPP state generation would measure where power plants stand, not what a state needs, so it is shown only as a check.

**Evidence.**

Estimates (`outputs/india_fit.csv`):

| Estimate | Value |
| --- | --- |
| FY2011-12 to FY2023-24 (one price base) | 0.828 (s.e. 0.063) |
| FY2006-07 to FY2023-24 (spliced price bases) | 0.866 (s.e. 0.031) |
| First differences (FY2011-12 window; spliced window) | 0.782; 0.787 |
| Used in the forecast: block-bootstrap mixture of both windows | median 0.850, 90% interval 0.731 to 0.939 |

Elasticity sensitivities, FY2047-48 (`outputs/sensitivity_2047.csv`):

| Case | P5 | Median | P95 | Median vs base |
| --- | --- | --- | --- | --- |
| Base | 3,217 | 4,703 | 7,002 | |
| Elasticity one OLS s.e. lower (-0.063) | 3,094 | 4,400 | 6,379 | -6.4% |
| Elasticity one OLS s.e. higher (+0.063) | 3,345 | 5,027 | 7,689 | +6.9% |
| FY2011-12 window only | 3,168 | 4,583 | 6,835 | -2.6% |
| FY2006-07 window only | 3,278 | 4,829 | 7,140 | +2.7% |
| First-difference estimate (shift -0.065) | 3,091 | 4,392 | 6,363 | -6.6% |
| Elasticity falls 0.2 per unit of ln income | 3,089 | 4,197 | 5,643 | -10.7% |

- Why about 1 percent per 0.01: on the central path, ln income per person rises by 1.05 (0.872 / 0.828, `outputs/growth_decomposition_2047.csv`), so each 0.01 of elasticity adds about 0.0105 to ln generation in FY2047-48.
- Every elasticity case moves the median by less than either income-growth case (+31 or -24 percent, question 1).

State-wise application (`outputs/state_income_2047.csv`):

- Each state's 2047 generation per person, relative to a person at India's income, is its relative income to the power 0.736. That is the elasticity 0.828 less gamma x kappa (1.0 x 0.092), the part of the urbanisation term that moves with income. It stays in the split because the dataset has no state urban share.
- A state 10 percent richer than India therefore needs about 7 percent more generation per person.

State panel check (`outputs/state_panel_check.csv`; diagnostic only, never used by the forecast):

- Regression: ln(NPP state generation per person) on ln(dataset GSDP per person), with state fixed effects, FY2018-19 to FY2023-24.
- Result: slope 0.434 (s.e. 0.242), 188 state-years, 32 states. Ladakh is skipped (not in the dataset) and Goa has no NPP generation.
- Why it misleads:
  - NPP state generation covers conventional stations only and is recorded in the state where the plant stands.
  - State generation per person runs from 0.08 times (Nagaland) to 15.9 times (Sikkim) the national 979 kWh. Chhattisgarh is at 4.7 times and Himachal Pradesh at 5.2 times; Delhi is at 0.26 times and Kerala at 0.20 times.
  - 97 percent of the variance of ln generation per person lies between states and is absorbed by the fixed effects. The slope rests on six years of movement within each state, which include the pandemic year and new plants.
  - Its standard error is almost four times the national one, and the national 0.828 lies within two of its standard errors. The s.e. is not clustered by state, so it is, if anything, too small.
- A region-wise estimate was rejected for the same reason: NPP regional generation includes inter-regional trade (the Western region generates 36 percent of India's conventional power and holds 26 percent of the 2047 requirement; `outputs/site_summary.json`, `regions`) and covers conventional sources only, from FY2018-19 (D10).

So the elasticity is estimated where generation and need coincide, India as a whole, and applied state by state.

**Sources.**

- Primary: NPP all-India generation (the estimate); NPP state generation, CEA 18-column Sub-report 16 (the panel check); Renewable India dataset, population and GSDP.
- Secondary: none in the estimate. The urbanisation prior (question 1) enters the projection, not the estimate.

## 5. What hurdles did you face, and how will you mitigate them?

**Short answer.**
The hurdles were the rules (two datasets for the core prediction, no installed capacity), gaps in the data (a short national series, no renewable generation by region, quirks in the GSDP series) and the width of the uncertainty.
Each has a mitigation in the model; what is left is stated as a known limit.

**Evidence.**

| # | Hurdle | Mitigation | Evidence |
| --- | --- | --- | --- |
| 1 | The core 2047 prediction must use only the two challenge datasets (kickoff, 13 Nov 2026) | India-only elasticity model on NPP and Renewable India data. The cross-country model is kept for justification only: 4,528 TWh, within 4 percent of our median | `outputs/core_comparison.csv`; D10, D12 |
| 2 | Only 18 annual national points | One parameter is estimated; a block bootstrap over two windows; two holdout years (-3.3 and +0.5 percent); rolling backtests (2.0 percent one year ahead, 6.4 percent five years ahead) | `outputs/india_fit.csv`, `holdout.csv`, `backtest_horizon.csv` |
| 3 | Past power shortages may inflate the elasticity | The later window carries half the weight; first differences give 0.78; the elasticity-decline setting gives -10.7 percent | `outputs/sensitivity_2047.csv` |
| 4 | GSDP quirks: a price-base change in 2011, values copied from the year before, incomplete 2024 values | Chained over matched states; the 2011 growth is imputed (4.63 percent per person); copied values and 2024 are left out | `outputs/india_fit.csv`, row `splice_growth`; `tests/test_india.cpp` |
| 5 | Neither dataset has an urban share | Only urbanisation beyond the income path enters, with a literature prior; the run without it is always shown (-5.3 percent) | `outputs/sensitivity_2047.csv` |
| 6 | NPP has no renewable generation after FY2023-24, and none by region or state | Base year FY2023-24; the holdout extends renewables; the regional base mix takes CEA renewables (secondary, labelled) | Question 3 |
| 7 | Regional and state generation measure plant location, not need | The requirement is split by drivers; the state panel check shows why; the transfer gap is reported | Question 4 |
| 8 | Installed capacity may not be used | Published GW figures are only converted to TWh with a stated capacity factor, as limits on generation; the dataset's capacity is a qualitative check | [data_audit.md](data_audit.md) |
| 9 | The dataset's six zones do not match NPP's five grid regions | NPP's grid regions; the organisers answered "Four, five, or six regions - either is fine" | D12 |
| 10 | The organisers' answer on published figures in the mix was "Use insights from this data source", which is not explicit | A clarification was sent on 17 Nov 2026; no reply has come yet. Following the kickoff guidance, published figures set limits and prices in the mix (Part 2) only, each labelled secondary. If the organisers rule that they may only justify the mix, the 2047 requirement and its regional split (Part 1) do not change | D12, D13 |
| 11 | The 2047 range is wide, mostly because of income growth | The plan is tested at the P90 requirement (6,400 TWh). The Robust plan leaves 633 TWh unmet there; with 45 percent of the solar potential usable it leaves none, at 1.0 times the published high-case build limits | `outputs/p90_levers.csv` |
| 12 | An annual plan cannot see hours | Hourly back-calculation from NPP Merit data: the Robust plan needs 448 GW and 3,044 GWh of storage; its gas runs 2,698 hours a year, 49 percent of its energy between 18:00 and 24:00 IST | `outputs/storage_land_2047.csv`, `site_summary.json` (`storage`) |
| 13 | Low-demand futures: at the Low growth scenario's P05 (2,332 TWh) the Robust policy has no feasible plan, because its coal floor (647 TWh) and the gas kept for peaking (51 TWh) are fixed in TWh while its non-fossil minimum is a share | Stated as a known limit; the dashboard explains it and names the levers. The Low carbon policy is feasible there (86.9 bn USD a year), and every policy is feasible at the Base scenario's P05 (3,217 TWh) and at every scenario's median | `outputs/presets.csv`; D13, known limits |
| 14 | Honest ranges need millions of scenarios | GPU Monte Carlo: 1,048,576 scenarios in 8.9 ms, against 2.1 s on 16 CPU threads; the same code runs in the browser | `outputs/timings.csv` |

**Before the final evaluation (21-22 November 2026):** answer the organisers' clarification if it arrives, and keep Part 2 as it is until then; rehearse these answers and the live dashboard; keep every new number traced to an output file.

**Sources.**

- Primary: NPP (all-India, regional and state generation; Merit hourly data); Renewable India dataset (population and GSDP).
- Secondary, all labelled: UN WPP 2024 and UN WUP 2018; the urbanisation literature; CEA General Review 2025; CEA NEP 2022-32, the CEA input slides (Dec 2024), NITI Aayog (2026), the DAE nuclear target and the MoP Annual Report (build limits and committed builds); CEA CO2 Baseline Database v22.0; IEA WEO 2025 and CEA/CERC costs; NISE, NIWE and CEA potentials; Ember and World Bank (cross-country check only).
