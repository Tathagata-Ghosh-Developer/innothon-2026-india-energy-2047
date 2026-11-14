# Techno-economic parameters: sources and extraction notes

This folder holds the techno-economic inputs for the 2047 generation allocation model.
All files were downloaded or read on **2026-11-15**.
Every value traces to a document that was fetched and read; nothing was filled from memory.
Where a value could not be found, the cell is blank and the gap is listed under "Not found" at the end.

## Compliance with the generation-only rule

Forecasts and comparisons use energy generated (TWh, BU, GWh) only.
Installed capacity (MW, GW) appears in these tables for one purpose: to turn resource potentials, pipelines and policy targets into energy.
Each such conversion is explicit, in the form

    energy (TWh per year) = capacity (GW) x capacity factor x 8.76

and the capacity factor used is named in the row (for example the two `implied_generation_twh_*` columns of `existing_fleet_generation.csv`).
Rows that are capacity-based are marked as such (`basis` column of `policy_targets.csv` and `offshore_wind.csv`, notes elsewhere).
Capacity factors themselves are ratios of energy to capacity and are provided only for this conversion.

## Conventions

* Fiscal year (FY) runs April to March and is written `2023-24`.
* 1 BU = 1 TWh = 1,000 GWh = 1,000 MU.
* A value computed by us (a ratio, a median, a unit change) says so in `notes`, with the formula.
* Several old CEA links (`cea.nic.in/wp-content/uploads/...`) now return a CEA "page not found" page after the site redesign.
  For those documents the original CEA URL is kept in `source_url`, and the Internet Archive snapshot that supplied the file is named in `notes`.
* Raw copies of every document are in `raw/` (table at the end). They are kept as downloaded and were read only as data.
* Helper scripts are kept outside the repository, because the project code is C++.
* Numeric columns are plain numbers, except a few `value` cells in `policy_targets.csv` that hold a published range such as `7350-7700`; a parser should split or skip them.

## Files

| File | Rows | Content |
|---|---|---|
| `capacity_factors.csv` | 358 | CUF and PLF by source, region, state and unit, from CEA, DAE, CERC, PIB and IEA, plus observed CUF derived from Ember (CEA and MNRE based) |
| `lcoe.csv` | 119 | Discovered tariffs, regulated tariffs, capital costs and cost projections |
| `emission_factors.csv` | 46 | CEA CO2 Baseline Database v21 grid and fuel factors, NEP projections, lifecycle factors |
| `existing_fleet_generation.csv` | 45 | Coal retirement list, coal fleet base, under-construction coal and its commissioning schedule, life-extension plan |
| `transmission.csv` | 61 | Inter-regional transfer capability by corridor (current and planned), regional transmission losses, T&D loss |
| `nuclear_pipeline.csv` | 25 | Nuclear plants under construction, sanctioned and planned, with state, region and expected commissioning; 100 GW target |
| `offshore_wind.csv` | 36 | Offshore potential, zones, targets, auction trajectory, VGF scheme |
| `policy_targets.csv` | 93 | National targets and official projections, each marked capacity-, generation- or emissions-based |

---

## capacity_factors.csv

Columns: `source_tech, geography_level, geography, region, metric, period, value_pct, low_pct, high_pct, n_years, basis, quality_flag, source_name, source_url, source_location, notes`.
`metric` tells the kind of factor: planning assumption, reported PLF, normative CUF, design-energy CUF, derived observed CUF, or projection.
All values are percentages.

**CEA National Electricity Plan 2022-32, Volume I Generation (May 2023).**
Annexure 5.1 (printed page 5.28, PDF page 154) gives the region-wise CUFs that CEA used in its capacity expansion model: solar for existing plants (NR 18.49, WR 19.54, SR 19.76, ER 17.94, NER 16.67), solar for planned or candidate plants (NR 22.21-24.5, WR 23.47, SR 23.74, ER 21.55, NER 20.02), onshore wind (NR 24.36, WR 27.16, SR 23.93; none for ER and NER) and offshore wind (WR 36.47, SR 57.49).
The page was rendered to an image and read to confirm the column alignment, because the text layer splits the cells.
The same annexure gives availability assumptions (coal 85 existing, 88 candidate; gas 90; nuclear 68; biomass 60), which are upper bounds on dispatch, not CUFs.
Table 5.14 and the conclusions (PDF pages 152-153) give the projected coal PLF of 58.4% in 2026-27 and 58.7% in 2031-32.
Exhibit 7.9 (printed page 7.16, PDF page 206) gives gas PLF for 2017-18 to 2021-22, read from the chart data labels.
The CEA URL now fails, so the file was taken from the Internet Archive copy of that URL (snapshot 20261007075917).
For a regional model, the "planned/candidate" solar and the wind values are the best official central values for new capacity.

**CEA Growth of Electricity Sector in India 1947-2025.**
Chart 13 (PDF page 35) gives the all-India PLF of coal and lignite stations for each year from 1985-86 to 2024-25 (40 values).
The labels were read from the PDF text layer and checked against a rendering of the chart.
The latest values are 64.15 (2022-23), 69.09 (2023-24) and 69.45 (2024-25).

