# Final evaluation: questions and answers

> Updated 21 Nov 2026 after the final evaluation to the current run; D14 lists the changes ([decisions.md](decisions.md)).

Saturday 21 November 2026, 11:30 to 11:40 AM: 10 minutes of questions after the 20-minute talk ([final_talk.md](final_talk.md)).
This file replaces [midpoint_qa.md](midpoint_qa.md) for the final. The midpoint panel's five questions are answered in full in [midpoint_panel_answers.md](midpoint_panel_answers.md).
Every number below matches `outputs/*.csv`, `outputs/site_summary.json` and the deck ([decisions.md](decisions.md), D13 and D14). Every cost is in real 2024 USD.

**Who answers.** Ayush Kumar and Sai Anirudh Guttapati cannot attend because of exams. Tathagata Ghosh takes all of their questions. Devendra Umbrajkar and Gaurav Choudhary keep their own topics.

## How to answer

- **Expect 6 to 8 questions** in 10 minutes. Answer each in 20 to 30 seconds: the short answer first, then one number. Use the "If pressed" line only if the jury asks a follow-up.
- **Tathagata routes** each question to its owner ("Devendra will take that"). The owner answers. When Tathagata owns the topic, Tathagata answers at once, without routing. If the owner is unsure, the backup in the table answers. Never talk over each other.
- **"Show me" questions:** Tathagata answers on the dashboard (kept on the Base scenario and the Robust policy; see the demo section of [final_talk.md](final_talk.md)).
- **If we do not know:** "We have not tested that. We can show how it would enter the model and follow up in writing." Never guess a number.
- **Spoken form:** say "terawatt-hours" for TWh and "twenty forty-seven" for FY2047-48. The answers use TWh to keep them short.

| Topic | Owner | Backup |
| --- | --- | --- |
| Data rules and sources | Tathagata Ghosh (slide 2) | Devendra Umbrajkar |
| The model and the elasticity | Tathagata Ghosh (slide 2) | Devendra Umbrajkar |
| Validation, range, drivers, demand features, official plans | Devendra Umbrajkar (slide 3) | Tathagata Ghosh |
| The generation mix, storage, land, corridors | Tathagata Ghosh (slide 4) | Gaurav Choudhary |
| Robustness, trade-offs, known limits | Tathagata Ghosh (slide 7) | Gaurav Choudhary |
| Engine, speed, tests, the dashboard, usability | Gaurav Choudhary (slides 5 and 6) | Tathagata Ghosh |
| Impact and next steps | Tathagata Ghosh (slide 7) | Gaurav Choudhary |

Within each group the most likely question comes first.

## 1. Data rules and sources

**1. Do you use any data beyond the two challenge datasets?** *Owner: Tathagata.*
For estimating the twenty forty-seven requirement, no. Generation comes from the National Power Portal and state population and GSDP from the Renewable India dataset, our only primary sources. Two driver assumptions the organisers' ruling allows enter the projection: UN population and urban paths, and our urbanisation prior. Without the prior, the pure two-dataset run gives 4,438 TWh, 5 percent lower.
*If pressed:* captive plants are excluded, as the organisers ruled, so 4,688 TWh is utility generation. Our cross-country model (4,528 TWh, within 4 percent) is a check used for justification only, never for estimation.

**2. Is it within the rules to use published figures for the mix?** *Owner: Tathagata.*
At the kickoff the organisers said published literature may set each source's growth and constraints; only the 2047 prediction must use the two datasets. So published limits, costs and emission factors only bound and price the mix, each labelled secondary. Asked in writing, the organisers answered: "Use insights from this data source." We asked for a clarification on 17 November and do not treat that answer as confirmed.
*If pressed:* if they rule that published figures may only justify the mix, the requirement and its regional split do not change; only the way the mix is chosen does.
*(Update this answer before the session if the organisers reply.)*

**3. Do you use installed capacity or consumption anywhere?** *Owner: Tathagata.*
Never as a metric. The dataset's capacity columns are never read, and no consumption figure enters the model. Published GW figures (potentials, build additions, committed plants) are converted to TWh with stated capacity factors and only bound generation in the mix. For example, 14,300 MW of committed nuclear becomes a floor of 85 TWh at a capacity factor of 0.68.
*If pressed:* storage in GW and GWh is a size the plan needs, not a forecast.

