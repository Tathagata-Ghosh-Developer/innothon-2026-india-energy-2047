# Final evaluation: talk script

Saturday 21 November 2026, 11:10 to 11:40 AM. Team "The Losers", IISc Bengaluru, to the ExxonMobil INNOTHON 2026 jury.
Format: 20 minutes of presentation, including a live dashboard demo, then 10 minutes of questions.
Deck: [deck/India_Energy_2047_The_Losers.pptx](deck/India_Energy_2047_The_Losers.pptx) (7 slides; PDF copy beside it).
Questions and answers: [final_qa.md](final_qa.md).
The jury scores innovation, technical implementation, usability, impact, and the quality of the presentation and demonstration.

Every number below matches `outputs/*.csv`, `outputs/site_summary.json` and the deck.

## Before the session

- **Display names:** "Your Name - The Losers" (for example "Gaurav Choudhary - The Losers").
- **Join 15 minutes early,** by 10:55 AM.
- **Open the dashboard at least 5 minutes before we start:** <https://india-energy-2047.vercel.app/app/>.
  - The first load takes about a minute, and the browser build then runs 65,536 futures before the first view appears.
  - Leave it on the "National forecast" tab. It opens on the Base scenario, the Robust policy and "Plan for requirement quantile" at P50. Do not touch anything else.
  - Open the landing page, <https://india-energy-2047.vercel.app/>, in a second browser tab for the optional map in the demo.
- **Backups, ready before we start:**
  - the PDF, [deck/India_Energy_2047_The_Losers.pdf](deck/India_Energy_2047_The_Losers.pdf);
  - the native dashboard, already loaded: `"%USERPROFILE%\.e47build_core\apps\e47_dashboard.exe" "<repo folder>"`. It runs the full 4.2 million futures, so its numbers match the slides;
  - the screenshots in [img/](img/) (`1_national_forecast.png`, `2_regional_requirement.png`, `3_generation_mix_2047.png`), if both dashboards fail.
- **Screen sharing:** Tathagata shares his whole screen (not one window), so he can switch between the slideshow and the browser with Alt+Tab without re-sharing. Sai Anirudh has the PDF open, ready to share at once if Tathagata's connection drops.
- **The repository is private.** Slide 1 shows the GitHub link, but never say the code is public. Say: "The repository is private; we will share access with the jury on request."
- **Check the organisers' reply** to our 17 November clarification on published figures in the mix. If it has arrived, update question 2 of [final_qa.md](final_qa.md) and the last paragraph of slide 2 before we start.
- **Mute when not speaking.** Each speaker ends with the hand-over line, so there is no gap.
- **Say it this way:**
  - "terawatt-hours" in full, never "T-W-h";
  - "twenty forty-seven" for FY2047-48; "twenty twenty-three twenty-four" for 2023-24;
  - "CO2" as "C-O-two"; "Mt" as "million tonnes"; "bn USD" as "billion dollars";
  - "GW" as "gigawatts", "GWh" as "gigawatt-hours", "km²" as "square kilometres";
  - "NPP" as "N-P-P" (the National Power Portal).

## Speakers and timing

Swap names freely; the script does not depend on who speaks.

| # | Slide | Speaker | Time | Clock at end | Wall clock at end |
| --- | --- | --- | --- | --- | --- |
| 1 | Title and the headline number | Tathagata Ghosh | 2:00 | 2:00 | 11:12:00 |
| 2 | Pipeline and data rules | Sai Anirudh Guttapati | 2:30 | 4:30 | 11:14:30 |
| 3 | Forecast, holdout and drivers | Devendra Umbrajkar | 3:00 | 7:30 | 11:17:30 |
| 4 | Regional mix and corridors | Ayush Kumar | 3:00 | 10:30 | 11:20:30 |
| 5 | Five innovations | Gaurav Choudhary | 2:00 | 12:30 | 11:22:30 |
| 6a | The engine (talk) | Gaurav Choudhary | 1:00 | 13:30 | 11:23:30 |
| 6b | Live demo | Tathagata Ghosh clicks, Gaurav Choudhary narrates | 4:00 | 17:30 | 11:27:30 |
| 7 | Impact, recommendations, close | Tathagata Ghosh | 2:30 | 20:00 | 11:30:00 |
| | Questions | Owners in [final_qa.md](final_qa.md) | 10:00 | 30:00 | 11:40:00 |