**CEA General Review 2025 (data for FY2023-24), Table 7.6** (PDF page 125).
This table is printed as an image, so it was rendered and transcribed by eye.
It gives the 2023-24 PLF of coal/lignite stations ("Thermal") by state and by ownership (central, state, private), and of nuclear stations by state (Rajasthan 68.11, Uttar Pradesh 74.38, Gujarat 89.30, Maharashtra 65.97, Karnataka 97.06, Tamil Nadu 69.10; all India 74.70).
All-India coal PLF was 75.09 (central), 64.70 (state) and 67.65 (private).

**Department of Atomic Energy, Annual Report 2024-25** (PDF page 34).
The NPCIL fleet PLF was 85% in 2023-24 and 87% in 2024-25, with availability factors of 85% and 88%, for reactors in commercial operation and excluding units under long shutdown.
**DAE Lok Sabha Starred Question 226 (11.12.2024), Annexure-A**, gives unit-wise PLF for April to October 2024; these 24 rows are seven-month values, and units under refurbishment or shutdown show 0 and are flagged `unit_not_operating`.
The new 700 MW PHWRs KAPS-3 and KAPS-4 ran at 70.43% and 67.14% in that period.

**CERC RE Tariff Order for FY 2026-27 (Petition 12/SM/2026, 23.08.2026)**, paragraphs 39-41 (PDF pages 25-26).
Normative CUF for small hydro is 45% in the Himalayan and north-eastern states and Odisha, 40% in Punjab and 30% elsewhere.
Normative PLF is 80% for Rankine-cycle biomass, 85% for gasifiers, 90% for biogas, and 45%, 60% or 53% for bagasse co-generation depending on the state.
CERC no longer sets generic solar or wind tariffs (they are bid), so it publishes no normative solar or wind CUF.

**PIB Cabinet approvals of five Arunachal Pradesh hydro projects** (PRIDs 2250050, 2250043, 2155471, 2077106, 2077113).
Each release states the capacity and the design energy in MU per year.
We computed the design-energy CUF as design energy / (MW x 8760 h): Kamala 45.6%, Kalai-II 46.2%, Tato-II 44.7%, Heo 47.6%, Tato-I 49.2%.
No official table of design energy for the existing hydro fleet was found, so existing hydro is covered by the observed CUF rows below.

**PIB Cabinet release on the offshore wind VGF scheme (19.06.2024, PRID 2026699).**
It states about 3.72 billion units a year from the 1 GW of projects; we converted this to an implied CUF of 42.5%.

**IEA World Energy Outlook 2025, Table B.4b** (India rows).
The capacity factors that the IEA assumes in its India LCOE (2024, 2035, 2050) were copied from the notes of the matching `lcoe.csv` rows, which were taken from the same table.

**Observed CUF derived from Ember's India subnational monthly data** (`india_monthly_full_release_long_format.csv`; the local copy is in `data/raw/generation/downloads/ember/`).
Ember compiles CEA monthly generation reports and MNRE capacity reports, utilities only, by state and month, with capacity in MW and generation in GWh.
For each state, fuel and fiscal year with 12 complete months, we computed CUF = annual generation / (mean of the 12 monthly capacity values x hours in the year).
Region and all-India values pool generation and average capacity of the member states (state to region mapping from `data/raw/generation/state/state_region_map.csv`; Jammu and Kashmir and Ladakh are merged).
Each row gives the median over FY2019-20 to FY2024-25 as `value_pct`, the minimum and maximum as `low_pct` and `high_pct`, and the latest year in `notes`.
Fuels covered: coal (includes lignite), gas, nuclear, hydro (large and small together), solar, onshore wind and bioenergy.
As a check, Ember's FY2023-24 state totals match CEA General Review 2025 Table 4.7 (for example Gujarat solar 13,468.91 GWh, Kerala solar 1,195.29 vs 1,195.28 GWh, Rajasthan solar 38,365.21 vs 38,363.28 GWh), and the all-India coal value for FY2023-24 (68.9%) is close to CEA's 69.09%.
Important caveat: solar capacity in this data includes rooftop, hybrid and off-grid plants, but rooftop and off-grid generation is only partly reported.
The ratio therefore understates the CUF of utility-scale plants, most strongly where rooftop is a large share (Gujarat 14.5%, Haryana, Delhi, Kerala).
`quality_flag` marks rows with an average capacity under 250 MW (`small_capacity`) and rows whose median is below 13% for solar or wind, below 15% for hydro, or below 5% for thermal and bioenergy (`reporting_gap_suspected` or `idle_or_reporting_gap`).
Headline medians: solar Rajasthan 22.8% vs Kerala 16.0%; solar by region NR 20.0, SR 18.9, WR 14.5, ER 12.2; wind Gujarat 22.8, Karnataka 22.7, Tamil Nadu 18.1, Rajasthan 15.8.

---

## lcoe.csv

