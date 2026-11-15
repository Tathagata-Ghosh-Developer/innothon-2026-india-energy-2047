# Research notes for the INNOTHON 2026 deck

Retrieved on 2026-11-15; section F (urbanisation prior, captive share, NPP) added on 2026-11-16. Every quote below was read in the fetched source named with it. Quotes are verbatim and at most 40 words. An ellipsis (...) marks a fragment of a longer sentence. Lines marked **Derived** are our own arithmetic on a fetched data file and are not statements by the publisher. Anything we could not confirm is listed in the UNVERIFIED section at the end and is kept out of both .bib files.

Bib keys point to `refs_web.bib` (web sources) and `refs_academic.bib` (papers). Bibliographic metadata for the papers was checked against the Crossref and OpenAlex records for each DOI.

---

## A. ExxonMobil vision and Global Outlook

**Edition note.** The latest edition is the **2026 Global Outlook** ("Our view to 2050"), published in September 2026. The 2025 edition has been superseded, so the keys below use 2026.

### A1. Edition and scope
- Quote: "The Outlook was published in September 2026."
- Quote: "The Global Outlook is ExxonMobil's view of demand and supply of energy and related products through 2050."
- Source: ExxonMobil, *ExxonMobil Global Outlook: Our view to 2050* (landing page), September 2026. https://corporate.exxonmobil.com/what-we-do/energy-supply/global-outlook . Key: `exxon_outlook_2026`

### A2. Headline framing (Executive summary, p. 2)
- Quote: "The world will change a lot by 2050, but two things will remain constant -- the need for more energy and lower emissions"
- Source: ExxonMobil, *2026 Global Outlook: Executive summary* (PDF, 15 pages, file created 17 Sep 2026), p. 2. https://corporate.exxonmobil.com/-/media/global/files/global-outlook/2026-global-outlook-executive-summary.pdf . Key: `exxon_outlook_2026_summary`

### A3. Population and prosperity as demand drivers
- Quote (Introduction page, 17 Sep 2026): "Growing global population and prosperity have been the biggest factors driving increased demand for energy and products."
  Source: ExxonMobil, Global Outlook, Introduction. https://corporate.exxonmobil.com/publications/global-outlook/introduction . Key: `exxon_outlook_2026_intro`
- Quote (Executive summary, p. 2): "Our Global Outlook projects that demand for energy will increase as economies expand, incomes rise, and living standards improve, even as global emissions peak and decline." Key: `exxon_outlook_2026_summary`
- Quote (p. 3): "The world's population will grow by ~1.5 billion people, almost entirely in developing countries."
- Quote (p. 3): "Global gross domestic product (GDP) will grow ~80%. Developing nations will expand their economies twice as fast as developed nations."
- Quote (p. 3): "Average income per person will rise ~70% globally, improving quality of life."
- Quote (p. 3): "In 2050, developing countries will use 25% more energy than today as their living standards improve and populations grow."
- Quote (p. 3): "Right now, more than 4 billion people live in countries without enough energy to meet modern standards of living."
- Quote (Developing countries page, 17 Sep 2026), fragments: "...has identified 1,000 kWh per person per year as the electricity consumption..." (needed for modern living standards; the page credits the Energy for Growth Hub) and "Our analysis estimates that basic living standards require at least 50 million British thermal units..." (per person per year).
  Source: ExxonMobil, Global Outlook, "Developing countries use more energy". https://corporate.exxonmobil.com/publications/global-outlook/developing-countries-use-more-energy . Key: `exxon_outlook_2026_developing`

### A4. Electricity demand growth
- Quote (Executive summary, p. 2): "Electricity demand will grow ~65% by 2050, driven by improved living standards in developing countries, industrial activity, data centers, and electric vehicles." Key: `exxon_outlook_2026_summary`
- Quote (p. 5): "Global electricity demand is expected to grow 65% by 2050 (vs. 2025), driven by improving living standards in developing countries."
- Quote (p. 6): "Electricity from all sources will grow from ~20% of energy use today to ~30% in 2050, with renewables growing the fastest."
- Quotes (Electricity page): "Generation from solar and wind is projected to grow more than 4x by 2050, meeting ~85% of global power demand growth." Also "Gas power generation will also grow ~25% by 2050" and "Generation from nuclear power is projected to grow ~60% by 2050". Also "The commercial sector (including data centers) is projected to account for ~20% of global electricity demand growth".
  Source: ExxonMobil, Global Outlook, "Electricity demand increases" (September 2026). https://corporate.exxonmobil.com/publications/global-outlook/electricity-demand-increases . Key: `exxon_outlook_2026_electricity`