There is no spare time in the 20 minutes.

- **Checkpoints:** slide 4 starts by 7:30, slide 6 by 12:30, the demo by 13:30 and slide 7 by 17:30.
- **If you are more than 20 seconds behind a checkpoint,** drop the sentences marked *(cut if short)*. Together they save about 2 minutes.
- **In the demo,** drop step 5 (the landing-page map) first.
- **The next speaker keeps time** for the current one and holds up a hand at 30 seconds to go.

## 1. Title and the headline number (Tathagata Ghosh, 2:00, ends at 2:00)

*Cursor:* the title; then the big "4,703 TWh"; then the "P10 to P90" line under it; then the row of regional medians. At "Robust plan", move to the "The Robust plan" box on the right; at the end, the live-dashboard link at the bottom.

Good morning. We are team The Losers from the Indian Institute of Science, Bengaluru: Ayush, Devendra, Sai Anirudh, Gaurav, and I am Tathagata.

The challenge asks two things. How much electricity must India generate in twenty forty-seven, nationally and region by region, allowing for population growth and urbanisation? And how should each region produce it, from coal, hydro, solar, wind, nuclear and natural gas? We measure energy generated, in terawatt-hours. We never forecast installed capacity or consumption.

Our answer is the big number: 4,703 terawatt-hours in twenty forty-seven. It is the median of 4.2 million simulated futures, and eight futures in ten lie between 3,497 and 6,400 terawatt-hours. That is 2.77 times the 1,698 terawatt-hours India's utilities generated in 2023-24: about 4.3 percent growth a year.

*(cut if short)* By grid region, the medians are North 1,405, West 1,226, South 1,068, East 840 and North-East 161 terawatt-hours.

To meet it, we recommend what we call the Robust plan. It is led by solar. It keeps the nuclear and hydro plants already being built, keeps coal mainly in the East, holds the gas fleet for the evening peak, and uses corridors that carry power from the West and South. At the median requirement it reaches 79.5 percent non-fossil, 912 million tonnes of CO2, and about 178 billion dollars a year.

*(cut if short)* All of this is live at india-energy-2047.vercel.app, and we will show it on slide 6. Our repository is private; we will share access with the jury on request.

*Hand-over:* Sai Anirudh will now show how one pipeline turns two datasets into these numbers.

## 2. Pipeline and data rules (Sai Anirudh Guttapati, 2:30, ends at 4:30)

*Cursor:* steps 1, 2 and 3 on the top row, resting on the 0.83; then steps 4, 5 and 6 on the second row. At "the rules", move to the "Source hierarchy" box: PRIMARY, then SECONDARY; then "Rule compliance".

Thank you. This slide is our pipeline in six steps, and the rules it follows.

Step one matters most. The twenty forty-seven requirement is estimated only on the two primary datasets the organisers gave us: generation from the National Power Portal, 2006-07 to 2023-24, and state population and GSDP from the Renewable India dataset.

Step two. From India's own history we estimate one number, the income elasticity. From 2011-12 to 2023-24, one percent more real income per person has meant 0.83 percent more generation per person, with a standard error of 0.063.

Step three. On the GPU we draw 4.2 million futures of income growth, population, urbanisation and model error, together. So our answer is a distribution, not one line.

Step four. Each of the 34 states and union territories in the dataset carries its own population and income, and poorer states close about 2.1 percent of their income gap each year. We sum the states into NPP's five grid regions.