`lcoe.csv` holds 119 rows of discovered tariffs, regulated tariffs, capital costs and cost projections.
Columns: `technology, cost_metric, year, value_low, value_central, value_high, unit, geography, project_or_auction, basis, source_name, source_url, source_location, notes`.
Raw copies of every document are in `raw/lcoe/` (subfolders `seci`, `pib`, `nuclear`, `coal`, `gas`, `hydro`, `cerc`, `cea`, `iea`, `state_bess`, `mop`).
Every computed value (capacity-weighted means, crore per MW, paise to rupees, rupees to lakh) is stated in the `notes` column of its row.
Tariffs are nominal and in the year of discovery unless the row says otherwise; no currency or price-year conversion was applied.

**SECI e-reverse auction result sheets (seci.co.in).**
For each tender we downloaded the one-page result sheet linked from the SECI "Tenders Results" page (`https://seci.co.in/tenders/results`, then the tender-detail page).
Tables were parsed with pdfplumber and checked by eye.
For each auction the row gives the minimum and maximum awarded tariff as low and high, and a capacity-weighted mean of the awarded tariffs as central.
The auctions cover plain ISTS solar (ISTS-XIII, XIV, XVI, XVIII, all 2024), solar with storage (ISTS-XV 2-hour, XVII 4-hour, XX 4-hour, XXI 6-hour), wind-solar hybrid (Tranches VII, VIII, IX), wind (Tranches XVI, XVII, XIX, XX), and firm or peak products (FDRE-IV, FDRE-VI, RTC-IV, FDRE-VII, FDRE-RTC-V, FDRE-IX).
Storage auctions in the same format give the standalone BESS pilot ESS-I (2022), ESS-3 Kerala (2025), the Odisha 125 MW/500 MWh BESS (Feb 2026) and the first pumped-storage auction PSP-I (Jul 2026).
Wind Tranche-XVI was a closed state-specific bid whose sheet gives no bid-opening date, so its year is the financial year of the RfS.
A few sheets show bidder names in two side-by-side tables (before and after the e-reverse auction); only the "after e-reverse auction" table with awarded capacity was used.

**PIB, Ministry of Power, 12 Dec 2023 (PRID 1985538) and 15 Dec 2025 (PRID 2204118).**
Both releases report written replies in the Rajya Sabha.
The 2023 release converts the first SECI BESS tariff (Rs 10.83 lakh/MW/month) to about Rs 10.18/kWh.
The 2025 release says recent BESS bids correspond to about Rs 2.1/kWh at 2 cycles a day without VGF and Rs 2.8/kWh at the expected 1.5 cycles a day, and that recent solar tenders average about Rs 2.5/kWh.
These are storage costs only and exclude the cost of charging energy.

**Rajasthan Electricity Regulatory Commission orders on BESS tariff adoption (Petitions 2372/2025 and 2356/2025).**
These orders adopt the RVUNL 1000 MW/2000 MWh tariffs (Rs 1.775-1.785 lakh/MW/month) and the NVVN 500 MW/1000 MWh tariffs (Rs 2.16-2.19 lakh/MW/month), both 2-hour systems with VGF.
The RERC website could not be reached, so copies of the orders hosted by JMK Research were used and the bidder tables read directly.
The copies are third-party mirrors of official orders and should be checked against the RERC site if possible.

**Department of Atomic Energy, Parliament answers (dae.gov.in uploads on cdnbbsr.s3waas.gov.in).**
Lok Sabha Starred Question 226 (11.12.2024), part (c), gives the 2023-24 average nuclear tariff of Rs 3.83/kWh.
Lok Sabha Unstarred Question 2937 (11.03.2020) has an annexure with station tariffs for 2019-20 in paise/kWh, one merged cell per station, including Kudankulam 1&2 at 412.06 paise/kWh.
The same answer lists sanctioned costs of projects under construction and sanctioned, which we divided by capacity to give crore per MW.
No official document with the newer Kudankulam 3&4 tariff was found; it appears only in news reports, so it is not in the table.

**New coal tariffs from state tariff-based competitive bids (Adani Power media releases).**
The UPPCL (Rs 5.383/kWh, May 2025), MPPMCL (Rs 5.838/kWh, Sep 2025) and BSPGCL Bihar (Rs 6.075/kWh, Aug 2025) results come from the winning bidder's own releases, because no regulator adoption order was found online.
These are company disclosures of the bid result and are marked as such in the notes.

**CERC Staff Paper on Grid Security Charge (25.09.2023).**
Section 3.3 (page 9) states that gas-based generation on RLNG and liquid fuel has variable charges of Rs 12-18 per unit.
Annexure-I (page 14) assumes Rs 15/kWh for gas generation in high-demand months and Rs 24/kWh for open-cycle operation.
The paper does not state the LNG price behind these figures, so they stand in for an LNG-price-dependent marginal cost, not a levelised cost.

**PSERC Order in Petition 54 of 2021 (03.01.2022).**
This Punjab regulator order gives the levelised tariff of Vishnugad Pipalkoti HEP (Rs 4.52/kWh, first year Rs 4.71/kWh, on a CEA-vetted cost) and the tentative levelised tariff of Subansiri Lower (Rs 5.38/kWh, not yet vetted).

