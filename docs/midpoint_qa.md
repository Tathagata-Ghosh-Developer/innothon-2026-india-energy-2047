# Midpoint review: questions and answers

There are 3 minutes for questions, so expect about five. Answer in 20 to 30 seconds: the short answer first, then one fact. Use the "If pressed" line only if the panel asks a follow-up.
Talk script: [midpoint_talk.md](midpoint_talk.md). Full list of jury questions: [team_plan.md](team_plan.md).

**Who answers.** The owner answers questions on their part of the talk. If the owner is unsure, Speaker 1 answers. Do not talk over each other.

**If we do not know.** Say so plainly: "We have not tested that yet; we will add it before the final." Never guess a number.

**Words to use.**
- Say "generation requirement" or "generation", never "demand" or "consumption".
- Lead with TWh; capacity in GW is not our forecast metric.
- "2047" means FY2047-48.

## Data and compliance (Speaker 1)

**1. Do you use any data beyond the two challenge datasets?**
The forecast is estimated only on NPP generation and the Renewable India state data. Three inputs are assumptions the organisers' ruling allows:
- the UN population path, which matches the dataset's state growth within 0.14 points a year;
- the urbanisation prior, which is our judgment;
- costs and resource limits, which are used only in the mix, not in the forecast.

*If pressed:* without the urbanisation prior, the run is purely the two datasets and gives 4,453 TWh, 5% lower.

**2. Do you use installed capacity or consumption anywhere?**
Not in the forecast. Resource potentials published in GW only cap generation in the mix, after an explicit conversion to TWh with capacity factors. We have asked the organisers to confirm this use (16 Nov).

**3. What about captive power?**
The organisers ruled that only NPP generation is used, so captive plants are neither estimated nor added. Our 4,703 TWh is India's utility generation in FY2047-48.

**4. Why five regions?**
NPP reports generation by its five grid regions, and the dataset's Zone column fills in any state NPP does not list. We asked the organisers on 16 Nov whether the five-region split is acceptable.

## Model (Speaker 2)

**5. Why a simple elasticity model and not machine learning?**
We have 18 years of annual national data. A model with one estimated slope can be tested and explained, while a flexible model would overfit that history. The test is the holdout: −3.3% and +0.5% on two years the model never saw.

**6. Is 0.83 too high? India had power shortages.**
It may be slightly high, because part of the early growth was catch-up as shortages closed. That is why:
- the later window, FY2011-12 to FY2023-24, carries half the weight;
- the dashboard has an elasticity-decline setting: a fall of 0.2 per unit of log income gives 4,197 TWh, 11% lower.

**7. How does urbanisation enter?**
Only as urbanisation beyond what income growth already implies, scaled by gamma. The datasets have no urban share, so gamma comes from published studies: centred at 1 with s.d. 0.5. Gamma = 0 gives only 5% less, so the result does not hinge on it.

**8. Your cross-country model gives 4,528 TWh. Why not use it?**
The organisers allow it for justification only, not estimation. It agrees within about 4% of our median. Across 153 countries the elasticity falls as income rises; India's own data show no decline yet.

## Validation and uncertainty (Speaker 3)

**9. How do you know the model works?**
- On two held-back years (FY2024-25, FY2025-26) the errors are −3.3% and +0.5%, both inside the 90% range.
- Rolling backtests from seven start years give average errors of 2% one year ahead and 6% five years ahead.

**10. Why is the range so wide?**
Income growth: 1.5 points a year more or less moves 2047 by +31% or −24%. Elasticity, urbanisation and population each move it by 11% or less. The range is honest; planning against only the median would hide it.

**11. What about EVs, data centres, green hydrogen or air conditioning?**
The elasticity already carries the structural change of the past 18 years. Genuinely new loads can be added as an extra TWh range in the dashboard. Our P90, 6,400 TWh, is 1,700 TWh above the median, which leaves room for them.

*If pressed:* we have not modelled each new load separately yet. This is planned with the industrial layer before the final.

## Regions and the mix (Speaker 4)

**12. Why is there no gas in your mix?**
At IEA's 2047 cost projections for India, gas generation costs more per MWh than coal or renewables, so a least-cost annual energy plan does not choose it. The value of gas is flexibility in peak hours, which an annual model cannot price. Our hourly check can run gas in the highest remaining hours, but the current least-cost plan has no gas, so storage covers those hours. A gas-for-peaking case is a natural test for the final.

*Do not say* gas has no future. Say its role is peaking and flexibility.

**13. Why is coal still 22% in 2047?**
India's coal fleet is young, so the plan keeps at least half of 2024 coal output in each region (the coal floor).
- A CO2 cap brings coal down to 14% and CO2 from 993 to 643 Mt, for only 0.65 bn USD a year more, because nuclear replaces coal.
- The coal that remains sits at large stations, which suits carbon capture.

**14. Nuclear is only 1.2% in your Robust plan. Why?**
It is cost. In the base plan, with less land, nuclear reaches its 10% limit. In the Robust plan more land is usable, and solar with firming is cheaper per MWh. Nuclear is the next step when CO2 is capped: under the cap its share rises to 8.8%.

**15. How do you handle solar intermittency?**
Firmed renewables (solar and wind with storage, round-the-clock) are 14.6% of the Robust plan. We also back-calculate total storage from NPP's hourly Merit profiles: about 3,000 GWh, around 6.8 hours. Land for the plan is 19,990 km², 0.61% of India.

**16. Where do costs and limits come from?**
- Costs: IEA WEO 2025 India projections to 2047, and CEA/MNRE tariffs.
- Resource potentials: MNRE, NIWE and CEA.
Every number has a source row in the repository.

**17. What about transmission?**
The plan has inter-regional corridors with losses. The Robust plan allows three times the 2032 corridor plan. In the base plan the West to East corridor runs full.

## Engine and next steps (Speaker 5)

**18. Why do you need a GPU?**
The uncertainty is millions of independent futures. The GPU runs a million in 7.7 ms, against 2.21 s on 16 CPU threads. So the dashboard recomputes live when someone changes an assumption. CPU and GPU results agree within 0.2%, checked by an automated parity test.

**19. What is left before the final?**
- Fold in the organisers' answers.
- Check the plan hour by hour with NPP's Merit profiles.
- Add an industrial demand layer for the steel and aluminium regions.
- Rehearse the live demo.

**20. What is your biggest risk?**
The size of 2047 depends mostly on income growth, which nobody can forecast precisely. So we give a range and a plan tested at the 90th percentile, not one number.

**21. Can we see the dashboard?**
Yes: india-energy-2047.vercel.app. A demo is not required today, but we can share it if the panel wants.