- Data file ("World" sheet, "Electricity generation (terawatt hours)"): World 32,243.5 TWh in 2025 and 51,863.6 TWh in 2050. The file's own % change column gives 0.6085 (+60.8%).
  Source: ExxonMobil, *2026 Global Outlook data* (XLSX; sheet header "Last update: 14 Sept 2026"). https://corporate.exxonmobil.com/-/media/global/files/global-outlook/2026-global-outlook-data.xlsx . Key: `exxon_outlook_2026_data`

### A5. India in the 2026 Outlook (data file only)
None of the Outlook pages we read and none of the 15 summary pages contain a sentence about India. India appears only as a row in the data file (key `exxon_outlook_2026_data`).
- "World" sheet, energy demand in quadrillion BTU: India 46.59 (2025), 53.85 (2030), 60.00 (2035), 66.29 (2040), 77.60 (2050). The file's % change for 2025 to 2050 is 0.6657 (+66.6%). India's share of world demand goes from 0.0752 to 0.1120 (7.5% to 11.2%).
- **Derived:** India adds 31.0 of the world's 73.3 quadrillion BTU increase from 2025 to 2050, which is about 42% of global energy demand growth in ExxonMobil's projection.
- "Regional" sheet, GDP (2015 US$, trillions): India 3.69 (2025) to 12.50 (2050). The file's % change is 2.386 (+239%).
- "Regional" sheet, energy intensity (thousand BTU per $ GDP): India 12.61 (2025) to 6.21 (2050), a change of -50.8%.
- "Regional" sheet, energy-related CO2 (billion tons): India 3.05 (2025) to 4.42 (2050), +44.7%, while World goes from 36.06 to 29.65, -17.8%. The summary text rounds the global fall to "~20%".
- The file gives no India-specific electricity row. The nearest is Asia Pacific "Memo: electricity demand": 51.51 (2025) to 91.69 (2050) quadrillion BTU, +78.0%.
- Older ExxonMobil commentary that names India (Chris Birdsall, Director, Energy & Economics, 28 Aug 2023), fragment: "...non-OECD countries such as India, China, Ethiopia, Niger, and many others in Africa, Asia, and Latin America."
  Source: ExxonMobil, "The power of Two in understanding the future". https://corporate.exxonmobil.com/news/viewpoints/the-power-of-two-in-understanding-the-future . Key: `exxon_power_of_two_2023`

### A6. Emissions and the "energy transition" framing
- Quote (Executive summary, p. 7): "By 2050, CO2 emissions are expected to fall ~20%, driven by efficiency improvements and a shift toward lower-emission solutions." Key: `exxon_outlook_2026_summary`
- Quote (p. 2): "Solving the "and" equation -- meeting rising energy demand and reducing emissions -- does not have to be a zero sum challenge. With the right policy and advancements in technology, the world can do both."
- Quote (p. 13): "Doing both -- increasing energy supply and reducing emissions -- is the only reasonable plan."
- Quote (landing page): "Increasing global energy supply AND reducing emissions is not only possible - it's essential." The page also has the chapter title "Affordability will drive the pace of any transition". Key: `exxon_outlook_2026`
- Quote (p. 6): "Lower-emission technologies like hydrogen, carbon capture and storage, and biofuels will become more important for industry and commercial transportation; however, the public policy needed to scale these technologies is lagging."

### A7. Projection versus scenarios (useful for the probabilistic pitch)
- Quote (Executive summary, p. 13): "The Outlook is a projection, helping to inform decisions by highlighting the future that appears most probable. It is not an endorsement of a particular outcome."
- Quote (p. 13): "Because the energy system will be influenced by many uncertainties, no single pathway can be reasonably predicted."
- Quotes (Projections vs. scenarios page), fragments: "A projection like the Outlook starts with current factors such as public policy and commercially available technology..." and "In contrast, many scenarios start with a hypothetical outcome and work backward..."
  Source: ExxonMobil, Global Outlook, "Projections vs. scenarios". https://corporate.exxonmobil.com/publications/global-outlook/projections-vs-scenarios . Key: `exxon_outlook_2026_projections`

### A8. Corporate purpose and vision
- Quote, purpose: "Create sustainable solutions that improve quality of life and meet society's evolving needs."
- Quote, vision: "Lead industry in innovations that advance modern living and a net-zero future."
  Source: ExxonMobil, "Who we are" (no page date shown). https://corporate.exxonmobil.com/who-we-are . Key: `exxon_who_we_are`
- Quote: "We're focused on meeting society's evolving needs for energy and essential products and reducing greenhouse gas emissions."
  Source: ExxonMobil, "Our approach" (no page date shown). https://corporate.exxonmobil.com/who-we-are/our-approach . Key: `exxon_our_approach`