**PIB Cabinet approvals of hydro projects (PRIDs 2250050, 2250043, 2155471, 2077106, 2077113).**
These releases give the approved investment, capacity and design energy (MU) for the Kamala, Kalai-II, Tato-II, Heo and Tato-I projects in Arunachal Pradesh.
Investment divided by capacity gives crore per MW.

**CERC RE Tariff Order for FY 2026-27 (Petition 12/SM/2026, 23.08.2026).**
Annexure I (page 31) gives the levellised generic tariffs for small hydro by region and size, and biomass Rankine-cycle tariffs by state.
Normative capital costs are on page 17.
For biomass the row gives the applicable first-year tariff (levellised fixed cost plus the first-year fuel cost); the fuel component is reset each year.

**PIB Cabinet approval of the offshore wind VGF scheme (19.06.2024, PRID 2026700).**
The release states an investment of about Rs 4,50,000 crore for the initial 37 GW of offshore wind, which divides to about Rs 12.16 crore/MW.
No discovered offshore wind tariff exists yet.

**CEA National Electricity Plan 2022-32, Volume I Generation (May 2023).**
Annexure 5.1 (page 5.30, PDF page 156) gives the real 2021-22 capital costs used in the expansion modelling.
Annexure 8.1 (page 8.4, PDF page 216) gives the nominal year-by-year capital cost per MW used for the fund requirement, of which 2026-27 and 2031-32 were kept.
Chapter 13 (PDF page 313) gives the developer range for pumped storage capital cost, and Chapter 6 (PDF pages 169 and 171) gives the December 2020 solar and wind tariff reference points.

**IEA World Energy Outlook 2025, Annex B, Table B.4b (page 455).**
The India rows of the Stated Policies Scenario table were taken for nuclear, coal, gas CCGT, solar PV, onshore wind and offshore wind, for 2024, 2035 and 2050.
LCOE is in year-2024 USD/MWh, and the notes carry the capital cost and typical capacity factor.
The values were not converted to rupees.

---

## emission_factors.csv

Columns: `fuel, metric, period, value, unit, geography, basis, source_name, source_url, source_location, notes`.

**CEA CO2 Baseline Database for the Indian Power Sector, User Guide Version 21.0 (November 2025, data for FY2024-25).**
The current CEA site serves the guides from `https://cea.nic.in/api/cdm-report-version/view-pdf/en/<id>`; id 78 is version 21.0 and id 76 is version 20.0 (checked from each cover page).
The database workbook itself did not download from that endpoint, so all values come from the user guide.
Table S and Table 4 (PDF pages 7 and 19) give the FY2024-25 grid factors: weighted average 0.710, operating margin 0.961, build margin 0.512 and combined margin 0.736 tCO2/MWh, adjusted for cross-border transfers and including RES and captive injection.
Table 5 (PDF page 19) gives the generation-weighted specific emissions of fossil stations on net generation: coal 0.969, gas 0.452, lignite 1.32 tCO2/MWh.
Version 20.0 Table 5 gives the FY2023-24 values (coal 0.969, gas 0.451, lignite 1.30) for comparison.
Annexure-I (PDF page 27) gives the weighted average including RES for every year from 2013-14 (0.774) to 2024-25 (0.710), with total emissions and net generation.
Appendix B (PDF page 32) gives the fuel emission factors in gCO2/MJ on a GCV basis including oxidation (Indian coal 90.6, imported coal 85.2, lignite 100.5, gas 49.4, oil 71.9, diesel 69.1, naphtha 66.0).
Section 4.5 explains that the weighted average divides fossil CO2 by all net generation including hydro, nuclear and RES, so these sources carry zero direct CO2; rows with value 0 record that convention.

**CEA NEP 2022-32 Vol I, Section 10** (printed page 10.10, PDF page 266).
The projected grid average falls to 0.548 kgCO2/kWh (net) in 2026-27 and 0.430 in 2031-32, with total power-sector emissions of 1,057 and 1,100 Mt.

**Ember subnational data methodology.**
Ember applies IPCC AR5 lifecycle factors (gCO2e/kWh: coal 820, gas 490, other fossil 700, bioenergy 230, solar 48, hydro 24, nuclear 12, wind 11, other renewables 38).
They are kept for lifecycle sensitivity only and are not comparable with the CEA direct factors.

---

## existing_fleet_generation.csv

Columns: `item, category, plant_or_group, state, region, sector, capacity_mw, period, implied_generation_twh_at_nep_plf, implied_generation_twh_at_fy25_plf, source_name, source_url, source_location, notes`.
Generation by source and region for recent years is already in the repository (`data/raw/generation/region/cea_region_generation_fy.csv` from CEA General Review Table 4.4 up to FY2023-24, and `ember_region_generation_fy.csv` up to FY2024-25), so it is not repeated here; one reference row points to it.
This table instead holds the coal-fleet facts needed for a 2047 coal floor.
Each capacity row is converted to energy at two stated PLFs, as MW x PLF x 8760 h: the NEP projection for 2031-32 (58.7%) and the CEA actual for FY2024-25 (69.45%).