**4. NPP has no regional renewable generation. How did you build the regional mix?** *Owner: Tathagata.*
For the requirement nothing is missing: NPP's all-India series includes renewables up to 2023-24, our base year. For the regional base mix, NPP gives conventional generation by region, 1,508.5 TWh (primary), and CEA's General Review fills in the regional renewables, 225.8 TWh (secondary, labelled).
*If pressed:* that base totals 1,734 TWh, 36 TWh above NPP's 1,698, because CEA's renewables total is higher than NPP's; the forecast uses NPP's 1,698. We checked every NPP service; none gives renewable generation by region or state.

**5. UN population paths and an urbanisation prior: aren't they outside data?** *Owner: Tathagata.*
Yes, and the organisers ruled on both. Outside projections are allowed if consistent with the dataset's state growth: every state's path to 2030 is within 0.14 points a year of its own 2019-24 growth in the dataset. The urbanisation coefficient is our own judgment, a prior centred at 1. It carries 4.8 percent of the growth; setting it to zero gives 4,438 TWh.

**6. Why five regions?** *Owner: Tathagata.*
NPP reports generation by its five grid regions, and the organisers answered in writing: "Four, five, or six regions - either is fine". Each of the 34 states and union territories sits in the region NPP reports it under; Rajasthan, for example, is Northern.

## 2. The forecast

**7. Why a simple elasticity model and not machine learning?** *Owner: Tathagata.*
We have 18 annual national data points. One estimated slope can be tested and explained; a flexible model would fit the noise. On two years the model never saw, its errors were minus 3.3 and plus 0.5 percent.
*If pressed:* the uncertainty comes from a block bootstrap (2,000 replicates) and 4.2 million futures, not from model complexity.

**8. Is income growth the only factor?** *Owner: Devendra.*
No, but it is the largest. On the central path to twenty forty-seven, income per person carries 87.5 percent of the growth, population 14.8 percent and urbanisation beyond income 4.8 percent; the fading of the unusually high 2023-24 level takes back 7.2 percent.
*If pressed:* we split the central path (4,589 TWh), not the median (4,688), because the median of millions of futures cannot be split into parts. The parts add up exactly, and a test checks it.

**9. Why is the range so wide?** *Owner: Devendra.*
Because future income growth is uncertain, and we show that honestly. Income per person growing 1.5 points a year slower or faster gives 3,568 or 6,163 TWh: minus 24 or plus 31 percent. The elasticity, urbanisation and population each move it by 11 percent or less. So we plan against P10 to P90, 3,471 to 6,408 TWh, and test the plan at P90.

**10. How do you know the model works?** *Owner: Devendra.*
A holdout and backtests. FY2024-25 and FY2025-26 were never used in fitting: the model gave 1,726 and 1,785 TWh against 1,785 and 1,776 from NPP, errors of minus 3.3 and plus 0.5 percent, both inside its 90 percent range. Rolling backtests from seven start years average 2.0 percent error one year ahead and 6.4 percent five years ahead.
*If pressed:* NPP has no renewables for those two years, so renewables are extended at their recent growth of 14.1 percent a year; checking conventional generation alone would be biased, because it fell 2.5 percent in FY2025-26.

**11. Is 0.84 too high, given India's past power shortages?** *Owner: Tathagata.*
Possibly slightly, so we test it. The first-difference estimate, 0.79, gives 4,366 TWh (minus 6.9 percent). One standard error either way moves twenty forty-seven by minus 6.5 or plus 7.0 percent. An elasticity that falls 0.2 per unit of log income gives 4,194 TWh (minus 10.5 percent). All lie well inside the P10 to P90 band; each 0.01 of elasticity moves the result by about 1 percent.