Step five. A least-cost linear program, solved with the open-source HiGHS solver, chooses each region's mix within published build limits, costs and emission factors. We solve it at the median requirement, and again at the 90th percentile.

Step six. The same C++ engine runs the dashboard you will see.

Now the rules. NPP and Renewable India are primary. Everything else is secondary and labelled: UN population and urban paths, our urbanisation prior, build limits from CEA, NITI Aayog and the Department of Atomic Energy, IEA costs, CEA emission factors, and NISE and NIWE potentials. Published figures only bound and price the mix. None of them enters the twenty forty-seven requirement.

*(cut if short)* We asked the organisers whether published figures may decide the mix. Their written answer was: "Use insights from this data source." We have asked for a clarification, and we do not treat it as confirmed. If they rule that these figures may only justify the mix, the requirement and its regional split do not change.

*Hand-over:* Devendra will now show whether the forecast holds up.

## 3. Forecast, holdout and drivers (Devendra Umbrajkar, 3:00, ends at 7:30)

*Cursor:* the history line on the left up to 1,698; the two bands; the labels 2,266, 3,594 and 4,703. At "held back", the holdout box (minus 3.3, plus 0.5). At "CEA", the NEP marker. At "what drives", the bar chart on the right. At the end, the minus 24 and plus 31 callout.

Thank you. On the left, the line up to 2023-24 is NPP history: 1,698 terawatt-hours. After it come the 4.2 million futures. The dark band holds half of them, the light band 80 percent. The median reaches 2,266 terawatt-hours in 2030-31, 3,594 in 2040-41, and 4,703 in twenty forty-seven.

Is it credible? We held back two years the model never saw, 2024-25 and 2025-26. The model's medians were 1,727 and 1,786 terawatt-hours; NPP gives 1,785 and 1,776. The errors are minus 3.3 and plus 0.5 percent, and both actual values fall inside the model's 90 percent range.

*(cut if short)* In rolling backtests from seven start years, the average error is 2.0 percent one year ahead and 6.4 percent five years ahead.

Our benchmark compares generation with generation. CEA's National Electricity Plan projects 2,666 terawatt-hours of gross generation in 2031-32. Our median for that year is 2,382, 11 percent lower, and CEA's figure lies between our 75th percentile, 2,551, and our 90th, 2,717. So it sits inside our range. CEA has published no figure for twenty forty-seven.

Now the chart on the right: what drives the growth. On the central path, income per person carries 87.5 percent of the growth, population 14.8 percent, and urbanisation beyond what income brings 4.8 percent. The fading of the unusually high 2023-24 level takes back 7.1 percent: that year sat above the fitted line, and the model lets the gap fade. So income is the largest driver, but not the only one.

*(cut if short)* We split the central path, 4,600 terawatt-hours, rather than the median, because the median of millions of futures cannot be split into parts.

And income growth decides where in the range we land. If income per person grows 1.5 points a year slower, twenty forty-seven comes in at 3,588 terawatt-hours, 24 percent less. If it grows 1.5 points faster, 6,169, 31 percent more. The elasticity, urbanisation and population each move it by 11 percent or less.

That is why we plan against the whole range, not one number, why we test every plan at the 90th percentile, and why the dashboard lets you move income growth yourself.

*Hand-over:* Ayush will now take the national total to the regions and the mix.

## 4. Regional mix and corridors (Ayush Kumar, 3:00, ends at 10:30)

*Cursor:* the bars from left to right: North, West, South, East, North-East, resting on each region's label as you name it. At "corridors", the flow box on the right. At "India generates", the line at the bottom.

Thank you. This is our recommended Robust plan for twenty forty-seven, by grid region, and each region builds on its own strength. Each bar is a region's generation by source, the black dash is its requirement, and the label says whether it exports or imports.

North. Rajasthan is in NPP's Northern region and has the largest published solar potential of any state, so the North's solar, 706 terawatt-hours, is led by Rajasthan. The North adds Himalayan hydro, 147 terawatt-hours, and committed nuclear, 52. It still imports 153.

