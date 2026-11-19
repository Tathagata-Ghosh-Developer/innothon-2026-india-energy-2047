# Team work plan

Status on 2026-11-19:

- The primary forecast uses only the two challenge datasets (National Power Portal and the Renewable India dataset); the cross-country model is an independent check used for justification only ([decisions.md](decisions.md), D10 and D12).
- The organisers' written ruling of 16 Nov is applied: no captive layer, the regional split from the challenge data, the cross-country model and resource potentials for justification only, and the headline in total TWh ([decisions.md](decisions.md), D12).
- The midpoint review (18 Nov, 6:00 PM slot) is done. The panel's five questions are answered with evidence in [midpoint_panel_answers.md](midpoint_panel_answers.md); the work behind the answers is [decisions.md](decisions.md), D13.
- Since the midpoint (D13): growth decomposition, elasticity sensitivities, a state-by-state income table, a state panel check and a new-loads scenario (Part 1 evidence; the 2047 requirement is unchanged); in Part 2, a base mix from NPP plus CEA renewables, published build limits, committed nuclear and hydro builds, the gas fleet kept for peaking, CEA v22 emission factors, a CEA cost basis and mix sensitivities.
- The forecast, regional strategy, dashboard (native and WebAssembly) and documentation work end to end.

The plan below runs to the final evaluation.

## Milestones

| Date | Milestone | What must be ready |
| --- | --- | --- |
| 16-17 Nov | Written queries to the organisers | Sent; answered (below) |
| 18 Nov | Midpoint review (done) | One slide ([beamer/midpoint.pdf](beamer/midpoint.pdf)); talk script [midpoint_talk.md](midpoint_talk.md). The panel's questions are answered in [midpoint_panel_answers.md](midpoint_panel_answers.md) |
| 19-20 Nov | Final-evaluation preparation | Docs, decks, dashboard text and landing page refreshed to the D13 results; live site redeployed; Q&A rehearsed ([midpoint_qa.md](midpoint_qa.md)) |
| 21-22 Nov | Final evaluation | Defensibility deck, live dashboard demo (deployed URL), recorded backup video, code in the shared GitHub repository |
| 30 Nov | Prize ceremony | |

## Written query to the organisers (sent; answered on 16 Nov)

1. May external population and urban-share projections (UN WPP 2024, UN WUP 2018, Technical Group) be used as stated driver assumptions in the core forecast?
   Answer: yes, if reasonably consistent with the state-level population growth in the challenge dataset; UN India-level growth is a reference. Checked in `outputs/population_consistency.csv`.
2. May a published urbanisation coefficient (Liddle 2013; Shahbaz et al. 2016) enter the core model, if the run without it (two datasets only) is always shown beside it?
   Answer: the coefficient is the team's own judgment.
3. NPP utility generation excludes captive plants. May captive generation (CEA General Review) be shown as a separately labelled layer next to the forecast?
   Answer: no; NPP generation only, no external data for estimation. The layer is removed.
4. May a cross-country model on non-challenge data (Ember, World Bank) appear as a labelled independent check?
   Answer: for justification only, not estimation.
5. May we split the national generation forecast across regions using population, GSDP and urbanisation, without regional consumption data?
   Answer: the regional split must use the challenge dataset. It now uses the dataset's state populations and GSDP, with NPP's grid regions, and no state urban term.
6. May published resource potentials (GW) be converted to generation potential (TWh per year) with capacity factors, used only as upper bounds in the supply plan?
   Answer: for justification only, not estimation.

The organisers also asked for the headline as the country's total in TWh, not per person.

## Follow-up questions (sent 16 Nov; answered)

- (a) Generation only, not consumption? Answer: "Generation data". We already use NPP generation only.
- (b) May resource potentials, costs and emission factors serve as limits and prices inside the mix optimisation? Answer: "Use insights from this data source". This is not explicit, so a clarification was sent on 17 Nov (whether the two-dataset rule covers the mix at all, and whether published figures may decide the mix or only justify it). No reply has come yet. Until then, following the kickoff guidance, published figures set the limits and prices of the mix (Part 2), each labelled secondary, and never enter the requirement (Part 1).
- (c) Is the NPP five-region split acceptable, given that the dataset's Zone column has six zones (including Central) and the kickoff mentioned four regional blocks? Answer: "Four, five, or six regions - either is fine". We keep NPP's five regions.

Answers (a) and (c) need no model change; (b) awaits the clarification ([decisions.md](decisions.md), D12).
If the reply says published figures may decide the mix, the docs, decks and Q&A change "a clarification has been requested" to the organisers' words. If it says they may only justify the mix, Part 2 must be rebuilt from challenge-data insights with the published figures beside it; Part 1 does not change.

## Workstreams to the final evaluation

| Workstream | Owner | Tasks before 21 Nov |
| --- | --- | --- |
| Forecast and validation | (assign) | Rehearse the panel answers on income, elasticity and new loads (questions 1, 2 and 4 of [midpoint_panel_answers.md](midpoint_panel_answers.md)) |
| Regional strategy | (assign) | Rehearse the base mix, published build limits, committed builds and gas for peaking; know the Low growth P05 limit and its levers |
| Data audit and compliance | (assign) | Walk through [data_audit.md](data_audit.md): the source hierarchy, Part 1 vs Part 2, no consumption or capacity data as forecast metrics |
| Dashboard and demo | (assign) | Open the redeployed WebAssembly dashboard on two machines; rehearse the demo, including the "Requirement evidence" and "Mix evidence" views; record a backup video |
| Deck and story | (assign) | Defensibility deck with the panel answers and likely questions ([midpoint_qa.md](midpoint_qa.md)) |