**12. Is income used state-wise? And why does the split use 0.74, not 0.84?** *Owner: Tathagata.*
State-wise: the GSDP and population of 34 states and union territories from the Renewable India dataset, with poorer states closing only about 0.2 percent of their income gap a year, so relative incomes barely change by twenty forty-seven. On state income the effective exponent is 0.74: the elasticity 0.835 minus the part of the urbanisation term that moves with income (1.0 times 0.093), because the dataset has no state urban share. Uttar Pradesh, at 0.44 of India's income per person in twenty forty-seven, needs 0.54 of India's generation per person.
*If pressed:* never quote 0.84 for the split. The states summed by region land within 0.2 points of the regional medians.
*If asked why poorer states do not catch up:* in this dataset they barely do. Measured from each state's first real, non-repeated GSDP year, the convergence speed is 0.0021 a year (standard error 0.0051), not distinguishable from zero. The 2.1 percent in the run presented on 21 November came from placeholder years that repeat a value. The estimate depends on how placeholders are read: a looser reading gives 0.0166 (about 1.7 percent), and leaving out Delhi and Chandigarh gives about zero. This change is why the Eastern median fell from 840 to 730 TWh and the Southern rose from 1,068 to 1,188. The national total does not depend on it ([decisions.md](decisions.md), D14, R25 and R28).

**13. Why not estimate the elasticity state by state?** *Owner: Tathagata.*
Because NPP state generation records where plants stand, not what a state needs: Sikkim generates 15.9 times the national figure per person, Nagaland 0.08 times. A state panel gives 0.52 (standard error 0.26), and 97 percent of the variation lies between states. So we estimate nationally, where generation and need coincide, and apply it state by state.

**14. What about electric vehicles, data centres, cooling and green hydrogen?** *Owner: Devendra.*
Loads that grew with income are already inside the elasticity, so adding them in full would double count. What lies beyond the historical trend is a separate demand-feature layer from official sources only: 44 drivers listed, 11 quantified as TWh of 2047 generation. Every feature at its central value (Features central) gives 5,810 TWh, plus 23.9 percent; drawing the features jointly gives a median of 5,668 TWh (P5 4,130 to P95 8,017). It is never the headline.
*If pressed:* the largest central sizes beyond the trend are transport 512 TWh, data centres 397, green hydrogen 230 and storage round-trip losses 150; auxiliary consumption, pump solarisation, rooftop solar and imports lower it. Space cooling adds nothing beyond the trend at its central value; the Electrification high and Climate stress scenarios raise it. The named scenarios run from 4,865 (All downside) to 6,555 TWh (All upside). The Robust plan meets the joint median in full; planned for the joint P90 (7,404 TWh) it leaves 1,635 TWh unmet, against 639 at our own P90.

**15. Why don't the regional medians add up to 4,688?** *Owner: Devendra.*
The median of a sum is not the sum of medians: the five regional medians add to 4,686 TWh. The plan splits the national median by regional shares, so its regional requirements (1,343, 1,276, 1,188, 730 and 151 TWh) add to exactly 4,688.

## 3. Comparison with official plans

We compare generation with generation only. CEA's energy requirement figures (the Electric Power Surveys) are consumption-side and are never compared with our forecast.

**16. How does your forecast compare with CEA's National Electricity Plan?** *Owner: Devendra.*
Like for like, generation against generation: CEA's National Electricity Plan 2022-32 projects 2,666 TWh of gross generation for FY2031-32. Our median is 2,378, 11 percent lower, and CEA's figure sits between our P75 (2,550) and P90 (2,719), inside our range. CEA has published no twenty forty-seven figure.
*If pressed:* our scope is NPP utility generation, without captive plants, and our path is checked against actual NPP generation up to FY2025-26.

**17. CEA's 2026 review gives about 3,596 BU for 2035-36. You are 20 percent lower. Why?** *Owner: Devendra.*
That is a gross-generation figure, so it is comparable. Our FY2035-36 median is 2,879 TWh, 19.9 percent lower, and 3,596 sits just inside our 95th percentile, 3,641: inside our range, at the high end. Our path is anchored on actual NPP generation through FY2025-26 and excludes captive plants.
*If pressed:* the 3,596 figure comes from press reports; the CEA document is not public yet.

**18. What about CEA's energy requirement projections, such as the Electric Power Survey?** *Owner: Devendra.*
We never compare with them. They are consumption-side and measured ex-bus, while we forecast energy generated, so a comparison would mix two metrics. For example, the 20th Electric Power Survey's 2,474 BU for FY2031-32 is context only, and so is the "about 3,365 BU" requirement quoted in the 2026 review reports.

## 4. The generation mix

**19. Why is nuclear only 2.8 percent?** *Owner: Tathagata.*
Cost. At IEA's 2047 costs, solar with storage is cheaper up to the land limit, so the plan keeps exactly the committed reactors: 14,300 MW under construction or sanctioned, 133 TWh. Under a CO2 cap nuclear is the next step: its share rises to 8.1 percent as coal falls.
*If pressed:* in the Base plan, with less usable land, nuclear reaches its 10 percent limit (469 TWh).