West. The largest solar region, 851 terawatt-hours, and the largest wind region, 110, about half of India's wind. It also holds the most gas kept for peaking, 20 terawatt-hours, 38 percent of it. It exports 299 terawatt-hours.

South. Solar 641 and wind 93, plus the most committed nuclear, 57 terawatt-hours, at Kudankulam and Kalpakkam. It exports 125 to the East.

East. The young coal fleet stays: 374 terawatt-hours, 41 percent of the plan's coal. Its solar and wind potential is small, so it imports 262.

North-East. Hydro, 120 terawatt-hours, 72 percent of its own generation, plus 16 from the Assam and Tripura gas fields.

The corridors carry 430 terawatt-hours: West to North 152, West to East 147, South to East 125, and North-East to North 6. That needs three times the corridors of the 2032 plan; the busiest, South to East, runs at two-thirds of its capability. India generates 4,718 terawatt-hours for a requirement of 4,703; the difference is corridor losses.

Nationally, solar gives 50.7 percent, coal 19.4, hydro 7.3, wind 4.5, nuclear 2.8 and gas 1.1. Firmed solar and wind with storage, sold round the clock, gives 14.2 percent; we show it separately because it is not one of the six sources.

*(cut if short)* Why so little nuclear and gas? Nuclear stays at the reactors already committed, 133 terawatt-hours, because at IEA's twenty forty-seven costs solar with storage is cheaper up to the land limit. Gas stays at its best NPP year, 51 terawatt-hours, because no new gas plant is published.

*(cut if short)* The plan splits the national median by regional shares, so its regional requirements differ from slide 1's regional medians by at most one terawatt-hour.

*Hand-over:* Gaurav will now explain what makes this plan defensible.

## 5. Five innovations (Gaurav Choudhary, 2:00, ends at 12:30)

*Cursor:* each row in turn, first the big number on the left, then the proof line on the right.

Thank you. Five things set this work apart, and each row ends with the number that proves it.

One: a probability distribution, not one path. Official outlooks give one number per year. We give 4.2 million futures and their range, 3,497 to 6,400 terawatt-hours, checked on two held-back years.

Two: the plan is stress-tested at the 90th percentile, because a plan sized only for the median fails exactly when growth is strong. At the 90th-percentile requirement, the Base plan leaves 1,463 terawatt-hours unmet; the Robust plan cuts that to 633.

Three: realistic limits. Each source grows only as fast as CEA, NITI Aayog and the Department of Atomic Energy publish. Existing fleets start at their best NPP year, and reactors and dams already under construction are floors. That is why nuclear is 133 terawatt-hours and hydro at least 185.

Four: one engine. The same C++20 code runs CUDA on a GPU, OpenMP on CPUs and WebAssembly in your browser. Every future gets the same random inputs on CPU and GPU, and an automated parity test checks them. A million futures take 7.73 milliseconds on a laptop GPU, against 2.19 seconds on 16 CPU threads.

*(cut if short)* The GPU figure is the kernel time, and all timings vary with machine load.

Five: source discipline. The requirement uses only NPP and the Renewable India dataset, and its one estimated parameter is the elasticity. Without our one literature prior, on urbanisation, the requirement is 4,453 terawatt-hours, 5 percent lower.

*Hand-over:* Next slide, please: how the engine and the dashboard fit together.

## 6a. The engine (Gaurav Choudhary, 1:00, ends at 13:30)

*Cursor:* the Data, C++20 engine and Outputs boxes; then "Tests and CI", "Dashboard" and "Vercel"; at the end, "What you can do live".

The engine is C++20. The Monte Carlo runs on CUDA on a GPU, or OpenMP on CPUs; the mix is a HiGHS linear program; and an hourly check on NPP's Merit profiles sizes storage. Seven test suites run in GitHub Actions on every push, with a CPU build, a smoke run and the WebAssembly build. Every input has a provenance row.