### A9. Low Carbon Solutions (carbon capture, hydrogen, lithium)
- Quote: "We're helping accelerate society's path to net zero by scaling up emissions solutions for key industries"
  Source: ExxonMobil, "Delivering industrial solutions". https://corporate.exxonmobil.com/what-we-do/delivering-industrial-solutions . Key: `exxon_industrial_solutions`
- Quote: "ExxonMobil is a global leader in CCS – a position further enhanced by our acquisition of Denbury Inc."
- Quote: "We have cumulatively captured more CO2 than any other company – 120 million metric tons –" (the page cites 2023 GCCSI data).
  Source: ExxonMobil, "Carbon capture and storage" (latest news item on the page dated 22 Sep 2026). https://corporate.exxonmobil.com/what-we-do/delivering-industrial-solutions/carbon-capture-and-storage . Key: `exxon_ccs`
- Quote: "We are scaling up production of low-carbon hydrogen to reduce CO2 emissions in our own facilities"
  Source: ExxonMobil, "Hydrogen". https://corporate.exxonmobil.com/what-we-do/delivering-industrial-solutions/hydrogen . Key: `exxon_hydrogen`
- Quotes: "ExxonMobil plans to become a leading supplier of lithium" and "By the first half of the 2030s, we aim to produce enough lithium" "...to supply the manufacturing needs of about 1 million EVs a year."
  Source: ExxonMobil, "Lithium". https://corporate.exxonmobil.com/what-we-do/delivering-industrial-solutions/lithium . Key: `exxon_lithium`

---

## B. ExxonMobil in India

- Quote: "ExxonMobil has been a key partner in India's energy journey for over three decades"
- Quote: "ExxonMobil brought the first supplies of LNG to India in the early 2000s."
- Quote: "The company is now a major LNG supplier to the country, helping to advance India's transition to a gas-based economy."
- Quote: "ExxonMobil's business and technology centers in Bangalore provide critical support to the Company's global operations."
  Source: ExxonMobil, "India: Operations" (no page date shown; it links an article dated 5 Oct 2026). https://corporate.exxonmobil.com/locations/india/operations . Key: `exxon_india_operations`
- Quote: "ExxonMobil said today it is investing nearly INR 900 crore (USD $110 million) to build a lubricant manufacturing plant"
- Quote: "...at the Maharashtra Industrial Development Corporation's Isambe Industrial Area in Raigad."
- Quote: "...the plant will have the capacity to potentially manufacture 159,000 kiloliters of finished lubricants annually"
- Quote: "It is expected to begin operations by the end of 2025."
  Source: ExxonMobil India newsroom, "ExxonMobil to build lubricant manufacturing plant in India", 31 Mar 2023. https://corporate.exxonmobil.com/locations/india/newsroom/03312023_exxonmobil-to-build-lubricant-manufacturing-plant-in-india . Key: `exxon_india_lubricant_plant_2023`
- Whether the plant has started operating is UNVERIFIED. The operations page still captions it "ExxonMobil's upcoming lubricant plant in Maharashtra".

---

## C. State of practice in Indian electricity planning, with IEA and ExxonMobil for contrast

### C1. National Electricity Plan 2022-32, Vol. I Generation (CEA): demand input
- Quote: "...the projected All India peak electricity demand and electrical energy requirement is 277.2 GW and 1907.8 BU for the year 2026-27 and 366.4 GW and 2473.8 BU for the year 2031-32 as per 20th Electric Power Survey (EPS) Demand projections."
- Quote: "The Energy Requirement & Peak Demand are inclusive of the impact due to increased adoption of Electric Vehicles, Installation of Solar roof tops, Production of Green hydrogen, Saubhagya scheme etc."
- Other figures in the same release: likely installed capacity of 609,591 MW (2026-27) and 900,422 MW (2031-32). Non-fossil capacity share is "likely to increase to 57.4% by the end of 2026-27 and may likely to further increase to 68.4% by the end of 2031-32". Average emission factor falls to 0.548 (2026-27) and 0.430 kg CO2/kWh net (2031-32). The plan is notified "once in five years" under section 3(4) of the Electricity Act, 2003.
  Source: Press Information Bureau (Ministry of Power), "Central Electricity Authority notifies the National Electricity Plan for the period of 2022-32", 31 May 2023. https://pib.gov.in/PressReleasePage.aspx?PRID=1928750 . Key: `pib_nep_2023`

### C2. NEP sensitivity cases
- Quote: "This scenario assumes a 5 per cent increase in both the estimated peak demand..." (as well as the energy requirement for 2026-27 and 2031-32).
- Quote: "...there will be no change in the resource mix and the total installed power capacity by 2026-27..."
- Quote: "...a modest increase of 3 GW is estimated in the coal-based power capacity..." (2031-32, higher-demand case compared with the base case).
- Quote: the plan "presents five different scenarios to assess the source-wise power mix".
  Source: Power Line magazine, "Green Roadmap: CEA prioritises renewables in the National Electricity Plan", 3 Jul 2023 (article signed Khushboo Goyal). https://powerline.net.in/2023/07/03/green-roadmap-cea-prioritises-renewables-in-the-national-electricity-plan/ . Key: `powerline_nep_scenarios_2023`