**20. Why is gas only 1.1 percent?** *Owner: Tathagata.*
No new gas plant is published, so gas stays at the existing fleet's best NPP year, 51 TWh in FY2020-21, kept for peaking and as a bridge for solar and wind. In our hourly check it runs 2,698 hours a year, at most 21 GW, with 49 percent of its energy between 6 pm and midnight.
*If pressed:* at IEA's 2047 costs gas energy is dearer than coal or renewables, so the optimiser would not add more even if it could.

**21. Why is coal still 19.0 percent?** *Owner: Tathagata.*
India's coal fleet is young, so the plan keeps at least half of each region's 2023-24 coal output, a floor of 647 TWh; coal ends at 893 TWh. A CO2 cap brings coal down to 13.8 percent and CO2 from 891 to 653 Mt, for only 0.45 bn USD a year more (real 2024 USD). The coal that remains sits at large stations, which suits carbon capture.

**22. Why does solar lead in the West, not the North? Rajasthan is in the North.** *Owner: Tathagata.*
Rajasthan is the largest single state, about 829 GW of NISE ground-mounted potential, and it does lead the North's 706 TWh of solar. But solar is at its land limit in every region, 35 percent of the potential, so it follows each region's total. The West's states together (Maharashtra, Madhya Pradesh, Gujarat, Chhattisgarh) hold 2,431 TWh a year of potential against the North's 2,017; hence 851 TWh in the West and 706 in the North.

**23. Is 448 GW of storage realistic? And how much land does the plan take?** *Owner: Tathagata.*
It is large but in line with today's plans. 448 GW and 3,048 GWh, about 6.8 hours, is 9 times the 336 GWh in CEA's optimal mix for 2029-30, and the Ministry of Power already projects 62 GW of pumped storage plus 99 GW and 396 GWh of batteries for 2034-35. Land for solar and wind: 19,990 km², 0.61 percent of India.
*If pressed:* after daily storage, 231 TWh (4.9 percent) is still short in the hourly check, which is why firmed renewables and gas matter. The land is 3.6 percent of India's wasteland; meeting P90 with 45 percent usable solar raises it to 25,402 km² (0.77 percent).

**24. What transmission does the plan need?** *Owner: Tathagata.*
Corridors three times the 2032 plan, carrying 271 TWh: West to North 94, West to East 155, South to East 6 and North-East to North 16. The West exports 249 TWh, the North-East 16 and the South 6; the East imports 155 and the North 107. The busiest corridor, West to East, runs at 43 percent of its capability.

**25. Why does the North-East get 16 TWh of gas, 31 percent of it?** *Owner: Tathagata.*
The optimiser does not site gas. In the Robust plan the gas kept for peaking equals the gas cap, 51 TWh, so every region is held at its 2023-24 share of the fleet. The North-East's share is the Assam and Tripura gas fields. When we tested the optimiser without these bounds (D13), it put 46 TWh of gas in the East, which has almost none today.

**26. What is "firmed solar and wind"? Is it a seventh source?** *Owner: Tathagata.*
No. It is solar and wind sold round the clock with storage: 686 TWh, 14.6 percent of the plan. We show it separately because it is not one of the six sources; with it the shares add up to 100 percent. It uses the same land limits as solar and wind.

**27. Where do costs and build limits come from? Why IEA costs and not CEA's?** *Owner: Tathagata.*
All are labelled secondary. Build limits come from CEA's National Electricity Plan 2022-32, CEA's December 2024 input slides, NITI Aayog (2026), the Department of Atomic Energy and the Ministry of Power's hydro schedule; Robust builds at the published high case. Costs are IEA WEO 2025, which gives India's costs for 2035 and 2050 on one basis, so 2047 is interpolated; they are in real 2024 USD, and any cost on another price base is converted with official price indices (the MoSPI GDP deflator and the RBI exchange rate). Emission factors are CEA's v22: coal 0.971 and gas 0.470 tonnes of CO2 per MWh.
*If pressed:* on CEA's cost basis coal rises from 19.0 to 23.9 percent and non-fossil sits at the 75 percent floor. That basis is now also in real 2024 USD, so the levels compare: 202.6 against 176.2 bn USD a year (182.1 to 225.2 across the RBI inflation band of 2 to 6 percent).