The dashboard is built with Dear ImGui and ImPlot. The same code compiles to WebAssembly and is served free from Vercel, so nothing needs installing. The right side of the slide lists everything you can change live. In the next four minutes we will show four moves: the forecast, a growth scenario, the regional plan, and where the plan breaks.

*Hand-over:* Tathagata, over to the dashboard.

## 6b. Live demo (Tathagata Ghosh clicks, Gaurav Choudhary narrates, 4:00, ends at 17:30)

**Starting state:** the app tab is loaded, on the "National forecast" tab, with the Base scenario, the Robust policy and "Plan for requirement quantile" at P50.

**The slide's numbered list is the full tour.** Today's demo takes the shorter path below. The slide's other moves (the P90 lever, the evidence views in "Data and assumptions") are ready for the questions; see the end of this file.

**Names on screen** (from `apps/dashboard/app.cpp`):

- Tabs: "National forecast", "Regional requirement", "2047 generation mix", "Models and backtest", "Uncertainty and sensitivity", "Data and assumptions".
- Sidebar, under "Scenario": the buttons "Base", "High growth", "Low growth", "New loads (published, outside headline)".
- Sidebar, under "Policy and planning": the buttons "Base", "Low carbon", "Robust", "Energy security".
- The slider "Plan for requirement quantile" sits under "Scenario", above "Scenarios (2^n)".

**Watch out for two "Base" buttons.** Every "Base" click in this demo is the one under "Scenario". The "Base" under "Policy and planning" would replace the Robust plan.

**Do not touch:**

- "Reset policy": it loads the Base policy, not Robust.
- "Scenarios (2^n)": more futures make every update slower.
- "Cross-country (check)": switching core takes about 20 seconds.

**Every scenario click recomputes all 65,536 futures and the plan.** Each update takes several seconds; the header shows "Recomputing..." until it is done. Do not click again while it shows.

### Step 1. National forecast (0:30, 13:30 to 14:00)

- **Click:** Alt+Tab to the browser, app tab.
- **Point at:** the "2047 requirement (median)" card; then the "90% range" card; then the fan chart; then the diamonds.
- **Say:** "This is the dashboard, live in the browser. The first card is the twenty forty-seven median, about 4,700 terawatt-hours. The browser runs 65,536 futures on its CPU threads, not 4.2 million on a GPU, so the last digits can differ from the slides. Next is the 90 percent range, about 3,200 to 7,000, and the diamonds are CEA's plan projections of gross generation."

### Step 2. High growth, then back to Base (0:50, 14:00 to 14:50)

- **Click:** "High growth" under "Scenario". The header shows "Recomputing...".
- **Say, during the wait:** "High growth: income per person grows 1.5 points a year faster to 2030, cities grow faster, and 100 to 300 terawatt-hours of new loads, such as green hydrogen, are added. The browser is re-running every future, so this takes a few seconds."
- **When it lands, point at** the median card and the band, and **say:** "The median moves to about 6,800 terawatt-hours, four times today, and the whole band moves up. That is slide 3's income risk, live."
- **Click:** "Base" under "Scenario".
- **Say, during the wait:** "Back to Base, so the next views match our slides."

### Step 3. Regional requirement (0:30, 14:50 to 15:20)

- **Click:** the "Regional requirement" tab.
- **Point at:** the "2047 P50" column of the table; then the "Share of national total (%)" bars on the right.
- **Say:** "Regional requirement. The medians match slide 1: North about 1,400, West 1,230, South 1,070, East 840 and North-East 160 terawatt-hours. On the right, each region's share of generation in 2023-24 sits next to its share of the twenty forty-seven requirement. The West generates more than a third of India's power today but needs about a quarter in twenty forty-seven; the corridors close that gap."

### Step 4. The 2047 generation mix, Robust plan (0:40, 15:20 to 16:00)