## Likely jury questions (prepare answers)

The full list, with short answers, is [midpoint_qa.md](midpoint_qa.md).

1. **Is the forecast compliant with the two-dataset rule?**
   Yes for the core: generation comes from the National Power Portal, population and real GSDP from the Renewable India dataset.
   Population and urban projections are driver assumptions the organisers' ruling allows, and the population paths are consistent with the dataset's state growth (every state within 0.14 points a year to 2030); the urbanisation coefficient is our judgment, based on published studies, and the run without it (two datasets only) gives 4,453 TWh, 5 percent below the forecast.
   The regional split uses only the challenge data; the cross-country model is shown only as an independent check, for justification.
   Published figures enter only the generation mix (Part 2), as limits and prices, labelled secondary.
2. **Is income the only factor?**
   No. On the central path to FY2047-48, income accounts for 87 percent of the growth, population for 15 percent and urbanisation beyond income for 5 percent; the fading of the high FY2023-24 level takes back 7 percent (`outputs/growth_decomposition_2047.csv`).
   New loads absent from the history (green hydrogen, 418 to 1,237 TWh) are a separate scenario: 5,537 TWh, +17.7 percent, never the headline.
3. **Is income used state-wise?**
   Yes: the GSDP and population of 34 states and union territories in the Renewable India dataset. National growth is chained over the states, and the regional split applies the income response state by state with convergence (`outputs/state_income_2047.csv`; for example Uttar Pradesh holds 17.6 percent of the population in 2047 and 12.4 percent of the requirement).
4. **Why an elasticity of 0.83, and why for India as a whole?**
   It is India's own: generation per person grew 0.83 percent for every 1 percent of real income per person from FY2011-12 to FY2023-24 (s.e. 0.063).
   It holds in growth rates too (0.78), so it is not a spurious trend fit, and it is between 0.78 and 0.87 in every window and form; one standard error either way moves 2047 by -6.4 or +6.9 percent.
   A state panel on NPP state generation gives 0.434 (s.e. 0.242), but NPP records generation where the plant stands (Sikkim at 15.9 times the national figure per person, Nagaland at 0.08), so it is a diagnostic only.
5. **How good is the model on data it has not seen?**
   FY2024-25 and FY2025-26 were not used: errors -3.3 and +0.5 percent, both inside the model's 90 percent range.
   Rolling backtests from seven origins: 2 percent one year ahead, 6 percent five years ahead.
   The model carries its own error: a persistent residual that starts at FY2023-24 (+0.070 log points) and reverts to the fitted relation.
6. **Why is the 2047 range so wide?**
   Mostly income growth: 1.5 points more or less growth a year moves the 2047 median by +31 or -24 percent.
   Elasticity, urbanisation and population each move it by 0 to 11 percent.
7. **How did you manage the missing renewable data?**
   Renewables are inside NPP's all-India generation up to FY2023-24, the base year; the holdout extends them at their recent growth.
   NPP has no renewable generation by region or state (every NPP service was checked; `api/NPP_SERVICES.md`), so the base mix takes regional renewables from the CEA General Review (secondary, labelled). The dataset's installed capacity is a qualitative check only.
8. **Is any capacity or consumption data used?**
   No capacity or consumption data enters the forecast.
   Published GW figures (potentials, build limits, committed units) only bound generation after an explicit capacity-factor conversion.
   See [data_audit.md](data_audit.md).
9. **Where is power needed versus generated?**
   The Western region generates 36 percent of India's conventional power but accounts for 26 percent of the 2047 requirement; the Northern, Southern and North-Eastern regions need more than they produce, so corridors must grow.
   In the Robust plan the West exports 299 TWh and the South 125 TWh; the East imports 262 TWh and the North 153 TWh.
10. **What is the binding constraint?**
    Land for solar and wind, not money or build rate.
    Both the base and the Robust policy meet the median; there the Robust policy costs 38 bn USD a year less and emits 462 Mt less CO2 (177.5 bn USD, 912 Mt). At P90 it leaves 633 TWh unmet instead of 1,463 and costs 253 against 236 bn USD a year.
    The P90 (6,400 TWh) needs about 45 percent of the solar potential usable (rooftop, agri-PV); at the published high-case build limits that is enough, while building at twice those limits without more land still leaves 219 TWh unmet.
11. **Can CO2 fall further without losing adequacy?**
    Yes, cheaply: a CO2 cap takes the Robust plan from 912 to 653 Mt a year (28 percent less) for 0.46 bn USD a year more; the marginal abatement cost stays at or below 4.0 USD per tonne, as nuclear replaces coal.
    It stops at 653 Mt because coal reaches its floor, half of its FY2023-24 output in every region (a young fleet), and the gas kept for peaking stays.
12. **What does the plan not cover?**
    At the Low growth scenario's 5th percentile (2,332 TWh) the Robust policy has no feasible plan, because the coal floor (647 TWh) and the gas kept for peaking (51 TWh) exceed its fossil allowance there; the dashboard explains this and names the levers. Every policy is feasible at the Base scenario's 5th percentile and at every median.
13. **Why use a GPU?**
    The uncertainty analysis is millions of independent scenarios.
    The GPU runs a million in 7.75 ms, against 4.15 s on 16 CPU threads (`outputs/timings.csv`; timings vary by run and machine load), so the dashboard recomputes live as the jury changes assumptions.
    The CPU and WebAssembly paths give the same results, checked by the parity test.
