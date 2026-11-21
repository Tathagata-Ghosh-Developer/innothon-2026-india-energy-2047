# Price indices and exchange rates: sources and extraction notes

This folder holds the official price indices, exchange rates and the inflation target needed to put every money figure in the model on one real price base, **real 2024 USD** (the price year of IEA WEO 2025).
It supports design section A of `docs/plans/2026-11-21-inflation-features-design.md`.
All sources were fetched and read on **2026-11-21**.
Every source in this folder is **secondary**: the project's primary sources are only NPP and the Renewable India dataset (`docs/data_audit.md`).
Every source is official: a government statistical office (NSO/MoSPI, the Office of the Economic Adviser, US BLS, US BEA) or a central bank (RBI; the US Federal Reserve's FOMC for the US inflation goal).
Every value traces to a file that was fetched and read; nothing was filled from memory.
Where an official figure does not exist yet, the row is either left out or built from official figures with a stated formula and marked **DERIVED** in `notes`.

## Conventions

* `period` is either an Indian fiscal year (April to March), written `2023-24`, or a calendar year, written `2024`. The `unit` column says which (for example "fiscal-year (April-March) average" or "calendar-year annual average").
* Each series has one unit and no gaps between its first and last period.
* `base` gives the index base (for example `2012=100`) or the price basis of a money series (for example `2011-12 prices`).
* A value computed by us (an average, a ratio, a linked value) says so in `notes`, with the formula.
* INR crore = 10 million rupees. GDP values are in INR crore as published.
* Raw copies of every file are in `raw/` (table at the end). They are git-ignored by the repository's `data/**/raw/**` rules; JSON, text and CSV downloads were gzip-compressed so that the same rules cover them.
* The extraction and check scripts were kept outside the repository, because the project code is C++.

## File

`price_indices.csv`, long format, 192 rows.
Columns: `series, unit, period, value, base, source_name, source_url, source_location, accessed, notes`.

| series | periods | content |
|---|---|---|
| `india_cpi_combined_general` | FY2011-12 to FY2025-26 | CPI All India Combined General, 2012=100, fiscal-year average; FY2025-26 is DERIVED |
| `india_cpi_combined_inflation` | FY2012-13 to FY2025-26 | Change in the fiscal-year average CPI, percent; FY2025-26 is DERIVED |
| `india_cpi_combined_general_base2024` | FY2024-25 to FY2025-26 | CPI All India Combined General, 2024=100 (FY2024-25 from MoSPI's linked back series) |
| `india_cpi_linking_factor_2012_to_2024` | 2025 | MoSPI linking factor between the 2012 and 2024 bases, Combined |
| `india_wpi_all_commodities` | FY2011-12 to FY2025-26 | WPI All Commodities, 2011-12=100, fiscal-year average |
| `india_wpi_all_commodities_inflation` | FY2012-13 to FY2025-26 | Change in the fiscal-year average WPI, percent |
| `india_wpi_all_commodities_base2022_23` | FY2022-23 to FY2025-26 | WPI All Commodities, 2022-23=100 |
| `india_wpi_linking_factor_2011_12_to_2022_23` | FY2024-25 | OEA linking factor between the 2011-12 and 2022-23 bases |
| `india_gdp_current_prices`, `india_gdp_constant_2011_12_prices`, `india_gdp_deflator` | FY2011-12 to FY2025-26 | GDP on the 2011-12 series (nominal, real at 2011-12 prices) and the implicit deflator, 2011-12=100 |
| `india_gdp_current_prices_base2022_23`, `india_gdp_constant_2022_23_prices`, `india_gdp_deflator_base2022_23` | FY2022-23 to FY2025-26 | The same on the new 2022-23 series |
| `rbi_inr_per_usd_fy_average` | FY2021-22 to FY2025-26 | Rupees per US dollar, fiscal-year average of the reference rate |
| `rbi_inr_per_usd_calendar_average` | 2020 to 2025 | Rupees per US dollar, calendar-year average of the reference rate |
| `rbi_inflation_target_cpi`, `..._lower_tolerance`, `..._upper_tolerance` | FY2016-17 to FY2030-31 | The CPI inflation target (4) and band (2, 6) under section 45ZA of the RBI Act, one row per fiscal year covered by a notification |
| `us_cpi_u_all_items` | 2020 to 2025 | US CPI-U, U.S. city average, all items, 1982-84=100, annual average |
| `us_gdp_implicit_price_deflator` | 2020 to 2025 | US GDP implicit price deflator, 2017=100 |
| `us_fomc_inflation_goal_pce` | 2026 | The FOMC longer-run inflation goal (2 percent, PCE price index), as reaffirmed effective 27 Jan 2026 |

---

## 1. India CPI (NSO, MoSPI)

**RBI, Handbook of Statistics on the Indian Economy 2025-26 (31 Jul 2026), Table 34 "Consumer Price Index - Annual Average".**
URL `https://www.rbi.org.in/Scripts/PublicationsView.aspx?id=23859`; raw copy `raw/rbi_hbie2026/HBIE2026_PublicationsView_23859.html`.
Column 7, "CPI All India General Index: Combined", source NSO, MoSPI.
The table prints the official fiscal-year averages of the 2012=100 series for FY2011-12 (93.3) to FY2024-25 (192.6), and 103.5 for FY2025-26 on the new base 2024=100.
Its notes say the 2012=100 indices end in December 2025 and the 2024=100 indices start in January 2025.

**RBI, same Handbook, Table 236 "Consumer Price Index (Average of Months) - Annual Variation".**
URL `https://www.rbi.org.in/Scripts/PublicationsView.aspx?id=24061`; raw copy `raw/rbi_hbie2026/HBIE2026_PublicationsView_24061.html`.
Column 7 (Combined) gives the official rates for FY2012-13 to FY2024-25.
For FY2025-26 it gives only part-year rates: 1.7 percent for April to December 2025 (base 2012) and 3.1 percent for January to March 2026 (base 2024).

**NSO, MoSPI, eSankhyiki CPI table (monthly indices).**
Page `https://esankhyiki.mospi.gov.in/macroindicators?product=cpi`, read through the portal's own data service (`https://api.mospi.gov.in/api/cpi/getCPIData`): base 2012, All India, Combined, General, January 2011 to December 2025 (2011 and 2012 are in the portal's "Back" series); base 2024, All India, Combined, CPI (General), January 2025 to October 2026, and the base-2024 back series for 2023 and 2024.
Raw copy `raw/mospi_cpi/esankhyiki_cpi_allindia_combined_general_20261121.json.gz`.
Use: a check of the RBI averages and the base-2024 average for FY2024-25.
The mean of the 12 monthly indices for each fiscal year, our computation, rounds to the RBI figure in every year from FY2011-12 to FY2024-25 (for example FY2024-25: 192.617 against 192.6).
FY2024-25 on base 2024 = mean of the back-series and current-series monthly indices April 2024 to March 2025 = 101.465, our computation, entered as 101.5.
FY2025-26 on base 2024 = 103.544 by the same method; RBI prints 103.5.

**NSO, MoSPI, "First Press Release of Consumer Price Index on Base 2024=100" (12 Feb 2026), as issued on PIB.**
URL `https://static.pib.gov.in/WriteReadData/specificdocs/documents/2026/feb/doc2026212787501.pdf`; raw copy `raw/mospi_cpi/pib_cpi_jan2026_doc2026212787501.pdf`.
Page 10, "Linking Factor and Back Series": the linking factor is the ratio of the 2025 geometric-mean indices, new over old; Combined **0.5267** (Rural 0.5222, Urban 0.5320).
MoSPI says the two series can be linked only at the general-index level.
Page 5 gives the final December 2025 index on base 2012 (Combined 198.0), which matches the eSankhyiki data.
Our ratio of the 2025 geometric means of the eSankhyiki monthly indices is 0.52673, which confirms the published factor.
The Expert Group report on the new CPI (January 2026, `raw/mospi_cpi/mospi_marquee_3ee743c5.pdf`, from `https://www.mospi.gov.in/uploads/Marquee/doc-3ee743c5-3e59-4e72-9955-9230961d8090.pdf`) gives the same linking formula and was read for context only.

**DERIVED rows.**

* `india_cpi_combined_general`, FY2025-26 = 103.5 / 0.5267 = **196.5** (2012=100). No official base-2012 average exists for FY2025-26, because that series stopped in December 2025.
* `india_cpi_combined_inflation`, FY2025-26 = 196.5 / 192.6 - 1 = **2.0 percent**. On base 2024 the two averages (101.465 and 103.544) give 2.05 percent.

## 2. India WPI (Office of the Economic Adviser, DPIIT)

**OEA, WPI series base 2011-12, financial-year index file.**
URL `https://eaindustry.nic.in/indx_download_1112/fin_year_index.xls` (linked from `https://eaindustry.nic.in/download_data_1112.asp`); raw copy `raw/oea_wpi/indx_download_1112_fin_year_index.xls` (last saved by OEA on 23 Jun 2026).
Sheet `Financial_Year_Index`, row "All commodities" (code 1000000000), columns `INDEX20122013` to `INDEX20252026`.
FY2012-13 106.9 to FY2025-26 155.99 (the file holds the unrounded mean 155.99167; RBI HBIE 2025-26 Table 33 prints 156.0, which is the value entered).
FY2011-12 is the base year, so its average is 100 by definition; the OEA files start in April 2012. This row is marked as a base-year value.
The OEA monthly file (`indx_download_1112_monthly_index_202606.xls`) and calendar-year file were also archived; the monthly file runs to April 2026, so the 2011-12 series is still published alongside the new one.

**RBI, Handbook 2025-26, Table 33 "Wholesale Price Index - Annual Average" and Table 235 "Wholesale Price Index - Annual Variation"**, source OEA.
URLs `https://www.rbi.org.in/Scripts/PublicationsView.aspx?id=23858` and `https://www.rbi.org.in/Scripts/PublicationsView.aspx?id=24060`; raw copies in `raw/rbi_hbie2026/`.
Table 33 matches the OEA file in every year.
Table 235, column 2 (All commodities), gives the inflation rates on the 2011-12 base for FY2012-13 to FY2025-26 (13.0 percent in FY2021-22, 0.7 percent in FY2025-26).

**OEA, WPI series base 2022-23.**
Yearly index file `https://eaindustry.nic.in/indx_download_2223/wpi_yearly_index_202606.xlsx` (linked from `https://eaindustry.nic.in/download_data_2223.asp`); raw copy `raw/oea_wpi/indx_download_2223_wpi_yearly_index_202606.xlsx`.
Row "All Commodities": FY2023-24 99.3, FY2024-25 101.0, FY2025-26 101.4. FY2022-23 is the base year (100 by definition).
Linking factor page `https://eaindustry.nic.in/linking_factor2223.asp` (raw copy `raw/oea_wpi/linking_factor2223.html`): All Commodities **1.53**, the ratio of the geometric means of the twelve monthly indices of the old and new series in FY2024-25. OEA advises caution and prescribes no linking method.

## 3. India GDP and GDP deflator (NSO, MoSPI National Accounts)

**2011-12 series: NSO, MoSPI, eSankhyiki National Accounts Statistics.**
Page `https://esankhyiki.mospi.gov.in/macroindicators?product=nas`, read through the portal's own data service (`https://api.mospi.gov.in/api/nas/getNASData`, base_year 2011-12, series Current, frequency Annual, indicator Gross Domestic Product).
Raw copy `raw/mospi_nas/esankhyiki_nas_gdp_annual_20261121.json.gz` (every vintage of every year, 66 records).
For each year the latest estimate was used:

| FY | estimate used |
|---|---|
| 2011-12 to 2013-14 | Additional Revision |
| 2014-15 to 2020-21 | Third Revised Estimates |
| 2021-22 | Second Revised Estimates |
| 2022-23 | Final Estimates |
| 2023-24 | First Revised Estimates |
| 2024-25 | Provisional Estimates |
| 2025-26 | First Advance Estimates (7 Jan 2026) |

The 2011-12 series was replaced by the 2022-23 series on 27 Feb 2026, so the FY2023-24, FY2024-25 and FY2025-26 values above are the last estimates on the 2011-12 base and will not be revised.
Cross-checks (all agree within 1 crore; eSankhyiki values are truncated, printed values rounded):

* MoSPI press note "First Advance Estimates of Gross Domestic Product for 2025-26" (7 Jan 2026), Statements 1 and 2, at constant (2011-12) and current prices: FY2023-24 30,122,956 and 17,650,591; FY2024-25 33,068,145 and 18,796,955; FY2025-26 35,713,886 and 20,189,919. URL `https://static.pib.gov.in/WriteReadData/specificdocs/documents/2026/jan/doc202617752701.pdf`; raw copy `raw/mospi_nas/doc202617752701.pdf`.
* MoSPI press note of 28 Feb 2025 (Second Advance Estimates 2024-25, Final Estimates 2022-23): FY2022-23 26,890,473 and 16,164,913. URL `https://static.pib.gov.in/WriteReadData/specificdocs/documents/2025/feb/doc2025228510301.pdf`; raw copy in `raw/mospi_nas/`.
* MoSPI press note of 30 May 2025 (Provisional Estimates 2024-25), current prices: FY2022-23 26,890,473, FY2023-24 30,122,956, FY2024-25 33,068,145. URL `https://www.mospi.gov.in/sites/default/files/press_release/NAD_PR_30may2025.pdf`; raw copy in `raw/mospi_nas/`.
* RBI Handbook 2024-25 (29 Aug 2025), Tables 1 and 2 on the 2011-12 base, FY2018-19 to FY2024-25 (`https://www.rbi.org.in/Scripts/PublicationsView.aspx?id=23175` and `id=23176`; raw copies in `raw/rbi_hbie2025/`), and RBI Handbook 2025-26 Tables 1 and 2 for FY2020-21 to FY2022-23 (`id=23826` and `id=23827`; raw copies in `raw/rbi_hbie2026/`).

**2022-23 series: NSO, MoSPI, press release "National Accounts Statistics - 2026 Publication" (31 Aug 2026), as issued on PIB.**
URL `https://static.pib.gov.in/WriteReadData/specificdocs/documents/2026/aug/doc_9748_20260831_15530601.pdf`; raw copy `raw/mospi_nas/doc_9748_20260831_15530601.pdf`.
Page 2, paragraph 10, column "Updated": FY2022-23 26,177,001 (both prices); FY2023-24 29,073,287 and 28,094,358; FY2024-25 31,798,524 and 30,115,428 (First Revised Estimates); FY2025-26 34,536,796 and 32,470,100 (Provisional Estimates).
These replace the values of the 5 Jun 2026 press note and of RBI Handbook 2025-26 Tables 1 and 2 (for example FY2024-25 31,807,309 and 29,988,619), after the new PPI, IIP and BkSPI (base 2022-23) were brought in. eSankhyiki (base_year 2022-23) holds the same updated values.
MoSPI has not yet published a back series before FY2022-23 on the new base (eSankhyiki "Back" series for base 2022-23 returns no data).
The new-series launch note (27 Feb 2026, `https://static.pib.gov.in/WriteReadData/specificdocs/documents/2026/feb/doc2026227806401.pdf`) and the 5 Jun 2026 press note (`https://static.pib.gov.in/WriteReadData/specificdocs/documents/2026/jun/doc202665884801.pdf`) are archived in `raw/mospi_nas/`.

**Deflator.** `india_gdp_deflator` = GDP at current prices / GDP at constant prices x 100, our computation, stored to 3 decimals (2011-12 = 100). `india_gdp_deflator_base2022_23` is computed the same way (2022-23 = 100).
The two series overlap only in FY2022-23 and are not linked here. A user who needs one series to FY2025-26 can either use the 2011-12 series (it runs to FY2025-26 on the First Advance Estimate) or chain the 2011-12 series to FY2022-23 with the growth of the 2022-23 series after it. Over FY2021-22 to FY2024-25 the two choices give 1.1199 and 1.1182.

## 4. Rupee-dollar reference rate (RBI)

**RBI, Handbook 2025-26, Table 133** (financial-year average and end-year rates), URL `https://www.rbi.org.in/Scripts/PublicationsView.aspx?id=23958`, and **Table 132** (calendar-year average), URL `https://www.rbi.org.in/Scripts/PublicationsView.aspx?id=23957`; raw copies in `raw/rbi_hbie2026/`.
US dollar column, rupees per dollar: FY2021-22 74.5039 (the value already used in `data/external/techno_economic/cea_cost_basis.csv`), FY2022-23 80.3635, FY2023-24 82.7897, FY2024-25 84.5756, FY2025-26 88.3071; calendar 2020 74.0996 to 2025 87.1661, with **2024 = 83.6693**.
Table notes: RBI reference rates from May 2012; Financial Benchmarks India Pvt. Ltd. (FBIL) has published the reference rates since 10 July 2018.
The RBI file server refuses scripted downloads of the XLSX versions (it returns a human-verification page), so the HTML table pages on `www.rbi.org.in` were archived instead.

## 5. RBI inflation target (section 45ZA of the RBI Act, 1934)

The Central Government, in consultation with the RBI, sets the target in terms of CPI once every five years and notifies it in the Official Gazette (section 45ZA).

* **5 Aug 2016 to 31 Mar 2021**: 4 percent CPI (Combined) inflation, lower tolerance 2 percent, upper tolerance 6 percent, Gazette notification of 5 Aug 2016. Source: RBI, Report on Currency and Finance 2020-21 (26 Feb 2021), Chapter II, paragraph II.3, `https://www.rbi.org.in/scripts/PublicationsView.aspx?id=20343` (raw copy `raw/rbi_target/RBI_RCF2021_ch2_id20343.html`).
* **1 Apr 2021 to 31 Mar 2026**: same target and band, Gazette notification of 31 Mar 2021. Source: RBI, "Review of Monetary Policy Framework - A Discussion Paper" (21 Aug 2025), Section I "Introduction and Backdrop", paragraph 6, `https://www.rbi.org.in/Scripts/PublicationsView.aspx?id=23171` (raw copy `raw/rbi_target/RBI_MPF_DiscussionPaper_id23171.html`).
* **1 Apr 2026 to 31 Mar 2031**: same target and band, **Gazette notification S.O. 1580(E) dated 25 Mar 2026**. Source: RBI, Annual Report 2025-26 (29 May 2026), Chapter III, paragraph III.5, Box III.1 and its footnote 2, `https://rbi.org.in/Scripts/AnnualReportPublications.aspx?Id=1463` (raw copy `raw/rbi_target/RBI_AnnualReport_Id1463.html`).

The CSV has one row per fiscal year covered (FY2016-17 to FY2030-31) for each of the target and the two tolerance levels. FY2016-17 is covered from 5 Aug 2016. No target has been notified beyond 31 Mar 2031.

## 6. United States price indices (BLS, BEA)

**US BLS, CPI for All Urban Consumers (CPI-U), U.S. city average, all items, not seasonally adjusted, 1982-84=100, series CUUR0000SA0.**
BLS Public Data API v2 (`https://api.bls.gov/publicAPI/v2/timeseries/data/`, annualaverage=true), period M13 "Annual"; raw copies `raw/us_bls/bls_api_v2_CUUR0000SA0_2019_2025_annual.json.gz` and the monthly values `raw/us_bls/bls_api_v1_CUUR0000SA0_2019_2025_post.json.gz`. The series page is `https://data.bls.gov/timeseries/CUUR0000SA0`.
Annual averages: 2020 258.811, 2021 270.970, 2022 292.655, 2023 304.702, 2024 313.689, 2025 321.943.
BLS published no October 2025 index (footnote "Data unavailable due to the 2025 lapse in appropriations"); the published 2025 annual average equals the mean of the 11 available months (our check: 321.943).

**US BEA, NIPA Table 1.1.9 "Implicit Price Deflators for Gross Domestic Product", line 1, 2017=100 (series A191RD).**
Annual flat file `https://apps.bea.gov/national/Release/TXT/NipaDataA.txt` (server Last-Modified 4 Nov 2026) and series register `https://apps.bea.gov/national/Release/TXT/SeriesRegister.txt` (A191RD = T10109:1); raw copies in `raw/us_bea/` (gzip).
Values: 2020 105.377, 2021 110.159, 2022 118.026, 2023 122.451, 2024 125.587, 2025 128.893.
Check: nominal GDP (A191RC) / real GDP in chained 2017 dollars (A191RX) x 100 agrees within 0.001 in every year.
BEA's API needs a registration key, so the published flat file was used.

**Federal Open Market Committee, "Statement on Longer-Run Goals and Monetary Policy Strategy", adopted effective 24 Jan 2012, as reaffirmed effective 27 Jan 2026.**
URL `https://www.federalreserve.gov/monetarypolicy/files/FOMC_LongerRunGoals.pdf`, fetched and read on 2026-11-21; raw copy `raw/us_fed/FOMC_LongerRunGoals.pdf`.
Page 1, fourth paragraph: "The Committee reaffirms its judgment that inflation at the rate of 2 percent, as measured by the annual change in the price index for personal consumption expenditures, is most consistent over the longer run with the Federal Reserve's statutory maximum employment and price stability mandates."
Row `us_fomc_inflation_goal_pce`, period 2026 (the year of the latest reaffirmation), value 2.
The goal is stated for the PCE price index, not the GDP deflator. The model uses it only to extend the US GDP deflator past 2025 and for the information-only nominal USD outlook (`site_summary.json`, `nominal_outlook`); no headline cost uses it.

## 7. Price basis of the organiser dataset's GSDP

Question: is `GSDP (crore INR)` in `data/raw/competition/renewable_energy_dataset.csv` at constant or current prices?

* The repository copy is byte-identical to the upstream file `data/renewable_energy_dataset.csv` of `https://github.com/chrisjaimyantony/renewable-india-dataset` (SHA-1 37f2078a...; upstream commit 89d5ef73 of 3 Jul 2025). Raw copies of the upstream README, CSV and notebook are in `raw/upstream_renewable_india/` (gzip).
* The upstream README lists "RBI & Planning Commission (GSDP data)" as the source and says that some missing values were predicted with XGBoost regression. Its disclaimer says "GSDP values are nominal and not inflation-adjusted". **The data contradict this statement.**
* Check against MoSPI state GSDP on the 2011-12 base (eSankhyiki, account type Regional, indicator Gross State Domestic Product; raw copy `raw/mospi_nas/esankhyiki_nas_gsdp_2011_12_20261121.json.gz`), our comparison:
  * Maharashtra 2012: dataset 1,357,942; MoSPI FY2012-13 constant (2011-12) prices 1,357,942; current prices 1,459,629.
  * Tamil Nadu 2016: dataset 1,036,762; MoSPI FY2016-17 constant 1,036,762; current 1,302,639.
  * Karnataka 2015: dataset 831,330; MoSPI FY2015-16 constant 831,322; current 1,045,168.
  * Over all states that match by name, 25 to 27 of 29 or 30 states equal the MoSPI constant-price value within 0.1 percent in each year 2012 to 2017 (23 in 2018); only one state equals the current-price value. Seven large states checked against RBI Handbook of Statistics on Indian States Tables 21 and 22 (current and constant prices, in `data/external/economy.csv`) give the same result.
* **Finding: from 2011 the values are at constant 2011-12 prices, with dataset year t matching fiscal year t to t+1.** The exception is Jammu and Kashmir, whose values from 2012 equal MoSPI current prices (for example 2016: 124,848 against current 124,848 and constant 100,203).
* From 2019 onward most values no longer match any MoSPI vintage (for example Maharashtra 2022: 1,974,483 against 2,286,037 constant), and 2021 to 2023 rise by nearly equal steps in several states, which fits the README's statement that missing values were predicted. The 2024 values are below 2023 in 32 of 34 states (equal in the other 2).
* Values before 2011 were not checked against an official 2004-05 series here; the data audit already treats them as constant 2004-05 prices with a base change in 2011.

---

## Raw files

| Folder under `raw/` | Content |
|---|---|
| `rbi_hbie2026/` | RBI Handbook of Statistics on the Indian Economy 2025-26: Tables 1, 2, 33, 34, 132, 133, 235, 236 (HTML pages) |
| `rbi_hbie2025/` | RBI Handbook 2024-25: Tables 1 and 2 on the 2011-12 base (HTML pages) |
| `mospi_cpi/` | eSankhyiki CPI monthly data (JSON, gzip); MoSPI CPI press release of 12 Feb 2026 (PIB copy); Expert Group report on the CPI revision (January 2026) |
| `mospi_nas/` | eSankhyiki national GDP (all vintages) and state GSDP on the 2011-12 base (JSON, gzip); MoSPI GDP press notes of 28 Feb 2025, 30 May 2025, 7 Jan 2026, 27 Feb 2026, 5 Jun 2026 and the NAS 2026 press release of 31 Aug 2026 |
| `oea_wpi/` | OEA WPI 2011-12 series (financial-year, calendar-year and monthly index files); WPI 2022-23 yearly index file; download and linking-factor pages |
| `rbi_target/` | RBI Annual Report 2025-26 Chapter III, Report on Currency and Finance 2020-21 Chapter II, Monetary Policy Framework discussion paper (HTML pages) |
| `us_bls/` | BLS API responses for CUUR0000SA0 (annual averages and monthly values, gzip) |
| `us_bea/` | BEA NIPA annual flat file and series register (gzip) |
| `us_fed/` | FOMC Statement on Longer-Run Goals and Monetary Policy Strategy (PDF, reaffirmed effective 27 Jan 2026) |
| `upstream_renewable_india/` | Upstream README, CSV and notebook of the organiser dataset (gzip) |

## Not found or not used

* An official FY2025-26 CPI average on the 2012=100 base (the series ended in December 2025); the row is DERIVED with MoSPI's linking factor.
* An official FY2025-26 CPI inflation rate for the full year; RBI publishes only the two part-year rates, so the row is DERIVED.
* An official back series of GDP before FY2022-23 on the 2022-23 base.
* The Gazette numbers of the 2016 and 2021 inflation-target notifications (the RBI sources give only their dates); the 2026 notification number, S.O. 1580(E), is given by the RBI Annual Report. The Gazette texts themselves were not fetched.
* The XLSX versions of the RBI Handbook tables (the RBI file server refused scripted downloads); the HTML tables were used.
* A calendar-year India GDP deflator. The deflators here are by fiscal year; converting an INR cost to a calendar-2024 price level needs either FY2024-25 as a proxy or a calendar-year deflator built from MoSPI's quarterly estimates, which were not fetched.