## 5. Robustness

**28. Is the Robust plan cheaper everywhere?** *Owner: Tathagata.* **(Trap.)**
No. It is cheaper at the median requirement: 176 against 213 bn USD a year (real 2024 USD). At the P90 requirement, a separate solve, it costs more, 253 against 236 bn USD a year, because it generates more: it leaves 639 TWh unmet instead of 1,471.
*Never pair the median cost (176) with the P90 shortfall (639). Median numbers: 79.9 percent, 891 Mt, 176 bn USD. P90 numbers: 639 and 1,471 TWh unmet, 253 and 236 bn USD.*

**29. The Robust plan still leaves 639 TWh unmet at P90. Is that robust?** *Owner: Tathagata.*
Robust here means cheaper and cleaner at the median and less short at P90, not adequate at P90. Closing the gap needs land, not speed: with 45 percent of the solar potential usable, the P90 requirement is fully met at the published high build rate, for about 258 bn USD a year. Building twice as fast without more land still leaves 217 TWh unmet.

**30. Where does the plan break?** *Owner: Tathagata.*
At a very low requirement. Below about 2,780 TWh the Robust plan has no feasible solution, because its coal floor (647 TWh) and peaking gas (51 TWh) are fixed amounts, while 75 percent non-fossil allows fossil fuels only a quarter of generation: 698 divided by 0.25 is about 2,790. The Low growth scenario's P5, 2,306 TWh, is below that line. Every policy is feasible at every scenario's median and at the Base scenario's P5, 3,190 TWh.
*If pressed:* the Low carbon policy, with a coal floor of 35 percent, is feasible there (86 bn USD a year). The fix is to let the coal floor bend at low requirement; we did not change the optimisation before the final, so we state the limit, and the dashboard names the levers.

**31. What would a deeper CO2 cut cost?** *Owner: Tathagata.*
Very little. A CO2 cap takes the Robust plan from 891 to 653 Mt a year, 27 percent less, for 0.45 bn USD a year more (real 2024 USD), because nuclear replaces coal. The highest marginal abatement cost is 4.04 USD per tonne. It stops at 653 Mt because the young coal fleet's floor binds, not money.

**32. What if the High growth scenario happens?** *Owner: Tathagata.*
Its median is about 6,580 TWh, above our P90. At that median the Robust plan as set leaves about 812 TWh unmet, at 253 bn USD a year. The lever is the same as at P90: usable solar land. We have not published a High growth land case, so we would show it live on the dashboard rather than quote a number.

## 6. Engineering

**33. Why do you need a GPU, and how fast is it?** *Owner: Gaurav.*
The uncertainty is millions of independent futures. 1,048,576 futures take 11.20 ms on a laptop GPU, against 2.26 s on 16 CPU threads (about 200 times faster) and about 20.0 s on one thread. Caveat: the GPU figure is the kernel time only, without data transfers, the single-thread time is scaled from a smaller run, and all timings vary with machine load.
*If pressed:* CPU and GPU results agree within 0.2 percent; an automated parity test checks this on a GPU machine.

**34. Why C++, CUDA, OpenMP, HiGHS and WebAssembly?** *Owner: Gaurav.*
One code base for every target. The same C++20 compiles to CUDA for the GPU, OpenMP for CPUs and WebAssembly for the browser, so the website runs the very engine that produced the slides. HiGHS is a free, open-source linear-program solver that also compiles to WebAssembly, so the least-cost plan is solved live in the browser. Everything runs on free tools and free hosting.

**35. How is it tested?** *Owner: Gaurav.*
Nine ctest suites: CPU-GPU Monte Carlo parity, allocation, storage, the cost-CO2 trade-off, the India core, the regional split, the price base, the demand features and the landing-page format. GitHub Actions runs a CPU build, the tests, a smoke run with 65,536 futures and the WebAssembly build on every push. The tests check, for example, that the growth decomposition adds up exactly and that the low-requirement limit is where we say it is.
*If pressed:* GitHub's runners have no GPU, so in CI the parity test compares double and single precision on the CPU; the CPU-GPU check runs on our GPU machine.