- **Click:** the "2047 generation mix" tab.
- **Point at:** the four cards ("Non-fossil share 2047", "CO2 from generation", "Generation cost", "Unmet requirement"); then the stacked bars; then the "Net export (+) / import (-)" chart; then scroll down to the storage and land table.
- **Say:** "This is the Robust plan: 79.5 percent non-fossil, 912 million tonnes of CO2, about 178 billion dollars a year, and no unmet requirement at the median. The next chart shows the West and South exporting while the North and East import. Below is the hourly check: about 448 gigawatts and 3,044 gigawatt-hours of storage, and 0.61 percent of India's land for solar and wind."

### Step 5. Optional: the six-source map (0:20, 16:00 to 16:20)

Skip this step if the clock is past 16:00.

- **Click:** Ctrl+Tab to the landing-page tab. Scroll to "Regional generation strength". Click the chips "Solar", then "Hydro", then "Natural gas".
- **Say:** "The landing page shows the same plan as a six-source map: solar largest in the West, hydro in the North and North-East, and the peaking gas largest in the West."
- **Click:** Ctrl+Tab back to the app tab. It is still on "2047 generation mix".

### Step 6. Where it breaks (0:55, 16:20 to 17:15)

Do these clicks in this order. A scenario button resets "Plan for requirement quantile" to P50, so choose "Low growth" first and set P10 second.

- **Click:** "Low growth" under "Scenario". Leave the policy at Robust.
- **Say, during the wait:** "Now let us break it. Low growth: income per person grows 1.5 points a year slower. At its median, about 3,400 terawatt-hours, the Robust plan still works."
- **Click:** drag the "Plan for requirement quantile" handle all the way to the left, until it reads "P10". Or Ctrl+click the slider, type 10 and press Enter.
- **When the red "Infeasible under these settings" appears, point at it and say:** "Now plan for the 10th percentile of that low future, roughly 2,500 terawatt-hours. Infeasible: the dashboard names the levers instead of showing numbers. The Robust plan keeps half of today's coal in every region, 647 terawatt-hours, plus 51 of gas for peaking, both fixed amounts. At 75 percent non-fossil, fossil may supply only a quarter of generation, so below about 2,780 terawatt-hours the floors no longer fit. We state this as a known limit; every policy is feasible at every scenario's median."

Why this holds: 647 + 51 = 698 TWh of fixed fossil generation. At 75 percent non-fossil, generation must be at least 698 / 0.25 = 2,792 TWh, and the lowest feasible requirement found by bisection is 2,780 TWh (test `robust_low_requirement_limit`). The Low growth P5 is 2,332 TWh (`outputs/presets.csv`, where the Robust plan is infeasible). Its P10 is not in the output files. Scaled from the Base scenario's ratio of P10 to P5 (3,497 / 3,217), it is about 2,540 TWh, well below 2,780 even with the browser's 0.5 percent sampling difference. That is why the script says "roughly 2,500".

### Step 7. Back to Base (0:15, 17:15 to 17:30)

- **Click:** "Base" under "Scenario". This also resets the quantile to P50; the policy stays Robust. Wait until the cards show 79.5 percent again.
- **Say:** "Back to Base, and the plan is feasible again. It is all at india-energy-2047.vercel.app for you to try. Tathagata will close."
- **Click:** Alt+Tab to the slideshow and go to slide 7.

### If the dashboard is slow or fails

- **An update takes more than 10 seconds.** Say: "The browser is re-running 65,536 futures and the least-cost plan on a single thread; natively, on our GPU, the futures take milliseconds. While it finishes..." Then say the next talking point while you wait.
- **It stalls for more than 20 seconds, or the page fails.** Alt+Tab to the native dashboard and do the same clicks; the tab and button names are identical. Say: "Here is the same engine running natively, with the full 4.2 million futures."
- **Both fail.** Share the PDF at slide 6 and open the screenshots in [img/](img/). Say: "These are the same views, captured from the same engine. The live link is on the slide for you to try."
- **Running late.** Drop step 5, then shorten step 3 to its first sentence. Always do step 6: the jury scores the demonstration.

