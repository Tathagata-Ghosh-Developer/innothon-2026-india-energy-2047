# Midpoint review: talk script

Wednesday 18 November 2026, 6:00 to 6:15 PM, Zoom. Team "The Losers".
Format: one slide ([beamer/midpoint.pdf](beamer/midpoint.pdf)), 10 minutes of talk, 3 minutes of questions.
Questions and answers: [midpoint_qa.md](midpoint_qa.md).

## Before the call

- Zoom display name: "Your Name - The Losers" (the organisers asked for the team name).
- Join by 5:50 PM. We are the first slot at 6:00.
- One person shares the slide (Speaker 1), full screen. A second person has the PDF open, ready to share if the first connection drops.
- The sharer moves the cursor to the part of the slide each speaker is talking about.
- Mute when not speaking. Each speaker ends with the hand-over line, so there is no gap.
- Say "terawatt-hours" in full. Say "twenty forty-seven" for FY2047-48.

## Speakers and timing

Swap names freely; the script does not depend on who speaks.

| # | Speaker | Part of the slide | Time | Ends at |
| --- | --- | --- | --- | --- |
| 1 | Tathagata Ghosh | Problem and the headline number | 1:45 | 1:45 |
| 2 | Sai Anirudh Guttapati | Left column: Approach | 2:00 | 3:45 |
| 3 | Devendra Umbrajkar | Findings: holdout and what drives the range | 1:45 | 5:30 |
| 4 | Ayush Kumar | Findings: regions, and the generation mix | 2:00 | 7:30 |
| 5 | Gaurav Choudhary | Right column: roadblocks, questions asked, next steps | 1:45 | 9:15 |

That leaves 45 seconds of margin. If you are running long, drop the sentences marked *(cut if short)*.

## 1. Problem and the headline number (Speaker 1, 1:45)

*Cursor: the title, then the big 4,703 in the middle column.*

Good evening. We are team The Losers from IISc Bengaluru: Tathagata, Sai Anirudh, Devendra, Ayush and Gaurav.

Our question is how much electricity India must generate in 2047, region by region, and which mix of sources should produce it.

Our answer is the big number in the middle of the slide. India will need about 4,700 terawatt-hours in 2047. That is 2.8 times what India generated in 2023-24, or about 3,000 terawatt-hours more.

But a single number is not enough for a plan that runs 25 years. So we forecast the full range. Eight futures in ten fall between 3,500 and 6,400 terawatt-hours. Our idea is to plan against that whole range, not one line, and to test every plan at the high end.

Two rules shape everything we did. First, we model energy generated, never installed capacity or consumption. Second, the forecast uses only the two challenge datasets: generation from the National Power Portal, and state population and income from the Renewable India dataset. The organisers answered our data questions in writing, and this slide follows their ruling.

Sai Anirudh will now explain how the model works.

## 2. Approach (Speaker 2, 2:00)

*Cursor: the left column, the 0.83, then down the six steps.*

Thank you. Please look at the left column.

The core of the model is one relationship, estimated on India's own data: generation per person rises with real income per person. The big number, 0.83, is that income elasticity. When income per person grows by one percent, generation per person grows by 0.83 percent.

The estimate is tight, with a standard error of 0.063. It holds in growth rates too, at 0.78, so it is not two trends that happen to move together. Every window and form we tried gives between 0.78 and 0.87.

The six steps are the pipeline.

1. Only NPP and organiser data enter the estimation.
2. We resample the history in blocks, two thousand times, so the uncertainty in 0.83 is carried into the forecast.
3. Urbanisation. Neither dataset has an urban share, so it enters through a prior, gamma, centred at one with a wide spread. That is our judgment, which the organisers allowed. Setting gamma to zero gives the pure two-dataset run, and it is only 5 percent lower.
4. We combine all of this in 4.2 million simulated futures on a GPU.
5. We split the national total across 34 states using the dataset's own state populations, with state incomes converging slowly, and group the states into NPP's five grid regions.
6. A least-cost optimisation chooses each region's generation mix, and we test it at the 90th percentile.

Devendra will show how well this performs.

## 3. Holdout and what drives the range (Speaker 3, 1:45)

*Cursor: the minus 3.3 and plus 0.5, then the minus 24 and plus 31.*

Thank you. Middle column, second row.

A forecast is only as good as its test. We held back the two most recent years, 2024-25 and 2025-26, and never used them in fitting. The model missed them by minus 3.3 percent and plus 0.5 percent, and both actual values fell inside its 90 percent range. In rolling backtests from seven start years, the average error is 2 percent one year ahead and about 6 percent five years ahead.

*(cut if short)* The model also carries its own error. Generation in 2023-24 sat above the fitted line, and we let that gap fade over the following years instead of assuming it vanishes at once or lasts for ever.