- Our reading: the NEP takes one demand path from the 20th EPS as its base case. On the demand side, the only alternative described in these sources is a flat +5% case. The other scenarios vary the supply mix. We did not see any probability statement in these sources.

### C3. 20th Electric Power Survey (CEA, November 2022): demand method
- Quote: "Based on the way forward suggested by the committee, electricity demand projections have been done for discoms/states/UTs/regions and for the country through partial end use method (PEUM) for the years 2021-22 to 2031-32."
- Quote: "Perspective electricity demand projection has also been done for the states/UTs/regions and for the country for the years 2036-37 and 2041-42."
- Quote: "Factors like reduction in transmission and distribution (T&D) losses, energy efficiency improvement measures, production of green hydrogen, penetration of electric vehicles, roof-top solar, solar pumps, among others, have appropriately been factored in the electricity demand forecast."
  Source: India Environment Portal record of the CEA report "Report on twentieth electric power survey of India (Volume-1)", publisher Central Electricity Authority, dated 17/11/2022. https://indiaenvironmentportal.org.in/reports-and-documents/report-on-twentieth-electric-power-survey-of-india-volume-1 . Key: `iep_20th_eps_2022`
- Quote, on how the 2036-37 and 2041-42 values were made: "...by extrapolating the figures of electricity consumption of states/UTs with the application of suitable growth rates."
  Source: Power Line magazine, "Report on Twentieth Electric Power Survey of India: CEA", 18 Nov 2022. https://powerline.net.in/2022/11/18/report-on-twentieth-electric-power-survey-of-india-cea/ . Key: `powerline_20th_eps_2022`

### C4. IEA World Energy Outlook scenario framing
- Quote: "None of the individual WEO scenarios should be regarded as a forecast"
- Quote: "The Current Policies Scenario (CPS) and the Stated Policies Scenario (STEPS) are exploratory"
- The page says all scenarios share "the same pathways for future population and economic growth." It describes STEPS (fragment) as "A scenario which builds on a broader reading of the policy landscape than the CPS" and NZE (fragment) as "A scenario which maps out a pragmatic but ambitious global pathway for the energy sector". For the APS, as used up to WEO-2024 (fragment): "...the APS assumes that all climate commitments made by governments and industries around the world..."
  Source: IEA, "Understanding WEO scenarios" (Global Energy and Climate Model documentation, 2025). https://www.iea.org/reports/world-energy-model/understanding-weo-scenarios . Key: `iea_weo_scenarios`
- Quote: "The WEO this year will not include the Announced Pledges Scenario (APS)."
- Quote: "None of the scenarios in the WEO are a forecast."
- Quote: "A framework that includes multiple scenarios allows us to explore the consequences of different policy choices."
- WEO-2025 has two exploratory scenarios (CPS and STEPS) and two normative ones (NZE and a new ACCESS scenario).
  Source: IEA commentary, "Scenarios in the World Energy Outlook 2025", Laura Cozzi and Tim Gould, 5 Nov 2025. https://www.iea.org/commentaries/scenarios-in-the-world-energy-outlook-2025 . Key: `iea_weo2025_scenarios_commentary`
- Quote, on global electricity demand: "It rises by around 40% to 2035 in both the CPS and the STEPS, and by more than 50% in the NZE Scenario."
- Quote, fragment: "Energy market dynamics are increasingly shaped by a group of emerging economies, led by India and Southeast Asia..."
  Source: IEA, *World Energy Outlook 2025*, Executive summary, 12 Nov 2025. https://www.iea.org/reports/world-energy-outlook-2025/executive-summary . Key: `iea_weo2025_execsum`
- Note: we found no sign that a WEO-2026 had been released by 2026-11-15, so WEO-2025 is the latest edition used here.

### C5. Contrast in one line (our synthesis, not a quote)
CEA uses one deterministic demand path from end-use and trend methods, plus a +5% sensitivity. The IEA uses several conditional scenarios with no probabilities attached ("not a forecast"). ExxonMobil gives a single "most probable" projection. None of the sources we read puts probability bands on electricity demand, which is the gap a probabilistic forecast fills.

---

## D. India facts to cross-check