**36. Why is the browser slower, and do its numbers differ?** *Owner: Gaurav.*
The browser has no GPU and one compute thread, so it runs 65,536 futures and 300 bootstrap replicates instead of 4.2 million and 2,000. A full update takes several seconds. Its medians and ranges agree with the native run within about 0.5 percent, so the last digit can differ from the slides.

**37. Can we reproduce it and see the code?** *Owner: Gaurav.*
Yes. One command, `energy2047` with the repository path, regenerates every output with a fixed seed (20470815) and a counter-based random generator, so runs repeat exactly. Every input has a provenance row in `docs/data_audit.md`. The repository is public on GitHub, with the code, the tests and the documentation.

## 7. Usability and impact

**38. Who would use this, and how?** *Owner: Gaurav.*
Planners at CEA, state utilities and grid operators, and anyone weighing investments in generation or transmission. They change any assumption (income growth, usable land, the coal floor, corridors, build limits) and see the national range, the regional split and the least-cost plan recompute in seconds, in a browser, with nothing to install. Every number traces back to an output file and a source row.

**39. What would you do next? What changed since the midpoint?** *Owner: Tathagata.*
Next: act on the organisers' reply on published figures; an hourly capacity-expansion model, so storage and gas are chosen inside the plan; let the coal floor bend at low requirement; an industrial layer for the steel and aluminium regions; and a refresh with each new NPP year. Since the midpoint the mix gained published build limits, NPP's regional base, committed nuclear and hydro, and peaking gas. The twenty forty-seven forecast was unchanged at the final. Since then, deflating Jammu and Kashmir's current-price GSDP moved the median from 4,703 to 4,688 TWh, a corrected convergence base year moved the regional split, costs moved to real 2024 USD, and a demand-feature layer from official sources replaced the green hydrogen scenario (D14).

## Numbers to memorise

| What | Value |
| --- | --- |
| 2047 median (FY2047-48) | 4,688 TWh; 2.76 times FY2023-24 (1,698 TWh); 4.3 percent a year; 2,990 TWh more |
| 80 percent range (P10 to P90) | 3,471 to 6,408 TWh |
| 90 percent range (P5 to P95), on the dashboard card | 3,190 to 7,021 TWh |
| Futures | 4.2 million (4,194,304) native; 65,536 in the browser |
| Regional medians | North 1,342; West 1,276; South 1,188; East 730; North-East 151 TWh |
| Milestones (median) | FY2030-31 2,262; FY2035-36 2,879; FY2040-41 3,585 TWh |
| Elasticity | 0.84 (0.835, s.e. 0.065), nationally; effective 0.74 (0.742) on state income in the split |
| Convergence of state incomes | about 0.2 percent of the gap a year (lambda 0.0021): relative incomes barely change by 2047 |
| Holdout | FY2024-25 1,726 vs 1,785 (minus 3.3 percent); FY2025-26 1,785 vs 1,776 (plus 0.5 percent) |
| Backtest error | 2.0 percent at 1 year; 4.1 percent at 3 years; 6.4 percent at 5 years |
| Growth shares (central path, 4,589 TWh) | income 87.5; population 14.8; urbanisation 4.8; model error minus 7.2 percent |
| Income growth 1.5 points slower / faster | 3,568 TWh (minus 24 percent) / 6,163 TWh (plus 31 percent) |
| Without the urbanisation prior | 4,438 TWh (minus 5 percent) |
| Demand features (outside the headline) | 44 drivers listed, 11 quantified; Features central 5,810 TWh (plus 23.9 percent); joint median 5,668 TWh (P5 4,130, P95 8,017) |
| Robust plan with the features | 0 unmet at the joint median; 1,635 TWh unmet at the joint P90 (7,404 TWh) |
| CEA NEP, FY2031-32 (gross generation) | 2,666 TWh vs our 2,378 (P75 2,550, P90 2,719) |
| CEA 2026 review, FY2035-36 (gross generation) | about 3,596 BU vs our 2,879 (P95 3,641) |
| Robust plan, at the median | 79.9 percent non-fossil; 891 Mt CO2; 176 bn USD a year (real 2024 USD); 0 unmet |
| Base plan, at the median | 70.0 percent non-fossil; 1,367 Mt CO2; 213 bn USD a year; 0 unmet |
| At the P90 requirement (6,408 TWh) | unmet: Robust 639, Base 1,471 TWh; cost: Robust 253, Base 236 bn USD a year |
| P90 lever | 45 percent usable solar: 0 unmet at the published high case, about 258 bn USD a year |
| Robust mix | solar 50.9; coal 19.0; firmed solar and wind 14.6; hydro 7.4; wind 4.2; nuclear 2.8; gas 1.1 percent |
| Robust mix, TWh | solar 2,390; coal 893; firmed 686; hydro 346; wind 198; nuclear 133; gas 51 |
| Floors | coal 647 TWh (50 percent of FY2023-24 coal); gas 51 TWh (FY2020-21); nuclear 133; hydro 185 TWh |
| Regions | West exports 249, North-East 16, South 6; East imports 155, North 107 TWh |
| Corridors | 271 TWh: West to North 94, West to East 155, South to East 6, North-East to North 16 |
| Generation vs requirement | 4,697 TWh generated for 4,688 required (corridor losses) |
| Storage and land | 448 GW, 3,048 GWh, 6.8 hours; 19,990 km², 0.61 percent of India |
| CO2 cap | 891 to 653 Mt (minus 27 percent) for plus 0.45 bn USD a year; highest 4.04 USD per tonne |
| Where it breaks | no feasible Robust plan below about 2,780 TWh; Low growth P5 is 2,306 TWh |
| Speed (1,048,576 futures) | GPU 11.20 ms; 16 CPU threads 2.26 s; 1 thread about 20.0 s |
| Tests | 9 ctest suites; GitHub Actions on every push |
| Costs | real 2024 USD (the IEA WEO 2025 price year); other price bases converted with official price indices |