Third row: what drives the range. By far the biggest driver is income growth. If income per person grows 1.5 points a year slower than our central path, 2047 falls by 24 percent. If it grows 1.5 points faster, 2047 rises by 31 percent. Elasticity, urbanisation and population each move 2047 by 11 percent or less.

So our message to planners is: watch income growth, and update the plan as each year's NPP data arrive. Our dashboard does that update live.

Ayush will take us from the national total to the regions and the mix.

## 4. Regions and the generation mix (Speaker 4, 2:00)

*Cursor: the 36 and 26 at the bottom of the middle column.*

Thank you. The bottom row: 36 and 26 percent.

Today the Western region produces 36 percent of India's conventional generation. In 2047 it accounts for only 26 percent of the requirement. The Northern, Southern and North-Eastern regions need a larger share than they produce today. In 2047 the Northern region needs about 1,400 terawatt-hours, the Western 1,230, the Southern 1,070, the Eastern 840 and the North-Eastern 160. So power must flow between regions, and the transmission corridors must grow.

For the mix, we solve a least-cost optimisation across the five regions with real limits: resource potentials, land, build rates, transmission corridors and policy shares.

Our recommended plan, which we call Robust, is 78 percent non-fossil. Solar supplies about half of all generation, followed by firmed renewables, hydro, wind and nuclear, and coal falls to 22 percent. Compared with the base policy, it costs 37 billion dollars a year less and emits 378 million tonnes less CO2: 993 instead of 1,371.

Then we stress-test it. At the 90th percentile, 6,400 terawatt-hours, the base plan leaves 1,529 terawatt-hours unmet and the Robust plan 638. The limit is land, not money. With 45 percent of the solar potential usable, through rooftops and agri-PV, even the 90th percentile is met.

Gaurav will close with our roadblocks and next steps.

## 5. Roadblocks, questions asked, next steps (Speaker 5, 1:45)

*Cursor: down the right column, then the "Organisers confirmed" line.*

Thank you. The right column lists the roadblocks we hit and what we did about each.

- NPP has no renewable generation after 2023-24, only all-India solar and wind from its Merit portal since September 2025. For the holdout we extend renewables at their recent growth of 14.1 percent a year.
- Neither dataset has an urban share, so urbanisation is a prior, and we always show the run without it.
- The state income series changes base in 2011 and repeats some values. We impute 2011 and skip the repeats.
- Captive power is ruled out, so we use NPP generation only, and the regional split uses the dataset's own state populations.
- The history includes years of power shortages, which may inflate the elasticity. A declining-elasticity case gives 11 percent less, and it is a control in the dashboard.

We also checked our approach with the organisers in writing. They confirmed that we use generation data only, that published resource and cost data may inform the mix, and that four, five or six regions are all acceptable. So nothing in our approach has to change.

Before the final, we will check the plan hour by hour with NPP's solar and wind profiles to size storage, and add an industrial demand layer for the steel and aluminium regions. Everything already runs in a live dashboard where every assumption is a control.

Thank you. We welcome your questions.

## Numbers to have ready

| What | Value |
| --- | --- |
| 2047 median (FY2047-48) | 4,703 TWh |
| 80% range (P10 to P90) | 3,497 to 6,400 TWh |
| 90% range (P5 to P95) | 3,217 to 7,002 TWh |
| FY2023-24 base | 1,698 TWh (2047 is 2.77 times, +3,005 TWh) |
| Elasticity | 0.83 (s.e. 0.063); growth rates 0.78; all windows 0.78 to 0.87 |
| Holdout | FY2024-25 -3.3%, FY2025-26 +0.5%, both inside the 90% range |
| Backtest error | 2.0% at 1 year, 4.2% at 3 years, 6.4% at 5 years |
| Income growth +/-1.5 points | +31% / -24% on 2047 |
| No urbanisation term (gamma = 0) | 4,453 TWh (-5%) |
| Declining elasticity | 4,197 TWh (-11%) |
| Regions, 2047 median | North 1,405; West 1,226; South 1,068; East 840; North-East 161 TWh |
| Robust plan | 78.3% non-fossil; coal 21.7%; 993 Mt CO2; 176 bn USD a year |
| Base plan | 70% non-fossil; 1,371 Mt CO2; 213 bn USD a year |
| Unmet at P90 | Base 1,529 TWh; Robust 638; 0 with 45% of solar potential usable |
| CO2 cap on Robust | 993 to 643 Mt (-35%) for +0.65 bn USD a year, at most 6.3 USD/t |
| Storage, Robust plan | 3,042 GWh, about 6.8 hours |
| Land, Robust plan | 19,990 km2, 0.61% of India |
| Speed | 1 million futures in 7.7 ms on the GPU (2.21 s on 16 CPU threads) |