**CEA NEP 2022-32 Vol I.**
Annexure 5.2 (PDF page 157) lists the 12 coal plants (2,121.5 MW) that CEA considered for likely retirement in 2022-32; two of them (Obra unit 7, Durgapur unit 4) had already retired in late 2022.
These are the only retirements in the NEP, so the plan keeps the rest of the coal fleet running to 2031-32.
Table 5.3 (PDF page 135) gives the coal and lignite base by region on 31.03.2022 (210,700 MW).
Table 5.4 (PDF page 136) gives 26,900 MW of coal and lignite under construction for 2022-32, and the conclusions give 19.1-27.1 GW of further coal needed by 2031-32 across scenarios.

**Ministry of Power Annual Report 2025-26** (PDF pages 20 and 26).
40,345 MW of coal capacity was under construction on 31.12.2025 (central 17,560, state 11,540, private 11,245), with a year-wise commissioning schedule from 2025-26 (8,485 MW) to 2031-32 (600 MW) that peaks in 2029-30 (13,700 MW).
Contracts for a further 20,480 MW were awarded in 2025, 24,020 MW of candidate capacity is at planning stages and 5,320 MW was under advanced bidding.
The report puts coal capacity at 226 GW on 31.12.2025 and the requirement at 307 GW by 2034-35.

**Ministry of Power, Rajya Sabha Unstarred Question 953 (27.07.2026)**, in the 38-page set of answers at the cited URL (PDF page 19).
It states 47,545 MW of thermal capacity under construction (including 4,845 MW of stressed projects), 21,080 MW commissioned since April 2023, a coal and lignite requirement of about 3,15,000 MW by 2035-36 against 2,11,855 MW on 31.03.2023, and an additional minimum of 1,05,000 MW envisaged.

**Coal fleet age and life extension.**
No official age-wise table of the coal fleet was obtained.
The CEA quarterly R&M review reports, which carry the phasing plan, returned "page not found" on the CEA site and were not in the Internet Archive.
A news report (NBM&CW, 7 Aug 2026) of a Ministry of Power reply to Parliament gives the plan: 223 coal units of 63,440 MW, all over 20 years old in December 2022, are scheduled for renovation and life extension in nine phases up to June 2046 (central 26,040 MW, state 28,870 MW, private 8,530 MW).
These rows are flagged as secondary in their notes.
Together with the NEP retirement list, they indicate that the older fleet is to be life-extended rather than retired before the mid-2040s.

---

## transmission.csv

Columns: `corridor, region_a, region_b, metric, value, unit, as_of, status, source_name, source_url, source_location, notes`.
Corridor codes: ER-NR, ER-WR, WR-NR, ER-SR, WR-SR, ER-NER, NER-NR.
Values are the aggregate rated capacity of inter-regional links (220 kV and above, AC and HVDC), which CEA calls inter-regional transmission capacity; they are not simultaneous transfer limits.

**CEA Growth Summary of Transmission Lines, September 2025** (page 4).
The table "Growth of Inter-Regional Transmission Capacity" gives the corridors on 30.09.2025: ER-NR 22,530, ER-WR 22,790, WR-NR 38,320, ER-SR 7,830, WR-SR 22,320, ER-NER 3,550 and NER-NR 3,000 MW, a total of 1,20,340 MW.
It also gives the totals for March 2022 to March 2025.
The page was rendered and read to confirm the cells.
The CEA URL now fails; the file came from the Internet Archive (snapshot 20260826114712).
An All India Radio news item of 22.01.2026 quoting the Ministry of Power confirms the total was still 1,20,340 MW.

**CEA National Electricity Plan Vol II Transmission** (executive summary, PDF pages 38 and 40).
The plan gives each corridor's capacity on 31.03.2022, the planned additions for 2022-27 and 2027-32, and the totals at the end of 2026-27 (1,42,940 MW) and 2031-32 (1,67,540 MW).
It also states 1,18,740 MW on 31.03.2024.
The CEA URL now fails; the file came from the Internet Archive (snapshot 20250804071953).

**CEA General Review 2025, Tables 6.1 and 6.2** (PDF pages 103-104).
Regional transmission losses in 2023-24 were NR 3.15%, WR 3.12%, SR 3.46%, ER 4.24% and NER 3.89% of the energy available in each region.
All-India losses in transformation, transmission, distribution and unaccounted energy were 17.63% of net energy available, and power-station auxiliary consumption was 6.15% of gross generation.
These two figures convert gross generation to energy delivered.

---

## nuclear_pipeline.csv, offshore_wind.csv, policy_targets.csv

Raw copies are under `raw/dae_parliament/`, `raw/pib/`, `raw/mnre_offshore/`, `raw/unfccc_ndc/`, `raw/nghm/`, `raw/mop/`, `raw/niti_viksit_netzero/` and `raw/third_party_mirrors/`.
Text was extracted with PyMuPDF (PDF) or a tag-stripping parser (HTML) and read by eye; no value was typed from memory.
Page numbers are PDF page numbers unless marked "printed".

### Nuclear pipeline

Columns: `project, units, state, region, reactor_type, capacity_mw, status, expected_commissioning, source_name, source_url, source_location, notes`.
`status` takes the values `under_construction`, `sanctioned`, `planned`, `recently_commissioned` and `target`.