## Words to use and words to avoid

| Say | Avoid | Why |
| --- | --- | --- |
| "generation requirement", "energy generated" | "demand", "consumption", "load forecast" | We forecast generation, never consumption. |
| TWh, "terawatt-hours" | installed capacity, or GW, as a forecast | Capacity is not our metric; GW appear only as published limits converted to TWh, and as storage sizes. |
| "twenty forty-seven" for FY2047-48 | "calendar 2047" | NPP reports fiscal years. |
| "the median, 4,688, in a range of 3,471 to 6,408" | "India will need exactly 4,688" | The forecast is a distribution. |
| "primary": NPP and the Renewable India dataset only | "official" or "trusted" for any other source | Every other source is secondary and labelled. |
| The organisers' words, "Use insights from this data source", and "a clarification has been requested" | "the organisers confirmed" or "approved" | The answer is not explicit and is not confirmed. |
| "Base scenario" (growth path) and "Base plan" (policy) | plain "Base" when it is unclear which | The dashboard has a "Base" button in each row. |
| "at the median" with 79.9 percent, 891 Mt, 176 bn USD | 176 bn USD next to 639 TWh unmet | Median costs and P90 shortfalls come from different solves. |
| "less short at P90" | "Robust meets P90" or "adequate at P90" | It still leaves 639 TWh unmet there. |
| 0.84 nationally; 0.74 effective on state income | 0.84 for the regional split | The split carries the urbanisation term's income part. |
| "committed builds" (under construction or sanctioned) | "planned capacity" | They are floors in TWh, not a capacity forecast. |
| "firmed solar and wind with storage" | "a seventh source" | It is solar and wind sold round the clock. |
| CEA gross generation (NEP 2,666; 2026 review 3,596) | comparing with the Electric Power Survey's energy requirement | Those figures are consumption-side. |
| "the GPU kernel time; timings vary with load" | "the browser runs on the GPU" | The browser runs the CPU path on its CPU threads (8 in our test), not the GPU. |
| "the code is public on GitHub" | "the repository is private" | The repository was made public on 20 Nov at the organisers' request. |
| Current numbers (79.9 percent, 891 Mt, 176 bn USD, 639 TWh) | Midpoint numbers (78.3 percent, 993 Mt, 176 bn USD, 638 TWh) or the numbers presented on 21 Nov (79.5 percent, 912 Mt, 178 bn USD, 633 TWh) | The mix changed after the midpoint (D13) and after the final (D14). |
| "Features central" and "the joint median", labelled outside the headline | adding a feature scenario to the 4,688 headline | The headline uses only the two challenge datasets (D14). |
| "captive plants are excluded, as the organisers ruled" | adding captive generation to any number | The ruling allows NPP generation only. |
