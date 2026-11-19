# Final evaluation: questions and answers

This file began as the midpoint Q&A (18 Nov 2026); it now prepares the final evaluation (21-22 Nov 2026) and carries the current numbers ([decisions.md](decisions.md), D13).
The midpoint panel's five questions are answered in full, with evidence and sources, in [midpoint_panel_answers.md](midpoint_panel_answers.md); section 0 below gives the short versions.
Talk script of the midpoint: [midpoint_talk.md](midpoint_talk.md) (a record; its numbers are the midpoint's). Work plan and the jury list: [team_plan.md](team_plan.md).

Answer in 20 to 30 seconds: the short answer first, then one fact. Use the "If pressed" line only if the jury asks a follow-up.

**Who answers.** The owner answers questions on their part of the talk. If the owner is unsure, Speaker 1 answers. Do not talk over each other.

**If we do not know.** Say so plainly: "We have not tested that; we can show how it would enter." Never guess a number.

**Words to use.**

- Say "generation requirement" or "generation", never "demand" or "consumption".
- Lead with TWh; capacity in GW is not our forecast metric.
- "2047" means FY2047-48.
- Our primary sources are NPP and the Renewable India dataset; call every other source secondary.
- On the organisers' answer about published figures in the mix, quote their words, "Use insights from this data source", and say a clarification has been requested. Never say it is confirmed.

## 0. The midpoint panel's questions (short versions)

**P1. Is income growth the only factor?**
No. On the central path, income accounts for 87 percent of the growth to 2047, population for 15 percent and urbanisation beyond income for 5 percent; the fading of the high FY2023-24 level takes back 7 percent. New loads absent from the history, such as green hydrogen (418 to 1,237 TWh), are a separate scenario: 5,537 TWh, never the headline.

**P2. State-wise, region-wise or national income?**
State-wise: the GSDP and population of 34 states and union territories in the Renewable India dataset. National growth is chained over the states, and the split applies the income response to each state's own income, with convergence. Example: Uttar Pradesh holds 17.6 percent of the population in 2047 and 12.4 percent of the requirement; Maharashtra 8.8 and 10.6 percent.

**P3. Renewable data is missing. How did you manage?**
Renewables are inside NPP's all-India generation up to FY2023-24, our base year. NPP has none by region or state (we checked every NPP service), so the base mix takes regional renewables from the CEA General Review, labelled secondary. The dataset's installed capacity is only a qualitative check.

**P4. How does the elasticity affect the result, and at what level?**
Estimated for India as a whole: 0.83, s.e. 0.063. Each 0.01 moves 2047 by about 1 percent; one standard error moves it by -6.4 or +6.9 percent. A state panel gives 0.43 (s.e. 0.24), but NPP state generation records where plants stand (Sikkim at 15.9 times the national figure per person, Nagaland at 0.08), so it is a check only.

**P5. Hurdles and mitigation?**
The two-dataset rule (an India-only model), a short series (one estimated parameter, a holdout), no regional renewable data (CEA, labelled), plant location in regional data (requirement from drivers), and an unclear answer on published figures in the mix (a clarification sent; Part 1 does not depend on it). The table is in [midpoint_panel_answers.md](midpoint_panel_answers.md), question 5.

## Data and compliance (Speaker 1)

**1. Do you use any data beyond the two challenge datasets?**
Our primary, trusted sources are the two the organisers gave us: the National Power Portal and the Renewable India dataset. Everything else is secondary and labelled.
The 2047 requirement is estimated only on NPP generation and the Renewable India state data. Two inputs are assumptions the organisers' ruling allows:

- the UN population path, which matches the dataset's state growth within 0.14 points a year;
- the urbanisation prior, which is our judgment.

Published build limits, committed builds, costs, emission factors and resource limits are used only to choose the generation mix, never in the 2047 requirement (see question 2a).

*If pressed:* without the urbanisation prior, the run is purely the two datasets and gives 4,453 TWh, 5% lower.

**2. Do you use installed capacity or consumption anywhere?**
Not as a forecast metric. The Renewable India dataset's installed capacity never enters any calculation; it is only a qualitative check on regional strengths. Published figures in GW (resource potentials, build additions, committed reactors and hydro plants) only bound generation in the mix, after an explicit conversion to TWh with capacity factors.

**2a. Is it within the rules to use published figures for the mix?**

- At the kickoff, the organisers said publicly available literature may be used for the growth rates and constraints of each energy source; only the 2047 energy prediction must use the two datasets.
- The two datasets have no costs, emission factors, build plans or resource potentials, and their installed capacity may not be used. So a mix justified by cost, energy security and CO2 needs published figures.
- They decide only the mix (Part 2); the 2047 requirement (Part 1) uses none of them.
- Their written reply on this point was "Use insights from this data source". We sent a clarification on 17 Nov; no reply has come yet.

*If pressed:* if the organisers rule that these figures may only justify the mix, the 2047 requirement and the regional split do not change; only the way the mix is chosen does.

**3. What about captive power?**
The organisers ruled that only NPP generation is used, so captive plants are neither estimated nor added. Our 4,703 TWh is India's utility generation in FY2047-48.

**4. Why five regions?**
NPP reports generation by its five grid regions, and the dataset's Zone column fills in any state NPP does not list. The organisers answered in writing: "Four, five, or six regions - either is fine".

**4a. Why is your base mix 1,734 TWh when NPP says 1,698 TWh?**
NPP gives conventional generation by region (1,508.5 TWh, which we use) but renewables only for all India. CEA's state renewables (225.8 TWh) fill the regional gap, and they are 36 TWh above NPP's renewable total. The requirement forecast uses NPP's 1,698 TWh.

## Model (Speaker 2)

**5. Why a simple elasticity model and not machine learning?**
We have 18 years of annual national data. A model with one estimated slope can be tested and explained, while a flexible model would overfit that history. The test is the holdout: −3.3% and +0.5% on two years the model never saw.

**6. Is 0.83 too high? India had power shortages.**
It may be slightly high, because part of the early growth was catch-up as shortages closed. That is why:

- the later window, FY2011-12 to FY2023-24, carries half the weight;
- the first-difference estimate (0.78) gives 4,392 TWh, 6.6% lower;
- the dashboard has an elasticity-decline setting: a fall of 0.2 per unit of log income gives 4,197 TWh, 11% lower.

**7. How does urbanisation enter?**
Only as urbanisation beyond what income growth already implies, scaled by gamma. The datasets have no urban share, so gamma comes from published studies: centred at 1 with s.d. 0.5. It accounts for 5% of the growth to 2047, and gamma = 0 gives only 5% less, so the result does not hinge on it.

**8. Your cross-country model gives 4,528 TWh. Why not use it?**
The organisers allow it for justification only, not estimation. It agrees within about 4% of our median. Across 153 countries the elasticity falls as income rises; India's own data show no decline yet.

**8a. Why not estimate the elasticity state by state?**
NPP state generation records where plants stand, not what a state needs: Chhattisgarh generates 4.7 times the national figure per person, Delhi 0.26 times. With state fixed effects, 97% of the variance is absorbed, and the slope (0.434, s.e. 0.242) rests on six years of within-state movement. So we estimate nationally, where generation and need coincide, and apply the elasticity to each state's own income.

## Validation and uncertainty (Speaker 3)

**9. How do you know the model works?**

- On two held-back years (FY2024-25, FY2025-26) the errors are −3.3% and +0.5%, both inside the 90% range.
- Rolling backtests from seven start years give average errors of 2% one year ahead and 6% five years ahead.

**10. Why is the range so wide?**
Income growth: 1.5 points a year more or less moves 2047 by +31% or −24%. Elasticity, urbanisation and population each move it by 11% or less. The range is honest; planning against only the median would hide it.

**11. What about EVs, data centres, green hydrogen or air conditioning?**
We report published estimates for five new loads (`outputs/new_loads_2047.csv`). Only green hydrogen is absent from the history, so only it is added, in a separate scenario: 418 to 1,237 TWh in 2047, median 5,537 TWh (+17.7%). Electric vehicles, data centres, cooling and cooking are reported but not added, because the income elasticity already carries part of them.

*If pressed:* our P90, 6,400 TWh, is 1,697 TWh above the median, and the plan is tested against it.

**11a. How does your forecast compare with CEA's?**
Against CEA's 20th Electric Power Survey our median is close:

- 2,382 TWh in FY2031-32 against 2,474 (−4%);
- 3,020 in FY2036-37 against 3,096 (−2%);
- 3,745 in FY2041-42 against 3,776 (−1%).

CEA has published no 2047 figure. Our 4,703 TWh is inside the 4,100 to 4,800 TWh band that comes from extending CEA's own growth.

**11b. CEA's 2026 review gives about 3,365 BU for 2035-36. You are about 20% lower. Why?**
Our median for FY2035-36 is 2,884 TWh. CEA's 3,596 BU of gross generation sits at our 95th percentile (3,634), so it is inside our range, at the high end.

- CEA's surveys have run high before: the 18th survey forecast 1,905 BU for 2021-22 against an actual 1,376 (38% high), and the 19th was 12 to 14% high by 2020-22.
- Our scope is NPP utility generation, without captive plants.
- Our path is calibrated on actual NPP generation up to FY2025-26.

*If pressed:* the 2026 review figures come from press reports; the CEA document itself is not public yet.

**11c. Your 5th percentile (3,217 TWh) is below every published 2047 figure. Is that credible?**
Published studies report central cases, not tails. Our 5th percentile is the slow-growth future: income per person growing 1.5 points a year slower than the central path gives about 24% less. We show it so the plan is tested against both ends, not because we expect it.

## Regions and the mix (Speaker 4)

**12. What role does gas play?**
Peaking and a bridge for renewables. The Robust plan keeps the existing gas fleet at its best NPP year, 51 TWh (1.1% of 2047 generation), each region within its FY2023-24 share: the West has the most (20 TWh), then the North-East and the North. In the hourly check it runs 2,698 hours a year, at most 21 GW, and 49% of its energy falls between 18:00 and 24:00 IST.

*If pressed:* published plans add no new gas, so we do not build more; at IEA's 2047 costs, gas energy is dearer than coal or renewables.

**13. Why is coal still 19% in 2047?**
India's coal fleet is young, so the Robust plan keeps at least half of FY2023-24 coal output in each region (the coal floor, 647 TWh).

- A CO2 cap brings coal down to 13.7% and CO2 from 912 to 653 Mt, for only 0.46 bn USD a year more, because nuclear replaces coal.
- The coal that remains sits at large stations, which suits carbon capture.

**14. Nuclear is only 2.8% in your Robust plan. Why?**
It is cost. The Robust plan builds exactly the committed reactors, 14,300 MW under construction or sanctioned (133 TWh), and no more, because solar with firming is cheaper per MWh when more land is usable. In the base plan, with less land, nuclear reaches its 10% limit. Under a CO2 cap nuclear is the next step: its share rises to 8.5%.

**15. How do you handle solar intermittency?**
Firmed renewables (solar and wind with storage, round-the-clock) are 14.2% of the Robust plan, and gas is kept for the evening peak. We also back-calculate total storage from NPP's hourly Merit profiles: about 3,044 GWh, around 6.8 hours, 448 GW. Land for solar and wind is 19,990 km², 0.61% of India.

**16. Where do costs and limits come from?**

- Base mix: NPP conventional generation by region (primary) plus CEA renewables by state (secondary).
- Build limits: published additions to 2047 from CEA NEP 2022-32, the CEA input slides (Dec 2024), NITI Aayog (2026), the DAE nuclear target and the MoP hydro schedule. The Robust plan builds at 1.0 times the published high case.
- Committed builds: nuclear units under construction or sanctioned; large hydro under construction.
- Costs: IEA WEO 2025 India projections to 2047. A CEA cost basis is a sensitivity; it applies nominal CERC rates to real 2021-22 capital costs, so it shows the direction of the mix, not a comparable cost level.
- Emission factors: CEA CO2 Baseline Database v22.0 (coal 0.971, gas 0.470 tCO2/MWh).
- Resource potentials: NISE, NIWE and CEA; NISE 2014 and NIWE 120 m are sensitivities.

Every number has a source row in the repository.

**16a. What if the published figures are different?**
`outputs/sensitivity_mix.csv`: on the central-case build limits the Robust plan still meets the median (703 TWh unmet at P90). With the low-case limits or the older NISE 2014 solar potential it cannot meet the median (1,555 or 1,418 TWh unmet), so those are not cheaper plans, only smaller ones. With 120 m wind the P90 shortfall rises to 1,044 TWh.

**17. What about transmission?**
The plan has inter-regional corridors with losses. The Robust plan allows three times the 2032 corridor plan; the West exports 299 TWh and the South 125 TWh, while the East imports 262 TWh and the North 153 TWh. In the base plan the West to East corridor runs full.

**17a. Is there any case where your plan fails?**
Yes, one we state openly: at the Low growth scenario's 5th percentile (2,332 TWh), the Robust policy has no feasible plan, because the coal floor and the gas kept for peaking are fixed in TWh while the non-fossil minimum is a share. The dashboard says so and names the levers. Every policy is feasible at the Base scenario's 5th percentile (3,217 TWh) and at every scenario's median.

## Engine and next steps (Speaker 5)

**18. Why do you need a GPU?**
The uncertainty is millions of independent futures. The GPU runs a million in about 9 ms (8.9 ms in the latest run), against 2.1 s on 16 CPU threads. So the dashboard recomputes live when someone changes an assumption. CPU and GPU results agree within 0.2%, checked by an automated parity test.

**19. What would you do next?**

- Act on the organisers' reply on published figures in the mix when it comes.
- An hourly capacity-expansion model, so storage and gas are chosen inside the plan, not checked after it.
- An industrial layer for the steel and aluminium regions.

**20. What is your biggest risk?**
The size of 2047 depends mostly on income growth, which nobody can forecast precisely. So we give a range and a plan tested at the 90th percentile, not one number.

**21. Can we see the dashboard?**
Yes: india-energy-2047.vercel.app. The data tab has two evidence views: "Requirement evidence" (growth decomposition, state table, state panel check, new loads) and "Mix evidence" (mix sensitivities, committed builds, gas hours).