### D1. Per-capita electricity consumption (government figure)
- Quote: "Per capita electricity consumption in India has also risen to 1,460 kilowatt-hours in 2024-25, registering an increase of 503 kilowatt-hours (about 52.6%) compared to 957 kilowatt-hours in 2013-14."
- Quote: "During FY 2025-26, India successfully met a peak power demand of 242.49 gigawatts (GW)."
- Quote: "As of January 2026, India's total installed power generation capacity stands at 520.51 GW."
  Source: Press Information Bureau backgrounder, "India's Power Sector: Progress, Reform, and the Road Ahead", 18 Mar 2026. https://static.pib.gov.in/WriteReadData/specificdocs/documents/2026/mar/doc2026318829301.pdf . Key: `pib_power_sector_2026`
- Caution on vintages. A PIB document titled "Major Highlights (as on 31.01.2025)" (file created 21 Feb 2025) lists per-capita consumption of 957 kWh (2014), 1395 kWh (2024) and 1538 kWh (2025), with targets of 1636 kWh (2026) and 2109 kWh (2030). The 1538 figure is an interim value; the March 2026 backgrounder gives the final 2024-25 value of 1,460 kWh. https://static.pib.gov.in/WriteReadData/specificdocs/documents/2025/feb/doc2025221507201.pdf . Key: `pib_power_highlights_2025`
- Generation, 2025-26. Quote: "India's total power generation during 2025-26 (up to March 2026) reached 1,845.921 BU."
- Quote: "The share of non-fossil fuels in total generation reached 29.2% in 2025-26 (538.97 BU)."
  Source: Press Information Bureau (Ministry of New and Renewable Energy), "India Ranks third globally in Renewable Energy Installed Capacity: Shri Pralhad Joshi", 8 Apr 2026. https://www.pib.gov.in/PressReleasePage.aspx?PRID=2250039 . Key: `pib_re_2026`