**PIB release 2220292, 29 Jan 2026 (DAE Rajya Sabha reply "Expansion Projects").**
This is the primary project list.
Its table gives location, capacity, physical progress, approved cost and expected completion for RAPP-7&8, KKNPP-3&4, KKNPP-5&6 and GHAVP-1&2, and lists the ten 700 MW PHWRs under pre-project activities (Kaiga-5&6, GHAVP-3&4, Chutka-1&2, Mahi Banswara-1 to 4) with "progressively by 2031-32".
A footnote gives RAPP-7 commercial operation on 15.04.2025; the text covers PFBR commissioning and FBR 1&2 pre-project approval.
The page truncates two state names, which were filled from the DAE answer below.

**DAE Lok Sabha Starred Question 251, 17.12.2025 (dae.gov.in PDF).**
Annexure-II gives the state for every project under construction and at pre-project stage (used for Chutka = Madhya Pradesh).
The statement gives the 100 GW split (DAE PSUs about 58-60 GW; NPCIL roadmap about 54 GW) and the ASHVINI joint venture for Mahi Banswara.

**DAE answers of 2024-2026 (dae.gov.in PDFs): LS 2155 (12.03.2025), LS 5176 (02.04.2025), LS 558 (23.07.2025), LS 545 Hindi (23.07.2025), LS 497 (03.12.2025), RS 52 (04.12.2025), LS 1638 (10.12.2025), LS 795 (04.02.2026), LS 5368 (25.03.2026), LS 226 (11.12.2024), LS 331 (27.11.2024).**
These were used for the interim targets and cross-checks: 22,380 MW by 2031-32 excluding RAPS-1 (LS 497, LS 1638); 22,480 MW in RS 52 and LS 5176 (the 100 MW gap equals RAPS-1); 6,600 MW in 8 reactors under construction and 7,000 MW in 10 reactors at pre-project stage (RS 52); about 90 GW to be added by 2047 and about 54 GW by NPCIL (LS 5368); and earlier completion years (LS 2155 Annexure-B).

**PIB backgrounder on PFBR first criticality, 7 Apr 2026 (static.pib.gov.in PDF).**
Page 1 gives first criticality on 06.04.2026; page 3 repeats 8.78 GW installed, 56,681 MU generated in 2024-25 and 22.38 GW by 2031-32.

**Union Budget 2025-26 speech (indiabudget.gov.in; local copy in `data/external/raw/budget2025_26/`).**
Paragraphs 61-62 (printed page 11) set at least 100 GW of nuclear by 2047 and a Rs 20,000 crore Nuclear Energy Mission for small modular reactors, with at least 5 operational by 2033.
The online file size (724,405 bytes) matches the local copy.

**Ministry of Power Annual Report 2025-26.**
Page 21 gives 6,600 MW of nuclear under construction for 2025-26 to 2031-32, plus 8,000 MW under administrative approval that may yield benefit by 2035-36.

**Third-party reproductions (aviation-defence-universe.com) of DAE replies dated 05 Aug 2026 (Lok Sabha) and 13 Aug 2026 (Rajya Sabha).**
The official PIB pages for these replies could not be located.
They are used only for (a) unit-wise revised commissioning dates (KKNPP-3 Jun-2027, KKNPP-4 Jun-2028, KKNPP-5 Jun-2029, KKNPP-6 Mar-2030, Kaiga-5 Jun-2031, Kaiga-6 Jun-2032, PFBR Mar-2027), which sit in the notes, and (b) the planned and in-principle sites (RAPP-9&10, KAPP-5&6, NAPP-3&4, Bhimpur 1-4, Jaitapur 6 x 1730, Kovvada 6 x 1208, Haripur 6 x 1000, Mithi Virdi 6 x 1000).
Each such row says so.
One inconsistency: the 13 Aug reproduction dates RAPP-7 commercial operation to 15.04.2026, against 15.04.2025 in the official PIB release.

### Offshore wind

Columns: `item, state, zone, value, unit, year, basis, source_name, source_url, source_location, notes`.

**MNRE "Strategy for Establishment of Offshore Wind Energy Projects", Rev. 02, 26 Sept 2023.**
Page 1 lists eight zones each off Gujarat and Tamil Nadu.
Table 1 on page 4 gives the 37 GW auction trajectory by year and model (A 1 GW, B 14 GW, C 22 GW).
Annexure-II on page 12 lists 13 Tamil Nadu Model-B sites with 3,409 sq km and 15,344 / 17,045 / 20,454 MW at 4.5 / 5.0 / 6.0 MW per sq km.
Pages 1 and 4 cover the 365 sq km Zone B3 site, and page 5 the lease terms.
The document calls this site both "equivalent to 0.5 GW" (page 1) and "sufficient for a 1.0 GW project" (page 4).

**Potential estimates.**
The 70 GW total comes from MNRE's Akshay Urja magazine (Sept 2024, page 9: "about 70 GW ... 16 zones, 8 each").
The 36 GW Gujarat and 35 GW Tamil Nadu split comes from the MNRE-hosted CIF REI Investment Plan (Oct 2024, page 19).
The latest Tamil Nadu figure is 31 GW, from MNRE Lok Sabha Unstarred Question 1630 (29.07.2026).
These are initial assessments, not bankable resource estimates.

