# Team work plan

Status on 2026-11-16:

- The primary forecast now uses only the two challenge datasets (National Power Portal and the Renewable India dataset); the cross-country model is an independent check ([decisions.md](decisions.md), D10).
- The organisers answered the written query on 16 Nov; the ruling is applied: no captive layer, the regional split from the challenge data, the cross-country model and resource potentials for justification only, and the headline in total TWh ([decisions.md](decisions.md), D12).
- The forecast, regional strategy, dashboard (native and WebAssembly) and documentation work end to end.

The plan below runs to the final evaluation.

## Milestones

| Date | Milestone | What must be ready |
| --- | --- | --- |
| 16-17 Nov, 18:00-20:00 | Office hours | Send the written query below; get feedback on the method |
| 18 Nov | Midpoint review (regional model review, sensitivity checks) | One slide: approach, findings, roadblocks ([beamer/midpoint.pdf](beamer/midpoint.pdf)) |
| 21-22 Nov | Final evaluation | Defensibility deck, live dashboard demo (deployed URL), recorded backup video |
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

## Follow-up questions (sent 16 Nov; answers awaited)

- (a) Generation only or also consumption data: whether the analysis must rest on generation data alone, or whether consumption data may be used anywhere.
- (b) Whether resource potentials, costs and emission factors may serve as limits and prices inside the mix optimisation.
- (c) NPP's five grid regions or the challenge dataset's six zones (four blocks) for the regional results.

## Workstreams

| Workstream | Owner | Tasks before 18 Nov | Tasks before 21 Nov |
| --- | --- | --- | --- |
| Forecast and validation | (assign) | Rehearse the elasticity, holdout and sensitivity answers below | Hourly solar and wind profiles from NPP Merit data for storage sizing |
| Regional strategy | (assign) | Review the Robust policy levers and their sources | P90 hedge: land (rooftop, agri-PV) and build-rate narrative |
| Data audit and compliance | (assign) | Walk through [data_audit.md](data_audit.md); confirm no consumption or capacity data in forecasting | Final provenance appendix for the deck |
| Dashboard and demo | (assign) | Open the deployed WebAssembly dashboard on two machines | Rehearse the live demo over a screen share; record a backup video |
| Deck and story | (assign) | Midpoint slide | Defensibility deck with likely questions and answers |

## Likely jury questions (prepare answers)

1. **Is the forecast compliant with the two-dataset rule?**
   Yes for the core: generation comes from the National Power Portal, population and real GSDP from the Renewable India dataset.
   Population and urban projections are driver assumptions the organisers' ruling allows, and the population paths are consistent with the dataset's state growth (every state within 0.14 points a year to 2030); the urbanisation coefficient is our judgment, based on published studies, and the run without it (two datasets only) gives 4,453 TWh, 5 percent below the forecast.
   The regional split uses only the challenge data; the cross-country model is shown only as an independent check, for justification.
2. **Why an elasticity of 0.83?**
   It is India's own: generation per person grew 0.83 percent for every 1 percent of real income per person from FY2011-12 to FY2023-24.
   It holds in growth rates too (0.78), so it is not a spurious trend fit, and it is between 0.78 and 0.87 in every window and form.
   Part of the early growth was catch-up as power shortages closed, which is why the later window carries half the weight and an elasticity-decline setting is in the dashboard.
3. **How good is the model on data it has not seen?**
   FY2024-25 and FY2025-26 were not used: errors -3.3 and +0.5 percent, both inside the model's 90 percent range.
   Rolling backtests from seven origins: 2 percent one year ahead, 6 percent five years ahead.
   The model carries its own error: a persistent residual that starts at FY2023-24 (+0.070 log points) and reverts to the fitted relation.
4. **Why is the 2047 range so wide?**
   Mostly income growth: 1.5 points more or less growth a year moves the 2047 median by +31 or -24 percent.
   Elasticity, urbanisation and population each move it by 0 to 11 percent.
5. **Is any capacity or consumption data used?**
   No capacity or consumption data enters the forecast.
   Resource potentials in GW only cap generation after an explicit capacity-factor conversion.
   See [data_audit.md](data_audit.md).
6. **What about captive power?**
   NPP covers utilities, and the organisers ruled that only NPP generation is used, so captive plants are neither estimated nor reported.
   The forecast is the country's total utility generation: 4,703 TWh in FY2047-48, 3,005 TWh more than in FY2023-24.
7. **Why does the cross-country model give less (4,528 TWh, total generation in calendar 2047)?**
   Across 153 countries the elasticity falls with income (0.44 to 0.57 at India's income); India's own data show no decline yet.
   An elasticity decline of 0.2 per unit of ln income brings the India model to 4,197 TWh.
   The cross-country model is used for justification only, not for estimation.
8. **Where is power needed versus generated?**
   The Western region generates 36 percent of India's conventional power but accounts for 26 percent of the 2047 requirement; the Northern, Southern and North-Eastern regions need more than they produce, so corridors must grow.
   The split uses the challenge dataset's state populations and NPP's grid regions.
9. **What is the binding constraint?**
   Land for solar and wind, not money or build rate.
   Both the base and the Robust policy meet the median; the Robust policy costs 37 bn USD a year less and emits 378 Mt less CO2 (176 bn USD, 993 Mt).
   The P90 (6,400 TWh) needs about 45 percent of the solar potential usable (rooftop, agri-PV); at the Robust build rate (2.5 times India's record year) that is enough, while building four times faster without more land still leaves 240 TWh unmet.
10. **Why use a GPU?**
    The uncertainty analysis is millions of independent scenarios.
    The GPU runs 4 million in about 35 ms, so the dashboard recomputes live as the jury changes assumptions.
    The CPU and WebAssembly paths give the same results, checked by the parity test.
11. **Can CO2 fall further without losing adequacy?**
    Yes, cheaply: a CO2 cap takes the Robust plan from 993 to 643 Mt a year (35 percent less) for 0.65 bn USD a year more; the marginal abatement cost stays at or below 6.3 USD per tonne, as nuclear replaces coal.
    It stops at 643 Mt because coal reaches its floor, half of its 2024 output in every region (a young fleet).