### D2. Net zero by 2070 and 500 GW of non-fossil capacity by 2030
- Quote: "First - India will reach its non-fossil energy capacity to 500 GW by 2030."
- Quote: "And fifth- by the year 2070, India will achieve the target of Net Zero."
  Source: Press Information Bureau (Prime Minister's Office), "National Statement by Prime Minister Shri Narendra Modi at COP26 Summit in Glasgow", 1 Nov 2021. https://pib.gov.in/PressReleasePage.aspx?PRID=1768712 . Key: `pib_cop26_2021`
- Quote: "...India now stands committed to reduce Emissions Intensity of its GDP by 45 percent by 2030, from 2005 level and achieve about 50 percent cumulative electric power installed capacity from non-fossil fuel-based energy resources by 2030."
- Quote: "The update is also a step towards achieving India's long term goal of reaching net-zero by 2070."
  Source: Press Information Bureau (Cabinet), "Cabinet approves India's Updated Nationally Determined Contribution to be communicated to the United Nations Framework Convention on Climate Change", 3 Aug 2022. https://pib.gov.in/PressReleasePage.aspx?PRID=1847812 . Key: `pib_ndc_2022`

### D3. Viksit Bharat 2047
- Quote: "And that dream is this: when the country completes 100 years of Independence, we will build a 'Viksit Bharat' (Developed India) in 2047."
- Quote: "By 2047, we are working towards a target of 100 GW of nuclear power capacity."
- Quote: "Twelve years ago, the country's solar energy capacity was only 2 GW—just 2 GW." and "Today, we have achieved a capacity of 160 GW."
  Source: Press Information Bureau (Prime Minister's Office), "Key highlights of Prime Minister Shri Narendra Modi's address from the ramparts of the Red Fort on the occasion of the 80th Independence Day", 15 Aug 2026. https://www.pib.gov.in/PressReleasePage.aspx?PRID=2299890 . Key: `pib_independence_day_2026`

### D4. Installed renewable and non-fossil milestones
- Quote: "India has achieved a landmark in its energy transition journey by reaching 50% of its installed electricity capacity from non-fossil fuel sources—five years ahead of the target set under its Nationally Determined Contributions (NDCs) to the Paris Agreement."
  Source: Press Information Bureau (Ministry of New and Renewable Energy), "India's Renewable Rise: Non-Fossil Sources Now Power Half the Nation's Grid", 14 Jul 2025. https://www.pib.gov.in/PressReleasePage.aspx?PRID=2144627 . Key: `pib_nonfossil_2025`
- Quote: "...a total of 283.46 GW of capacity from non-fossil fuel sources has been installed in the country as on 31.03.2026."
- Quote: "...India achieved a total non-fossil capacity addition of 55.3 GW during FY 2025–26."
  Source: PIB, 8 Apr 2026 (see D1). Key: `pib_re_2026`

---

## E. Academic references (`refs_academic.bib`)

| Key | Reference | DOI | How verified |
|---|---|---|---|
| `salmon2011parallel` | Salmon, Moraes, Dror, Shaw (2011). Parallel random numbers: as easy as 1, 2, 3. SC '11, ACM, pp. 1-12 | 10.1145/2063384.2063405 | Crossref record (title, subtitle "as easy as 1, 2, 3", event SC '11) |
| `huangfu2018parallelizing` | Huangfu, Hall (2018). Parallelizing the dual revised simplex method. Math. Prog. Comp. 10(1):119-142 | 10.1007/s12532-017-0130-5 | Crossref; HiGHS site: "If you use HiGHS in an academic context, please acknowledge this and cite the following article" (https://highs.dev/, key `highs_website`) |
| `gardner1985forecasting` | Gardner, McKenzie (1985). Forecasting trends in time series. Management Science 31(10):1237-1246 | 10.1287/mnsc.31.10.1237 | Crossref; abstract: "This paper develops an exponential smoothing model designed to damp erratic trends." |
| `golub1973differentiation` | Golub, Pereyra (1973). The differentiation of pseudo-inverses and nonlinear least squares problems whose variables separate. SIAM J. Numer. Anal. 10(2):413-432 | 10.1137/0710036 | Crossref |
| `cameron2008bootstrap` | Cameron, Gelbach, Miller (2008). Bootstrap-based improvements for inference with clustered errors. Rev. Econ. Stat. 90(3):414-427 | 10.1162/rest.90.3.414 | Crossref; abstract: "We investigate inference using cluster bootstrap-t procedures that provide asymptotic refinement." |
| `barro1992convergence` | Barro, Sala-i-Martin (1992). Convergence. J. Political Economy 100(2):223-251 | 10.1086/261816 | Crossref and OpenAlex list only Barro as author. The RePEc record lists "Robert Barro and Xavier Sala-i-Martin" (https://econpapers.repec.org/RePEc:ucp:jpolec:v:100:y:1992:i:2:p:223-51, key `repec_barro_1992`) |
| `bates1969combination` | Bates, Granger (1969). The combination of forecasts. Operational Research Quarterly 20(4):451-468 | 10.1057/jors.1969.103 | Crossref lists the journal under its later name, Journal of the Operational Research Society. The JSTOR record (10.2307/3008764) lists it as "OR". |
| `lempert2003shaping` | Lempert, Popper, Bankes (2003). Shaping the Next One Hundred Years: New Methods for Quantitative, Long-Term Policy Analysis. RAND | 10.7249/MR1626 | Crossref (publisher RAND Corporation, ISBN 0833032755) |
| `dargay2007vehicle` | Dargay, Gately, Sommer (2007). Vehicle ownership and income growth, worldwide: 1960-2030. Energy Journal 28(4):143-170 | 10.5547/ISSN0195-6574-EJ-Vol28-No4-7 | Crossref. The Gompertz form is confirmed in the authors' BIEE 2006 conference slides: "We use an S-shaped Gompertz function to model this relationship." (key `dargay_biee_2006`) |
| `yoo2010electricity` | Yoo, Lee (2010). Electricity consumption and economic growth: a cross-country analysis. Energy Policy 38(1):622-625 | 10.1016/j.enpol.2009.05.076 | Crossref. The RePEc abstract reports "a statistically significant inverted-U-shaped relationship between per-capita consumption of electricity and per-capita income" (key `repec_yoo_2010`) |
| `wolfram2012energy` | Wolfram, Shelef, Gertler (2012). How will energy demand develop in the developing world? J. Econ. Perspectives 26(1):119-138 | 10.1257/jep.26.1.119 | Crossref. Abstract: "Within a country, the adoption of energy-using assets typically follows an S-shaped pattern" and "above a second threshold, increases in ownership level off." |

The last two are the requested peer-reviewed papers on electricity or energy demand saturation with income. Yoo and Lee use a cross-country panel of per-capita electricity. Wolfram, Shelef and Gertler describe S-shaped appliance and vehicle adoption with income.

Added on 2026-11-16 (details in section F):

| Key | Reference | DOI | How verified |
|---|---|---|---|
| `liddle2013population` | Liddle (2013). Population, affluence, and environmental impact across development: evidence from panel cointegration modeling. Environmental Modelling & Software 40:255-266 | 10.1016/j.envsoft.2012.10.002 | Crossref record; full text read in the author version (MPRA 52088) |
| `shahbaz2016role` | Shahbaz, Mallick, Mahalik, Sadorsky (2016). The role of globalization on the recent evolution of energy demand in India: implications for sustainable development. Energy Economics 55:52-68 | 10.1016/j.eneco.2016.01.013 | Crossref record; abstract on IDEAS; full text read in the author version (MPRA 69127) |
| `holtedahl2004residential` | Holtedahl, Joutz (2004). Residential electricity demand in Taiwan. Energy Economics 26(2):201-224 | 10.1016/j.eneco.2003.11.001 | Crossref record; publisher's typeset PDF (journal page numbers) read on a third-party mirror |
| `kunsch1989jackknife` | Kunsch (1989). The jackknife and the bootstrap for general stationary observations. Annals of Statistics 17(3):1217-1241 | 10.1214/aos/1176347265 | Crossref record (title, author, volume, issue, pages). Cited for the moving-block bootstrap; no quote used |

---

## F. Urbanisation prior (gamma) for the India-only core

Read on 2026-11-16. The model's prior is gamma ~ N(1.0, 0.5), truncated at zero (`outputs/india_fit.csv`, row `gamma_mean`). The three papers below are the basis. None of them measures exactly our quantity (the effect of urban share on total electricity generation per person in India), so each one comes with a caveat. That is why the prior is wide and why the deck always shows the gamma = 0 run beside it.

### F1. Liddle (2013), key `liddle2013population`
- Crossref: https://api.crossref.org/works/10.1016/j.envsoft.2012.10.002 (no issue number). Full text read in the author version, MPRA paper 52088, https://mpra.ub.uni-muenchen.de/52088/1/MPRA_paper_52088.pdf, which states "Published in Environmental Modelling & Software, 40 (2013), 255-266." Pages below other than the abstract are MPRA page numbers; the journal page for Table 6 was not reachable (ScienceDirect returned 403).
- Quote (abstract; journal p. 255, MPRA p. 1): "This paper analyzes urban population's and affluence's (GDP per capita's) influence on environmental impact in developed and developing countries by taking as its starting point the STIRPAT framework."
- Quote (MPRA p. 16): "Thus, for a population variable we consider urban population, which is constructed by multiplying a country's population by the share living in urban areas (as described in Table 1)."
- Quote (MPRA p. 24), fragment: "...the elasticity for urban population in poor countries would be essentially one ... reflects the fact that rural populations in poor countries would have virtually no access to electricity..."
- Number used: MPRA p. 36, Table 6, "Long-run elasticities estimates and 99% confidence intervals (C.I.) from FMOLS for residential electricity as the dependent variable", row "Poor", column "Urban population": 1.022 (s.e. 0.132), 99% interval [0.680, 1.364]. Table A.1 (MPRA p. 38) lists India (IND) in the Poor panel of 37 countries.
- Caveats: the regressor is log urban population (urban share times population), not the urbanisation rate; the dependent variable is total residential electricity, not per person; the panel is called "Poor"; FMOLS on 1971-2007.

### F2. Shahbaz, Mallick, Mahalik and Sadorsky (2016), key `shahbaz2016role`
- Crossref: https://api.crossref.org/works/10.1016/j.eneco.2016.01.013 (no issue number; RePEc lists 55(C)). Abstract read on IDEAS, https://ideas.repec.org/a/eee/eneeco/v55y2016icp52-68.html ; full text read in the author version, MPRA paper 69127, https://mpra.ub.uni-muenchen.de/69127/1/MPRA_paper_69127.pdf . Pages other than the abstract are MPRA page numbers.
- Quote (abstract; journal p. 52): "Furthermore, while financial development is negatively related to energy consumption, economic growth and urbanization are the key factors leading to increased energy demand in the long run."
- Quote (MPRA p. 27): "A 1 per cent increase in urban population leads to a 0.5649-0.7999 per cent increase in energy use in India."
- Quote (MPRA p. 14), fragment: "ln ECt is the natural log of energy consumption per capita ... lnUt is the natural log of urban population per capita"
- Number used: MPRA p. 29, "Table-5: Long and Short Runs Results", long-run row ln Ut: 0.5806, 0.5649, 0.6935 and 0.7999 across the four globalisation models (economic, social, political, overall), all significant.
- Caveats: total energy use, not electricity; U is urban population per person (in effect the urban share); ARDL on 1971-2012.

### F3. Holtedahl and Joutz (2004), key `holtedahl2004residential`
- Crossref: https://api.crossref.org/works/10.1016/j.eneco.2003.11.001 . Read in the publisher's typeset PDF (journal page numbers) on a third-party mirror, https://static.aminer.org/pdf/PDF/000/304/411/forecasting_the_consumption_for_electricity_in_taiwan.pdf .
- Quote (abstract, p. 201): "We have used a proxy variable, urbanization, to capture economic development characteristics and changes in electricity-using capital stocks not explained by income. The variable provides significant explanatory power to the model both in the short- and long-run."
- Quote (p. 216): "An increase of 1% in urbanization leads to nearly a 4% increase in residential electricity consumption."
- Quote (p. 217): "The short-run vs. long-run elasticities are 0.23 vs. 1.04 for income and 1.61 vs. 3.91 for urbanization."
- Quote (p. 212), fragment: "...the variables are transformed to natural logarithms with the exception of the urbanization variable, since it is already in percentage terms."
- Numbers used: p. 216, equation (13), unrestricted cointegrating vector, urbanisation coefficient 3.31 (also Table 3, p. 215); equation (14), restricted, 3.91. Because the urbanisation variable is in levels and consumption in logs (p. 212), we read these as semi-elasticities; the paper itself calls them elasticities.
- Caveats: residential electricity in Taiwan, about 1956-1995; p. 205 defines urbanisation as the share of people in urban areas above 100,000 people, not the census urban share.

### F4. From the three papers to the prior (our reasoning, not a quote)
Residential use is about a quarter of India's electricity (`docs/methodology.md`, section 2.3), and the papers' estimates range from about 0.6 (total energy, India) to about 1.0 (residential electricity, poor countries) as elasticities, and 3.3 to 3.9 as semi-elasticities (residential, Taiwan). A mean of 1.0 with s.d. 0.5 covers that range for total electricity. The data cannot pin gamma down (urban share is in neither challenge dataset and moved in step with income), so the deck reports the gamma = 0 run: 4,647 TWh in FY2047-48 against 4,894 TWh with the prior (`outputs/sensitivity_2047.csv`).

### F5. Captive generation, CEA General Review 2025 (key `cea_general_review_2025`)
- Table 1.6, "Growth of Electricity Generation From Captive Power Plants...", PDF page 31: 223,903 GWh in 2023-24. Recorded when the table was extracted into `data/raw/generation/national/cea_national_generation_fy.csv` (`data/raw/generation/SOURCES.md`, item 4).
- Derived: 223,903 / 1,698,030 GWh of NPP utility generation in FY2023-24 = 13.2% (`outputs/india_fit.csv`, row `captive_share` = 0.13186).
- The 2025 report could not be re-opened from this session on 2026-11-16 (CEA links returned 404), so the table number and value rest on the repository's extraction.

### F6. National Power Portal (keys `npp_dashboard`, `npp_18col`, `npp_merit`)
All endpoints, coverage, validation and caveats are documented in `data/raw/npp/SOURCES.md` (accessed 2026-11-15). Facts used in the deck:
- The dashboard chart's all-India series covers FY1946-47 to FY2023-24 and "The API returns nothing after FY2023-24."
- The 18-column reports and daily reports cover conventional stations above 25 MW; both print "It excludes generation from RES".
- Renewable generation exists on NPP only in the dashboard chart (all-India, to FY2023-24) and in the Merit real-time series (all-India solar and wind, from 2025-09-18); "No region or state split exists on NPP for either."
- Derived (`data/raw/npp/npp_region_generation_fy.csv`, FY2025-26 totals, Bhutan import excluded): Northern 24.9%, Western 36.0%, Southern 20.1%, Eastern 17.7%, North-Eastern 1.3% of 1,529,525 GWh.
- Derived (`outputs/holdout.csv` and `apps/energy2047.cpp`): NPP renewable generation grew 14.1% a year from FY2018-19 (97,924 GWh) to FY2023-24 (189,490 GWh); the holdout extends it at that rate.

---

## UNVERIFIED or not obtained (kept out of both .bib files)

1. **NEP 2022-32 full text.** The CEA PDF links returned 404, and cea.nic.in could not be reached from this session. As a result, the name of the generation-planning model, the details of the five supply scenarios, and whether the NEP contains any probabilistic analysis are UNVERIFIED. The NEP facts above come from PIB and Power Line. Please download the gazette PDF from CEA and check the demand chapter.
2. **Definition of PEUM** as a "combination of time series analysis and End Use Method". This wording appeared only in a search-engine summary of the CEA 20th EPS PDF, which we could not open. It is UNVERIFIED.
3. **20th EPS all-India figures for 2036-37 and 2041-42.** Not obtained, because the tables in the Power Line article are images.
4. **Status of the Raigad (Isambe) lubricant plant**, that is, whether it has started operating. UNVERIFIED.
5. **India-specific electricity demand in the ExxonMobil Outlook.** None found; the data file stops at Asia Pacific for electricity.
6. **Opening date and headcount of the Bengaluru technology centre.** Not documented in the pages we fetched.
7. **IEA India projections in WEO-2025**, for example a claim that India's electricity use roughly doubles by 2035. Seen only in a search summary, not fetched. UNVERIFIED.
8. **Medlock and Soligo (2001)**, Energy Journal 22(2):77-105, DOI 10.5547/ISSN0195-6574-EJ-Vol22-No2-4. Its existence was verified, but it covers energy demand by sector rather than electricity saturation, so it is optional and left out of the .bib.