**PIB releases.**
PRID 1535909 (19 Jun 2018) gives the 30 GW by 2030 target and the missed 5 GW by 2022 target.
PRID 2026699 (19 Jun 2024, Cabinet VGF) gives 1 GW (500 MW each state), Rs 6,853 crore plus Rs 600 crore for ports, about 3.72 billion units a year from 1 GW, 2.98 MtCO2e a year avoided, and Rs 4.5 lakh crore for an initial 37 GW.
PRID 1941109 (20 Jul 2023) gives CTU planning for 10 GW (5 GW per state) and the ISTS waiver to 31.12.2032.
PRID 2089056 (Year End Review 2024) records SECI's 4 GW seabed-lease tender off Tamil Nadu.
The 42.5% capacity factor in the notes is our own conversion of 3.72 TWh from 1 GW and is labelled as such.

**MNRE Lok Sabha Unstarred Question 519, 22.07.2026.**
This gives the official status line: "The first offshore wind energy project is yet to be awarded in the country."
Press reports that the 2024-25 Gujarat and Tamil Nadu tenders were cancelled for lack of bids were not checked against an official document and are not used.

### Policy targets

Columns: `target, value, unit, target_year, basis, status, source_name, source_url, source_location, notes`.
`basis` is `capacity`, `generation`, `emissions`, `emissions_intensity` or `other`; `status` is `adopted_ndc`, `official_target`, `announced` or `projection`.
Of the 93 rows, 31 are capacity-based, 41 generation-based, 9 emissions or emissions-intensity and 12 other (demand, energy requirement, hydrogen tonnes), counting the CEA NEP rows added below.

**PIB 1768712 (COP26 national statement, 01 Nov 2021).**
It gives the five Panchamrit elements: 500 GW non-fossil capacity, 50% of energy requirements from RE, 1 billion tonnes less projected emissions to 2030, intensity below 45%, and net zero by 2070.

**PIB 1847812 (Cabinet, updated NDC, 03 Aug 2022).**
It sets a 45% cut in emissions intensity from 2005 by 2030, and about 50% of installed capacity from non-fossil sources (a capacity-based target).

**India's NDC 2031-2035 (UNFCCC, April 2026) and PIB 2245209 (Cabinet approval, 25 Mar 2026).**
Page 10 (printed 9) has the 47% intensity cut by 2035, about 60% non-fossil installed capacity by 2035 (conditional on technology transfer and low-cost finance), and a 3.5-4.0 GtCO2e sink.
Page 5 has 52.57% non-fossil capacity on 28.02.2026 and a 36% intensity reduction over 2005-2020.

**PIB 2144627 (14 Jul 2025).**
The annexure table at 30.06.2025 shows thermal 242.04 GW (49.92%), nuclear 8.78 GW (1.81%) and RE including large hydro 234.00 GW (48.27%) out of 484.82 GW.
The 50.08% non-fossil share is our sum of the two non-fossil lines.

**PIB 1888547 (Cabinet, National Green Hydrogen Mission, 04 Jan 2023).**
It sets at least 5 MMT a year by 2030, about 125 GW of associated RE capacity, and about 50 MMT a year of GHG abatement.
No official electricity requirement in TWh was found: the Cabinet release, the PIB backgrounders of May 2024 and Nov 2025, and the mission document link were checked, and the nghm.mnre.gov.in and mnre.gov.in mission document URLs did not download.
That row is left blank.
The closest official figure is in the CEA NEP Vol II (see below): an initial MNRE estimate of 70.5 GW of additional electricity demand for green hydrogen and ammonia by 2031-32, stated in GW, not energy.

**2047 statements.**
PIB 2064702 (14 Oct 2024) reports the Power Minister expecting 708 GW of demand and needing 2,100 GW of capacity by 2047.
The Independence Day speeches of 2021 (PIB 1746062), 2025 (PIB 2156749) and 2026 (pmindia.gov.in) set energy independence before 2047, a more than tenfold nuclear increase, 100 GW nuclear by 2047, and 5 new reactors this decade.
PIB 2226010 (10 Feb 2026) quotes the Power Secretary's aspiration of 40% electricity in the energy basket by 2047.

**Ministry of Power Annual Report 2025-26 (May 2026), pages 19-20.**
The 20th EPS gives 446 GW peak and 3,215 BU energy requirement in 2034-35.
CEA's 2034-35 capacity study gives 1,029 GW in total: coal 307, gas 20, nuclear 22 and RE 680 (large hydro 73, PV 447, wind 138, other 22), plus 62 GW of pumped storage and about 99 GW / 396 GWh of BESS.
The non-fossil share is about 68.2% of capacity and about 49.7% of generation in 2034-35, against 25.5% of generation in 2024-25.
Page 10 mentions a 300 GWh storage target by 2030 as a Chintan Shivir discussion item only.