## 7. Impact, recommendations, close (Tathagata Ghosh, 2:30, ends at 20:00)

*Cursor:* the four small charts from left to right; rest on the words "at the median" under the first three and "at P90" under the fourth. Then the three numbers in the middle (19,990 km², 3,044 GWh, 912 to 653 Mt); then down "What we recommend"; then the "Known limits" line.

Thank you, Gaurav. Here is the impact. At the median requirement, the Robust plan reaches 79.5 percent non-fossil instead of 70, emits 912 million tonnes of CO2 instead of 1,374, and costs 178 instead of 215 billion dollars a year.

The fourth chart is a separate solve, at the 90th-percentile requirement of 6,400 terawatt-hours. There the Robust plan leaves 633 terawatt-hours unmet instead of 1,463. At that level it costs more than the Base plan, 253 against 236 billion dollars a year, because it generates more.

It needs 19,990 square kilometres for solar and wind, 0.61 percent of India, and about 3,044 gigawatt-hours of daily storage at 448 gigawatts. A CO2 cap takes it from 912 to 653 million tonnes, 28 percent less, for only 0.46 billion dollars a year more, because nuclear replaces coal. Below 653, the young coal fleet's floor binds, not money.

Our recommendations. Plan for a range: income growth alone moves twenty forty-seven by minus 24 to plus 31 percent. Unlock solar land through rooftops, agri-PV and canal-tops: 45 percent of the potential meets even the 90th-percentile requirement. Build at the published high case, and keep committed nuclear, hydro and the peaking gas. Reinforce the corridors that carry 430 terawatt-hours to the North and East. Cap CO2, then capture what remains at large coal stations. And refresh the forecast with each new year of NPP data.

*(cut if short)* Our known limits, stated openly. You saw the first: no feasible Robust plan below about 2,780 terawatt-hours. And NPP publishes no regional renewable generation, so the regional base mix adds CEA state data, labelled secondary.

To close: in twenty forty-seven India will need about 2.8 times today's generation. Our plan meets it region by region, tested against the whole range, with lower emissions and lower cost at the median: more electricity for a growing, more prosperous India.

*Hand-over to questions:* Thank you. We welcome your questions.

## Into the questions (20:00 to 30:00)

- Stay on slide 7. Keep the dashboard tab ready in the Base scenario with the Robust policy, in case the jury asks to see something.
- Tathagata routes each question to its owner ("Devendra will take that") using the owner list in [final_qa.md](final_qa.md). The owner answers in 20 to 30 seconds: the short answer first, then one number.
- Expect 6 to 8 questions in 10 minutes. Do not talk over each other.
- If we do not know: "We have not tested that. We can show how it would enter the model and follow up in writing." Never guess a number.

**Moves ready for "show me" questions** (start from the Base scenario and the Robust policy; each update takes several seconds):

- **The P90 lever** (questions 28 and 29 in [final_qa.md](final_qa.md)). On "2047 generation mix", drag "Plan for requirement quantile" to P90: about 633 TWh of unmet requirement appears. Then drag the "solar" slider under "Usable resource (solar / wind / hydro)" from 35 to 45 percent: unmet falls to zero in the full run (the browser should show zero or close to it). Say that this plan, at P90, costs about 258 billion dollars a year, and never compare it with the median plan's 178. To return, click "Robust" under "Policy and planning", then "Base" under "Scenario".
- **The evidence views.** "Data and assumptions" tab, then "Requirement evidence" (growth decomposition, state table, state panel check, new loads) or "Mix evidence" (mix sensitivities, committed builds, gas peaking hours). The mix sensitivities are solved when the view opens, so allow a few seconds.
- **Income growth by hand.** Drag "Income growth per person, to 2030" down or up by 1.5 points; the twenty forty-seven median moves by about a quarter (3,588 or 6,169 TWh in the full run). "Reset scenario" brings it back.