**NITI Aayog, Scenarios Towards Viksit Bharat and Net Zero, Power Vol. 7 (Feb 2026).**
These are generation-based scenario results with no 2047 point; 2050 is the nearest year.
Section 4.4.2 (page 78) gives total generation of 7,350-7,700 TWh (current policy) and 9,700-10,200 TWh (net zero) in 2050.
Table 4.2 (page 79) gives 2050 generation shares by source, Section 4.1 (page 70) gives 2050 consumption, and page 76 gives nuclear capacity in 2070.
Ranges are kept as "low-high" strings because they span two models.
The report's disclaimer says it is not a statement of policy.

**PIB Global Wind Day 2026 backgrounder (14 Jun 2026), page 2.**
It gives 100 GW of wind by 2030 and 156 GW by 2036.

**CEA NEP 2022-32 Vol I (rows added to policy_targets.csv).**
Tables 5.11 and 5.12 (PDF pages 147-148) give the projected gross generation by source from CEA's hourly dispatch of the optimal mix.
For 2026-27 the total is 2,025.0 BU: coal and lignite 1,203.4, gas 34.1, nuclear 77.9, large hydro including imports 207.7, solar PV 339.3, wind 153.5 and other RE 9.1.
For 2031-32 the total is 2,665.7 BU: coal and lignite 1,334.8 (printed as 1,334.7 in Table 5.14), gas 33.4, nuclear 117.6, large hydro including imports 246.2, solar PV 665.6, wind 258.1 and other RE 10.0.
These rows are generation-based.
The same chapter gives the energy requirement (1,907.8 and 2,473.8 BU), rooftop solar treated on the demand side (17.6 and 55.8 BU), the RE share of generation including large hydro (about 35% and 44.3%), the installed capacity of the mix (609.6 and 900.4 GW, capacity-based, for reference) and the finding that 500 GW non-fossil capacity is reached by 2029-30.
Section 10 (PDF page 266) gives projected power-sector CO2 of 1,057 Mt (2026-27) and 1,100 Mt (2031-32).

**CEA NEP Vol II Transmission (row added to policy_targets.csv).**
The executive summary (PDF page 41) states MNRE's initial estimate of 70.5 GW of additional electricity demand for green hydrogen and green ammonia by 2031-32, in coastal Gujarat, Odisha, West Bengal, Andhra Pradesh, Tamil Nadu and Karnataka.

---

## Raw files

| Folder under `raw/` | Content |
|---|---|
| `lcoe/cea/` | CEA NEP 2022-32 Vol I Generation (Internet Archive copy) |
| `lcoe/seci/` | SECI auction result sheets and tender pages |
| `lcoe/pib/`, `lcoe/nuclear/`, `lcoe/coal/`, `lcoe/gas/`, `lcoe/hydro/`, `lcoe/cerc/`, `lcoe/state_bess/`, `lcoe/iea/`, `lcoe/mop/` | Tariff and cost sources for `lcoe.csv`; `lcoe/cerc/` also holds the CERC order used for normative CUFs; `lcoe/mop/` holds the Ministry of Power Rajya Sabha answers of 27.07.2026 (RS 953) |
| `cea_co2/` | CEA CO2 Baseline Database user guides v20 and v21 |
| `cea_transmission/` | CEA NEP Vol II Transmission and the CEA transmission growth summary of September 2025 (both Internet Archive copies) |
| `dae_annual_report/` | DAE Annual Report 2024-25 |
| `dae_parliament/` | DAE Parliament answers 2024-2026 |
| `mop/` | Ministry of Power Annual Report 2025-26 |
| `pib/`, `mnre_offshore/`, `unfccc_ndc/`, `nghm/`, `niti_viksit_netzero/`, `third_party_mirrors/` | Sources for the nuclear, offshore and policy tables |

The CEA General Review 2025, the CEA Growth book and the Ember files used for `capacity_factors.csv` are already in `data/raw/generation/downloads/` and were not copied again.

## Not found or not used

* Official state-wise solar and wind CUF tables from CERC, SERCs, MNRE or NIWE were not found in a usable form; the observed state CUFs are therefore derived from CEA and MNRE data (via Ember), and the NEP regional planning CUFs are the official alternative.
* Design energy of the existing hydro fleet.
* NPCIL station-wise PLF history before April 2024 (only the fleet values for FY2023-24 and FY2024-25 and the April-October 2024 unit values).
* An official coal fleet age profile; only the life-extension plan, from a secondary report.
* An official Kudankulam 3&4 tariff, a regulator-adopted new-coal tariff or CEA/CERC coal benchmark, a gas tariff tied to a stated LNG price, lignite tariffs, an offshore wind discovered tariff, and TERI or CEEW cost projections.
* The NTPC, NHPC and SJVN own-auction result sheets (only SECI sheets were used, plus the NVVN and RVUNL storage orders).
* The CEA CO2 Baseline Database workbook for v21 (the user guide was used).
* A TWh figure for the electricity need of the Green Hydrogen Mission.
* The CEA "Roadmap for achieving 100 GW nuclear by 2047" (October 2025), which returned "page not found" and was blocked in the web archive, so there is no year-wise or state-wise path to 100 GW.
* A year-by-year inter-regional capacity plan beyond the NEP Vol II corridor totals for 2026-27 and 2031-32.
