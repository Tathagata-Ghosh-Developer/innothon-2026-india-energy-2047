# External datasets: sources and extraction notes

State-level drivers and resource potential for the India 2047 generation-requirement forecast and the regional generation-mix allocation. Every value in these CSVs was read from a document that was downloaded and archived under `data/external/raw/` (one subfolder per source). Nothing was filled from memory. A blank value means the fetched source did not give it, and the row's `note` says why.

All CSVs are UTF-8, comma-separated, with a header row. Each has a `source_url` column, except `state_region_map.csv`, which has `area_source_url` and `region_source_url`. Units are in a `unit` column or in the column name.

## Files

| file | rows | content |
|---|---|---|
| `state_region_map.csv` | 39 | 36 states/UTs, 2 composite labels, 1 India row: ISO code, grid region, UT flag, area |
| `population_projection.csv` | 2,029 | Technical Group state projections 2011-2036 (1 March and 1 July), UN WPP 2024 India (Medium 2000-2050; Low/High 2024-2050) |
| `urbanization.csv` | 294 | Census 2011 and projected (2021-2036) state urban share; UN WUP 2018 and WUP 2025 national series to 2050 |
| `economy.csv` | 1,005 | GSDP at constant 2011-12 prices and at current prices, 2011-12 to 2024-25; national real GDP (RBI, World Bank WDI) |
| `resource_potential.csv` | 922 | Solar, wind (120 m, 150 m), large hydro, small hydro, pumped storage, biomass, bagasse, coal and lignite by state |
| `climate_land.csv` | 476 | Area, forest and tree cover (ISFR 2023), wasteland (2008-09, 2015-16), mean GHI, wind power density at 100 m |
| `nuclear_sites.csv` | 40 | Operating units, units under construction, sanctioned projects and in-principle sites, by state |
| `policy_targets.csv` | 147 | National targets (500 GW, net zero 2070, NDC 2022, 100 GW nuclear) and NEP 2022-32 demand, capacity and generation benchmarks |
| `nep2023_state_demand.csv` | 135 | NEP 2022-32 / 20th EPS state-wise energy requirement (MU) and peak demand (MW) for 2021-22, 2026-27, 2031-32 |
| `new_loads.csv` | 5 | Secondary. Published 2050 electricity estimates (low, central, high TWh) for electric vehicles, green hydrogen, data centres, space cooling and electric cooking, for a separate scenario outside the headline |

The helper and extraction scripts were kept outside the repository and are not committed. The prose below is the record of each extraction step (table, page, filters, unit conversions), detailed enough to repeat by hand or in any language.

## State and UT naming and mapping (applies to every file)

The `state` column uses one canonical name for each of the 36 current units:

- **States (28):** Andhra Pradesh, Arunachal Pradesh, Assam, Bihar, Chhattisgarh, Goa, Gujarat, Haryana, Himachal Pradesh, Jharkhand, Karnataka, Kerala, Madhya Pradesh, Maharashtra, Manipur, Meghalaya, Mizoram, Nagaland, Odisha, Punjab, Rajasthan, Sikkim, Tamil Nadu, Telangana, Tripura, Uttar Pradesh, Uttarakhand, West Bengal.
- **UTs (8):** Andaman and Nicobar Islands, Chandigarh, Dadra and Nagar Haveli and Daman and Diu, Delhi, Jammu and Kashmir, Ladakh, Lakshadweep, Puducherry.

The source's own spelling is kept in `state_as_reported`. A national total printed by a source is kept as `India`.

Reorganised units are handled in one way throughout, and values are never split between units:

- **Telangana / Andhra Pradesh (2014).** Every source used reports the two states separately, so no composite was needed. The label `Andhra Pradesh (undivided)` is defined in `state_region_map.csv` for completeness but appears in no other file.
- **Ladakh / Jammu and Kashmir (2019).**
  - A source that reports the pre-2019 state, or does not split it, is stored under `Jammu and Kashmir (incl. Ladakh)`.
  - This covers the NIWE 120 m wind atlas, the MNRE potential tables (except wind 150 m, which is attributed to Ladakh), the Wastelands Atlas, and GSDP for 2011-12 to 2018-19 (per the RBI footnote).
  - Where a source reports the two UTs separately (Technical Group projections, NEP, CEA hydro, MNRE 2024-25 solar), they are kept separate.
- **Dadra and Nagar Haveli and Daman and Diu (merged 2020).**
  - Where a source prints the two former UTs separately, the values are summed into the merged UT and the note says so. This applies to the Technical Group population, the NEP energy requirement and the Wastelands Atlas district rows.
  - Shares are recomputed from the summed counts.
  - Peak demand is not summed, because state peaks are non-coincident. Those rows are kept as `component` rows in `nep2023_state_demand.csv`.
- **Non-state labels:**
  - `Others` in `resource_potential.csv` is an MNRE row not attributed to any state: 284.4 MW of bagasse cogeneration and 0.79 GW of the 2014 solar estimate.
  - `DVC` in `nep2023_state_demand.csv` is the Damodar Valley Corporation command area, in parts of Jharkhand and West Bengal.
  - Neither label joins to a state.
- **Region totals are unaffected by these choices.** Both members of each composite, and DVC's two host states, lie in a single grid region.

---

## 1. state_region_map.csv

Columns: `state, state_code, grid_region, is_ut, row_type, area_km2, area_note, members, region_source, code_source, area_source_url, region_source_url`.

`row_type` is one of `state`, `union_territory`, `composite` or `national_total`. Filter to `state` and `union_territory` for the 36 current units.

**Grid region**
- Source: Central Electricity Authority (CEA), National Electricity Plan (Volume I, Generation) 2022-32, notified in Gazette No. 3189 dated 18.05.2023, Annexure 4.1 (PDF page 125, printed page 4.5).
- The original URL https://cea.nic.in/wp-content/uploads/document_upload/2023/06/NEP_2022_32_FINAL_GAZETTE.pdf now returns 404 on the redesigned CEA site. The Internet Archive capture of that URL dated 16 Jul 2025 was used: https://web.archive.org/web/20250716152340/http://cea.nic.in/wp-content/uploads/document_upload/2023/06/NEP_2022_32_FINAL_GAZETTE.pdf (raw: `raw/nep2023/NEP_2022_32_FINAL_GAZETTE.pdf`).
- Extraction:
  - Annexure 4.1 lists each state/UT followed by a regional subtotal row: "Northern Region", "Western Region", "Southern Region", "Eastern Region", "North Eastern Region".
  - Each state takes the region of the next subtotal row below it.
  - Names as printed: "J&K" is Jammu and Kashmir, "A&N" is Andaman and Nicobar Islands, and "AP" inside the North Eastern block is Arunachal Pradesh. "Dadar Nagar Haveli" and "Daman & Diu" are both in the Western block, so the merged UT is Western. "DVC" was skipped.
  - Region labels are shortened to Northern, Western, Southern, Eastern and North-Eastern.

**State code**
- ISO 3166-2:IN subdivision codes, read from the table on https://en.wikipedia.org/wiki/ISO_3166-2:IN (fetched 15 Nov 2026; raw: `raw/iso3166/ISO_3166-2_IN_wikipedia.html`). The page cites the ISO Online Browsing Platform.
- Names were matched after removing diacritics and the parenthetical "(Pondicherry)".
- Codes include IN-TS (Telangana), IN-LA (Ladakh), IN-DH (Dadra and Nagar Haveli and Daman and Diu) and IN-OD (Odisha).
- The page's "Union territory" category was checked against the `is_ut` flag; all 36 agree.

**Area (km2)**
- Source: Forest Survey of India, India State of Forest Report (ISFR) 2023 Vol I, Table 2.4, column A "Geographical Area" (PDF page 84). Taken from `climate_land.csv`; see section 6.
- Jammu and Kashmir (54,633 km2) and Ladakh (168,055 km2) are the Survey of India shapefile areas printed in brackets in ISFR Table 1.3. ISFR states that it has not received separate geographical areas for the two UTs.
- The composite row carries the unified figure of 222,236 km2 that ISFR prints. The two shapefile areas sum to 222,688 km2, which is 452 km2 more.
- `Andhra Pradesh (undivided)` is the sum of the ISFR areas of Andhra Pradesh and Telangana.
- `India` = 3,287,468.88 km2 (ISFR total, which uses the unified J&K figure).

**Caveats**
- The Census of India site (censusindia.gov.in) timed out from this network, so Census area tables could not be fetched directly.
- A&N Islands and Lakshadweep are island UTs that the NEP lists under the Eastern and Southern regions. Their region assignment is administrative, not a statement about grid connection.

---

## 2. population_projection.csv

Columns: `state, state_as_reported, year, reference_date, population, urban_population, urban_share, variant, source, source_url, note`.

- `population` and `urban_population` are persons.
- `urban_share` is a fraction (0-1).
- `variant` is one of "Technical Group projection", "UN WPP 2024 Medium", "UN WPP 2024 Low" or "UN WPP 2024 High".

### 2a. Technical Group on Population Projections (state totals and urban, 2011-2036)

- **Document:** "Population Projections for India and States 2011-2036", Report of the Technical Group on Population Projections, National Commission on Population, Ministry of Health and Family Welfare. The fetched PDF's cover page reads "November, 2019"; the report is commonly referred to by its July 2020 release.
- **URL:** https://nhm.gov.in/New_Updates_2018/Report_Population_Projection_2019.pdf (raw: `raw/techgroup_popproj/Report_Population_Projection_2019.pdf`, 268 pages).
- **Units in source:** thousands of persons and percent. Converted to persons (x 1000) and to a fraction (/ 100).
- **Tables used** (PDF pages; Persons columns only):

  | Table | Content | Reference date | PDF pages |
  |---|---|---|---|
  | 8 | Total population | 1 March | 47-59 |
  | 9 | Urban population | 1 March | 60-72 |
  | 10 | Urban percent | 1 March | 73-85 |
  | 11 | Total population | 1 July | 86-98 |
  | 12 | Urban population | 1 July | 99-111 |
  | 13 | Urban percent | 1 July | 112-124 |

  Table 14 (1 October) was not used.
- **Extraction:**
  - Each page holds two or three areas, each with Persons, Male and Female columns, and 26 year rows (2011-2036).
  - Plain text order is unreliable, because some columns are printed on a shifted baseline. Each number was therefore assigned to a column by its x position.
  - Column anchors come from the printed column-number row ("1 2 3 ..."). Where that row is misprinted (Table 13, page 112, prints "7 5 9"), the Persons/Male/Female header words were used instead.
  - Area names are read from the header block above each three-column group, so names that wrap over two lines are handled.
  - Values in each Persons column were ordered top to bottom and mapped to 2011-2036. Exactly 26 values were required per column.
  - The printed page number at the foot of each page was excluded.
  - 38 areas per table: India, 35 states/UTs, plus Dadra & Nagar Haveli and Daman & Diu printed separately. Jammu & Kashmir (UT), Ladakh and Telangana are printed separately in the report.
- **Merged UT:** Dadra & Nagar Haveli + Daman & Diu were summed. The urban share was recomputed as summed urban population / summed total.
- **Checks:**
  - The sum of areas equals the printed INDIA row within 3 thousand (1 March) and 38 thousand (1 July).
  - The printed urban percent equals urban / total within 0.6 percentage points for every area and year, except Lakshadweep, where rounding of small counts gives gaps up to 1.1 points.
  - Spot values matched the PDF text: India 1 July 2036 = 1,521,140 thousand; J&K 1 July 2011 = 12,309; Telangana 2036 = 39,480; Ladakh 2036 = 316.
- **Census anchor:** the INDIA row for 1 March 2011 is 1,210,855 thousand total and 377,106 thousand urban (31.14%). These are the Census 2011 counts.
- **Headline figures:** India 1 March 2036 = 1,518,288 thousand (the summary's "151.8 crores"); 1 July 2036 = 1,521,140 thousand.
- **Caveats:**
  - The report projects India's urban population separately. Its INDIA urban total exceeds the sum of state urban totals by a gap that grows from 15 thousand (2011) to 7.38 million (2036, 1 July). State urban values are as printed.
  - The summary text (PDF page 7) says urban share rises "31.8 percent in 2011 ... to 38.6 percent by 2036". This does not match the report's own Tables 10 and 13 (31.14% to 39.58% on 1 March; 31.26% to 39.69% on 1 July). The table values are used.
  - The projections stop at 2036. Extension to 2047 is left to the model.

### 2b. UN World Population Prospects 2024 (national, to 2050)

- **Publisher:** UN DESA Population Division, World Population Prospects 2024 (July 2024).
- **URL:** https://population.un.org/wpp/assets/Excel%20Files/1_Indicator%20(Standard)/CSV_FILES/WPP2024_TotalPopulationBySex.csv.gz (raw: `raw/unwpp2024/`). The CSV was used instead of the Data Portal API, which needs a token.
- **Extraction:**
  - Rows with LocID 356 (India).
  - Variant "Medium" for 2000-2050 (2000-2023 are estimates, 2024 onward projections), and "Low" and "High" for 2024-2050.
  - Column PopTotal (thousands, 1 July) x 1000.
- **Headline figures:** Medium 2036 = 1,588.3 million, 2047 = 1,666.6 million, 2050 = 1,679.6 million. Low 2050 = 1,548.0 million. High 2050 = 1,812.4 million.
- **Caveat:** WPP's own 2011 estimate for India is 1,261.2 million, against the Census-based 1,216.1 million in the Technical Group (1 July 2011). The 2036 Medium figure is 4.4% above the Technical Group. The two series are on different bases, so splice with care.

---

## 3. urbanization.csv

Columns: `state, state_as_reported, year, reference_date, measure, value, unit, concept, status, source, source_url, note`. `value` is the urban share in percent.

`concept` separates three definitions that must not be mixed:

- **"national definition (Census of India)"**
  - From Technical Group Table 10 (1 March; PDF pages 73-85; extraction as in 2a), for 2011, 2021, 2026, 2031 and 2036.
  - The 2011 rows (`status` = "Census 2011") fall on the Census 2011 reference date. India 2011 = 31.14%, which is the Census 2011 urban share (377,106 thousand / 1,210,855 thousand).
  - Jammu and Kashmir 2011 is the UT as split out by the Technical Group. Census 2011 itself reports the undivided state.
  - The merged UT is recomputed from Tables 8 and 9.
  - 37 areas (36 units plus India) for each year.
  - Headline: India 2036 = 39.58%.
- **"national definition (as used by UN WUP 2018)"**
  - UN DESA, World Urbanization Prospects 2018, File 2 "Percentage of Population at Mid-Year Residing in Urban Areas", 1950-2050, five-yearly. India row (country code 356); header row 17 of the sheet.
  - The original URL https://population.un.org/wup/Download/Files/WUP2018-F02-Proportion_Urban.xls is no longer served on the redesigned UN site. The Internet Archive capture of 24 Aug 2024 was used: https://web.archive.org/web/20240824204701/https://population.un.org/wup/Download/Files/WUP2018-F02-Proportion_Urban.xls (raw: `raw/unwup/WUP2018-F02-Proportion_Urban.xls`).
  - India: 2015 = 32.777%, 2030 = 40.142%, 2050 = 52.841%.
  - The annual file (F21) could not be retrieved; the archive returned HTTP 429.
- **"Degree of Urbanisation: cities / towns and semi-dense areas / cities and towns / rural areas"**
  - UN DESA, World Urbanization Prospects 2025, File 2, https://population.un.org/wup/assets/Download/Countries%20and%20Aggregates/WUP2025-F02-Degree-of-Urbanization_percPop_by_category.xlsx (raw: `raw/unwup/`).
  - The India row from each of the sheets Cities, Towns, Cities and Towns, and Rural. Kept for every fifth year from 1950 to 2050, plus 2011.
  - This is a harmonised grid-based classification: cities at 1,500 or more inhabitants per km2 and 50,000 or more people; towns and semi-dense areas at 300 or more per km2 and 5,000 or more people.
  - It gives India 80.8% "cities and towns" in 2011, which is not comparable with the census urban share. Do not use it as a substitute.

---

## 4. economy.csv

Columns: `state, state_as_reported, fiscal_year, year_start, measure, price_basis, value, unit, source, source_url, note`. All values are in INR crore: source Rs lakh / 100, and WDI rupees / 1e7.

**4a. State GSDP (RBI Handbook of Statistics on Indian States 2024-25, released 11 Dec 2025; data from NSO, MoSPI)**
- **Table 22, GSDP at constant (2011-12) prices, Rs lakh:** https://rbidocs.rbi.org.in/rdocs/Publications/DOCs/22T_11122025E6BC0CB35180406EAB6E0D49DE51C8E8.XLSX
- **Table 21, GSDP at current prices, Rs lakh:** https://rbidocs.rbi.org.in/rdocs/Publications/DOCs/21T_11122025D994949B48C44B68B4465FBB9ADDFF3D.XLSX
- Both were found on https://www.rbi.org.in/Scripts/AnnualPublications.aspx?head=Handbook+of+Statistics+on+Indian+States (raw: `raw/rbi_hsis/`).
- **Extraction:**
  - Both sheets of each workbook were read: (i) covers 2011-12 to 2016-17 and (ii) covers 2017-18 to 2024-25.
  - Fiscal years come from row 5 and state names from column B. Footnote rows were skipped.
  - "-" (not available) was kept as blank, with a note.
- **Jammu and Kashmir:** the row "Jammu & Kashmir*" carries the RBI footnote "For the years 2011-12 till 2018-19, figures relate to Jammu and Kashmir & Ladakh and for the years 2019-20 to 2024-25, relate to UT of Jammu and Kashmir". It is stored as `Jammu and Kashmir (incl. Ladakh)` for 2011-12 to 2018-19 and as `Jammu and Kashmir` from 2019-20.
- **Coverage:**
  - 33 units at constant prices. There is no Ladakh, Lakshadweep or merged DNH&DD GSDP.
  - Current prices add a Ladakh row, with values for 2022-23 and 2023-24 only.
  - Not available at constant prices: Gujarat 2023-24, and 2024-25 for A&N, Chandigarh, Goa, Gujarat, Manipur, Mizoram, Nagaland and Sikkim.
- **Real values are available, so nominal is supplementary.** Real GSDP is `price_basis = "constant 2011-12 prices"`.

**4b. National real GDP**
- **RBI Handbook of Statistics on the Indian Economy 2025-26 (31 Jul 2026), Table 2, Macro-Economic Aggregates (At Constant Prices), Rs crore:** https://rbidocs.rbi.org.in/rdocs/Publications/DOCs/2T_HBIE310720261ED415A38F9E465C9B9BEDEA897741FB.XLSX (raw: `raw/rbi_hbie/`).
  - Row "Gross Domestic Product".
  - 2020-21 to 2022-23 are at 2011-12 prices; 2022-23 to 2025-26 are at 2022-23 prices, the new NSO series. The base is read from the table's row 7.
  - 2024-25 is the First Revised Estimate and 2025-26 the Provisional Estimate.
- **World Bank WDI NY.GDP.MKTP.KN (GDP, constant LCU), India, 1960-2025:** https://api.worldbank.org/v2/country/IND/indicator/NY.GDP.MKTP.KN?format=json&per_page=200 (last updated 11 Nov 2026; raw JSON and indicator metadata in `raw/worldbank_wdi/`).
  - WDI year t is Indian fiscal year t to t+1, stored as `fiscal_year` "t-(t+1)".
  - Rupees were divided by 1e7 to give crore.
  - WDI values for 2022-23 to 2024-25 equal RBI's 2022-23-base figures exactly. So WDI carries the new base, linked back to 1960 by the World Bank; the linking method is not stated in the fetched metadata.
  - WDI 2025-26 (32,257,756 crore) differs from RBI's Provisional Estimate (32,312,034 crore), a different vintage.
- **Caveats:**
  - State GSDP (2011-12 base) and national GDP (2022-23 base) are on different price bases. Use state shares of the sum of GSDP rather than mixing levels.
  - The sum of GSDP is not equal to national GDP.

---

## 5. resource_potential.csv

Long format: `state, state_as_reported, resource, metric, value, unit, as_of, source_document, source_url, note` (922 data rows).
`state` follows the shared canonical list. Extra labels used: `India` (the source's own all-India total row, kept for cross-checks), `Jammu and Kashmir (incl. Ladakh)` (composite; sources that predate or do not split the 2019 reorganisation), and `Others` (MNRE table row "Others", not attributable to a state; 284.4 MW bagasse cogeneration and 0.79 GW of the 2014 solar estimate). Sum of state rows (including composite and Others) equals the source's India row for every resource/metric.

Resources and metrics:

| resource | metric(s) | unit | source |
|---|---|---|---|
| solar | technical_potential_nise_2014 | GW | MNRE RE Statistics 2023-24, Table 5.1 |
| solar | ground_mounted_potential_nise_2025 | GW | MNRE RE Statistics 2024-25, Table 7.1 |
| wind_120m | potential; potential_cuf_25_28pct ... potential_cuf_gt_35pct | GW | NIWE 120 m atlas, Table E.1 |
| wind_150m | potential | GW | MNRE RE Statistics 2023-24, Table 5.1 (NIWE 2023) |
| large_hydro | identified_capacity_reassessment_2017_23_mw, ..._1978_87_mw, in_operation_mw, under_construction_mw, concurred_or_appraised_not_yet_under_construction_mw, under_examination_in_cea_mw, returned_to_project_authorities_mw, under_survey_and_investigation_mw, yet_to_be_realised_mw | MW | CEA status of large hydro potential development, 31.10.2026 |
| small_hydro | potential | MW | MNRE RE Statistics 2023-24, Table 5.1 |
| pumped_storage | exploitable_potential_mw, exploitable_potential_n_projects, in_operation_mw, under_construction_mw, concurred_not_yet_under_construction_mw, under_survey_and_investigation_mw, balance_not_yet_taken_up_mw (state rows); potential_on_stream / off_stream_open_loop / off_stream_closed_loop / total (India rows, MW and count) | MW, count | CEA State Profile on Hydro Development (2026-10); CEA Status of PSP Development (2026-10) |
| biomass | biomass_power_potential | MW | MNRE RE Statistics 2023-24, Table 5.1 |
| bagasse_cogeneration | potential | MW | MNRE RE Statistics 2023-24, Table 5.1 |
| coal, lignite | measured_331_mt, indicated_332_mt, inferred_333_mt, total_resource_mt | million tonnes | Ministry of Coal Annual Report 2025-26, Ch. 2 |

### 5a. MNRE Renewable Energy Statistics 2023-24 (solar 2014, wind 150 m, small hydro, biomass, bagasse)
- URL: https://cdnbbsr.s3waas.gov.in/s3716e1b8c6cd17b771da77391355749f3/uploads/2024/10/20241029512325464.pdf
- Publisher: Ministry of New and Renewable Energy (Statistics Division). Published Oct 2024; table reflects potential estimates as quoted in the report (solar: NISE 2014; wind: NIWE 2023 at 150 m agl; small hydro: IIT Roorkee 2016, projects up to 25 MW; biomass and bagasse cogeneration: ASCI 2021).
- Raw file: `data/external/raw/mnre_re_statistics/RE_Statistics_2023-24_MNRE.pdf`
- Extraction: PDF page 37 (printed page 27), "Table 5.1 Estimated potential in RE Sector (in MW)", read with pdfplumber `extract_tables()`. Columns used: Wind Power, Small Hydro Power, Biomass Power, Bagasse Cogeneration, Solar Power (the Large Hydro column was not used; CEA is the large hydro source). Multi-line state names were rejoined. Solar and wind were divided by 1000 (MW to GW); the other columns are kept in MW. Zeros printed in the table are kept as 0.
- Mapping: "Orissa" to Odisha; "Andaman & Nicobar" to Andaman and Nicobar Islands; "Dadar & Nagar Haveli & Daman and Diu" to the merged UT; "UT of Jammu & Kashmir (including Ladakh)" to the composite label, except the wind cell, which reads "1 (Ladakh)" and is assigned to Ladakh (0.001 GW) with Jammu and Kashmir set to 0 (RE Statistics 2024-25 shows the same split: J&K blank, Ladakh 1 MW).
- Totals: solar 748.99 GW; wind 150 m 1163.856 GW; small hydro 21,133.62 MW; biomass 28,446.91 MW; bagasse cogeneration 13,818.4 MW.
- Caveats: the 2014 solar figure is a land-based estimate (3% of wasteland) that MNRE has since superseded (see 5b). Number of small hydro sites is not given in this table. Wind 120 m and 150 m figures are "installable potential" from different NIWE studies and are not additive.

### 5b. MNRE Renewable Energy Statistics 2024-25 (solar ground-mounted, NISE 2025)
- URL: https://cdnbbsr.s3waas.gov.in/s3716e1b8c6cd17b771da77391355749f3/uploads/2025/11/20251106791170361.pdf
- Publisher: MNRE (Statistics Division), Nov 2025. Solar column from NISE report "Solar PV Potential of India (Ground Mounted)", Sep 2025.
- Raw file: `data/external/raw/mnre_re_statistics/RE_Statistics_2024-25_MNRE.pdf`
- Extraction: PDF page 45 (printed page 36), "Table 7.1 Estimated potential in RE Sector (in MW)", column "Solar Power*" (footnote: ground mounted) only, via pdfplumber `extract_tables()`; divided by 1000 to GW. This table lists Jammu and Kashmir (8.58894 GW) and Ladakh (8.55664 GW) separately for solar. Blank cells (Lakshadweep, Others) are left blank.
- Total: 3,343.37839 GW.
- Caveat: this table prints blanks where the 2023-24 edition printed 0, so it was used only for the new solar column.

### 5c. NIWE India's Wind Potential Atlas at 120 m agl (wind 120 m)
- URL: https://niwe.res.in/static/pdf/India%27s_Wind_Potential_Atlas_at_120m_agl.pdf (linked from https://niwe.res.in/department/department_wra/)
- Publisher: National Institute of Wind Energy, Chennai (MNRE), October 2019.
- Raw file: `data/external/raw/niwe_wind_atlas/Indias_Wind_Potential_Atlas_at_120m_agl.pdf`
- Extraction: PDF page 14 (printed page 3), "Table E.1: State-wise detailed Wind Potential at 120m agl", text lines parsed with a regex (state name followed by six integers). Column order (CUF 25-28%, 28-30%, 30-32%, 32-35%, >35%, Total) confirmed against Table 6 on PDF page 39, which repeats the same figures with a clean header. Values divided by 1000 (MW to GW). Base case land availability WA120-80:30:5 (80% of wasteland, 30% of cultivable land, 5% of forest land within >25% CUF areas).
- Mapping: "Jammu & Kashmir*" to the composite label (atlas predates the Oct 2019 reorganisation); "Daman, Diu, Dadra*" to the merged UT; "A & N Islands" to Andaman and Nicobar Islands. Asterisked states are "negligible by method" per the source footnote.
- Total: 695.509 GW (CUF bands 340.574 / 140.429 / 82.208 / 75.004 / 57.294 GW).

### 5d. CEA Status of Large Hydro Electric Potential Development (large hydro)
- URL: https://cea.nic.in/api/dashboardReports/download-pdf/en/c7ac51a9-c51e-473f-913f-8e06696220bb (listed at https://cea.nic.in/reports/dashboardView?headingId=107&subHeadingId=1003&subSubHeadingId=10036&lang=en as "Hydro Electric Potential Development-Region wise 2026-10")
- Publisher: Central Electricity Authority; status as on 31.10.2026. Projects above 25 MW.
- Raw file: `data/external/raw/cea_hydro/CEA_Hydro_Potential_Development_Region_wise_2026-10.pdf`
- Extraction: single page; every line with a state name followed by 16 numbers was parsed. The header is printed as overlapping rotated text, so it was rebuilt from character positions: (1) identified capacity as per reassessment study 1978-87, (2) exploitable identified capacity as per reassessment study 2017-23, then MW and % pairs for (3) in operation, (4) under construction, (5) concurred by CEA and yet to be taken up / appraised by CEA, (6) under examination in CEA, (7) returned to project authorities, (8) under survey and investigation, (9) yet to be realised. Only MW values were kept; regional sub-total rows were dropped; "ALL INDIA" kept as `India`.
- Coverage: 30 states/UTs as listed by CEA (J&K and Ladakh separate; UTs without hydro potential are not listed).
- Totals: 2017-23 reassessment 133,410 MW (state rows sum to 133,410.03); in operation 44,889.07 MW; under construction 13,023.5 MW; 1978-87 study 145,320 MW.
- Caveats: Rajasthan and Tamil Nadu show operating capacity at or above the identified potential; source footnote says "yet to be realised" is not an arithmetic remainder. The list includes three multipurpose projects (Upper Siang 11,200 MW, Bursar 800 MW, Kishau 422 MW).

### 5e. CEA State Profile on Hydro Development and Status of PSP Development (pumped storage)
- URLs: https://cea.nic.in/api/dashboardReports/download-pdf/en/770f1d78-78ec-4513-a8d5-2034b27af59a ("State Profile on Hydro Development 2026-10", listed under subSubHeadingId=10038) and https://cea.nic.in/api/dashboardReports/download-pdf/en/35e65bfd-4942-4bd4-8140-252d529ac876 ("Status of Pumped Storage Development in India 2026-10", subSubHeadingId=10039).
- Publisher: CEA, October 2026 (profile pages dated 11-11-26).
- Raw files: `data/external/raw/cea_hydro/CEA_State_Profile_on_Hydro_Development_2026-10.pdf`, `data/external/raw/cea_psp/CEA_Status_of_PSP_Development_2026-10.pdf`
- Extraction (state rows): every profile page containing "Breakup of Hydro Power Potential from Pumped Storage" (21 pages) was read; the state comes from the page title ("HP", "UP", "MP", "AP" expanded). "Exploitable Hydro Power Potential from Pump(ed) Storage Projects" gives number of projects and installed capacity (MW). The breakup lines that follow (in operation, under construction, concurred and yet to be taken up, under S&I, balance) were classified by keyword; labels split over two lines were rejoined. Each state's breakup MW was checked against its potential (all 21 match; Uttar Pradesh project counts in the breakup sum to 12 against 13 in the total, noted in the row).
- Extraction (India rows): page 1 of the status report, "Potential" row: on-stream 62 projects / 56,671 MW; off-stream open loop 77 / 89,793 MW; off-stream closed loop 183 / 200,659 MW; total 322 / 347,122.6 MW; "In Operation" total 7,425.6 MW; "Under Construction" total 16,820 MW.
- Check: the 21 state potentials sum to 347,122.6 MW and 322 projects, matching the status report. State in-operation (7,425.6 MW), under-construction (16,820 MW), concurred (10,400 MW) and S&I (95,550 MW) sums also match page 1.
- Coverage: 21 states with identified PSP sites; states without a PSP section in the profile have no rows (not zero-filled).
- Caveats: Gujarat's 1,440 MW "in operation" (Kadana, Sardar Sarovar) is noted by CEA as not working in pumping mode. Identified PSP potential is a site list that keeps growing (it is not a resource ceiling).

### 5f. Ministry of Coal Annual Report 2025-26, Chapter 2 (coal and lignite)
- URL: https://www.coal.nic.in/sites/default/files/2026-02/chap2AnnualReport2026en.pdf
- Publisher: Ministry of Coal, Annual Report 2025-26 (Feb 2026), citing the GSI inventory of geological resources of Indian coal as on 01.04.2025 (estimates by CMPDI, MECL, GSI, SCCL and others; up to 1200 m depth; seams 0.9 m and above). Marked provisional.
- Raw file: `data/external/raw/moc_coal_lignite/MoC_AnnualReport_2025-26_chap2.pdf`
- Extraction: PDF page 4, block "State-wise Coal Resources" (15 states + Total) and PDF page 5, block "State-wise Lignite Resources" (8 states/UTs + Total); text lines parsed by regex (name followed by four numbers: Measured (331), Indicated (332), Inferred (333), Resource). Upper-case names title-cased; "Tamilnadu" to Tamil Nadu, "Pondicherry" to Puducherry, "Jammu & Kashmir" to Jammu and Kashmir.
- Totals: coal 400,715.45 Mt (measured 220,412.34; indicated 149,048.07; inferred 31,255.04); lignite 47,370.54 Mt.
- Caveats: "Measured (331)" is the proved category. The state-table inferred column (31,255.04 Mt) equals the type-wise table's inferred (30,506.45) plus "exploration mapping" tertiary coal (748.58), so it includes the latter. These are geological resources, not extractable reserves.

### Gaps (resource_potential.csv)
- Small hydro: number of potential sites by state not found in the fetched tables (MNRE overview pages are client-rendered and returned no content to a plain download).
- Wind at 120 m is only available from the 2019 atlas (J&K and Ladakh combined); wind at 150 m is only available as a single state column (no CUF breakdown) from the MNRE statistics table; NIWE's own 150 m report was not located.
- Waste-to-energy potential is not included (not in the fetched tables).
- PSP potential exists only for the 21 states listed by CEA.

---

## 6. climate_land.csv

Long format: `state, state_as_reported, variable, value, unit, year, source_document, source_url, note`.
36 current units plus `Jammu and Kashmir (incl. Ladakh)` (composite) and `India` where the source prints a total. Blank `value` means not available from the fetched source; the `note` says why.

### Geographical area, forest cover, tree cover (ISFR 2023)

- Publisher: Forest Survey of India (MoEFCC). India State of Forest Report 2023, Volume I, released December 2024. Assessment year 2023, satellite imagery of 2021-22 (IRS Resourcesat-2 LISS-III, 1:50,000).
- URL: https://fsi.nic.in/uploads/isfr2023/isfr_book_eng-vol-1_2023.pdf (raw copy: `raw/isfr2023/isfr_book_eng-vol-1_2023.pdf`)
- Units: km2 and percent of geographical area (GA).
- Variables: `geographical_area_km2`, `forest_cover_km2`, `tree_cover_km2`, `forest_cover_pct_of_ga`, `forest_and_tree_cover_pct_of_ga`.
- Extraction: text of PDF page 84 (Table 2.4 "State/UT wise Forest Cover inside and outside Recorded Forest Area (RFA)/Green Wash (GW)") was read with pdfplumber. Each state line has 29 numbers. Column A = GA; column T (Total Forest Cover 2023 = forest cover inside plus outside RFA/GW, all density classes) = `forest_cover_km2`; column G (Tree Cover 2023) = `tree_cover_km2`; the last column (`Y*100/A`, forest plus tree cover as % of GA, as printed) = `forest_and_tree_cover_pct_of_ga`. `forest_cover_pct_of_ga` is derived as `T / A * 100` (it is not printed per state in Vol I). Names split over two lines in the PDF (e.g. "Arunachal / Pradesh") were joined.
- Jammu & Kashmir and Ladakh: Table 2.4 prints one GA (222,236 km2, the Census 2011 area of the unified state) for both UTs and footnotes that SoI has not supplied separate areas. PDF page 52 (Table 1.3, Recorded Forest Areas) prints "Shapefile Area" in brackets from the Survey of India shapefile (August 2021): J&K 54,633 km2 and Ladakh 168,055 km2. These shapefile areas are used as `geographical_area_km2` for the two UTs (note says so), and as the denominator of their derived `forest_cover_pct_of_ga`. The composite row carries GA 222,236 km2, combined forest cover 23,632.31 km2 (sum of the two UT rows) and 10.63%. The shapefile areas sum to 222,688 km2, which is 452 km2 more than 222,236; use either the composite or the shapefile split, not both.
- Check: the 34 other units plus 222,236 km2 sum exactly to the printed India GA of 3,287,468.88 km2; India forest cover 715,342.61 km2 = 21.76% of GA, matching Table 2.3 (p.78).
- Caveats: J&K `forest_and_tree_cover_pct_of_ga` (12.69) is as printed and its base area is not stated; Ladakh has no printed percentage. Lakshadweep GA in Table 2.4 is 29.63 km2 (Table 1.3 prints 30).

### Wasteland area (Wastelands Atlas of India 2019)

- Publisher: Department of Land Resources (Ministry of Rural Development) with NRSC/ISRO. Wastelands Atlas of India 2019, change analysis 2008-09 vs 2015-16 (IRS LISS-III three-season data, 1:50,000; 23 wasteland classes).
- Index page: https://dolr.gov.in/wasteland-atlas-of-india-2019/ (the redesigned DoLR site no longer lists it; the chapter links were read from the Internet Archive snapshot https://web.archive.org/web/20250225120417/https://dolr.gov.in/wasteland-atlas-of-india-2019/). Chapter PDFs are still served from the DoLR S3WaaS CDN:
  - State table (item 29, "Wastelands Map of India 2015-16, State-wise and Category-wise distribution of Wastelands during 2015-16 vis-a-vis 2008-09"): https://cdnbbsr.s3waas.gov.in/s3d69116f8b0140cdeb1f99a4d5096ffe4/uploads/2024/04/20240425368929714.pdf (raw: `raw/wasteland_atlas2019/WAI2019_29_statewise_categorywise_2015-16_vs_2008-09.pdf`)
  - Union Territories chapter (item 27): https://cdnbbsr.s3waas.gov.in/s3d69116f8b0140cdeb1f99a4d5096ffe4/uploads/2024/04/20240425546899163.pdf (raw: `raw/wasteland_atlas2019/WAI2019_27_union_territories.pdf`)
  - Also saved for reference: front matter/executive summary (item 1) and the J&K chapter (item 10).
- Units: km2; percent of the atlas's total geographical area (TGA).
- Variables: `wasteland_area_km2` (year 2008-09 and 2015-16), `wasteland_pct_of_tga` (both years), `wasteland_atlas_tga_km2`.
- Extraction: page 2 of the item-29 PDF (atlas page 11, "State wise total area under wastelands (sq.km) during 2015-16 vis-a-vis 2008-09") was parsed line by line: TGA, wasteland 2008-09, 2015-16, and % of TGA for both years were kept for the 30 state rows and the TOTAL row (mapped to `India`). The table's single "Union Territories" row (8 UTs, TGA 9,490 km2) was replaced by the district-wise totals on pages 6-7 of the UT chapter: the "Total" column of the 2008-09 and 2015-16 lines and the TGA column were read for each district; Dadra & Nagar Haveli + Daman + Diu were summed into `Dadra and Nagar Haveli and Daman and Diu`, and Karaikal + Mahe + Puducherry + Yanam into `Puducherry`; "--" (class absent) was read as 0. The script asserts the UT sums equal the printed Grand Total (309.95 / 306.23 km2, TGA 9,490). UT percentages are derived (`area / TGA * 100`). Delhi is in the state table.
- Caveats: Jammu & Kashmir is the undivided pre-2019 state (mapped to `Jammu and Kashmir (incl. Ladakh)`), and its totals include 120,849 km2 of unsurveyed area, most of it classed as snow/glacial or barren rocky; the atlas does not split J&K and Ladakh, so those two current units are blank. The atlas TGA differs slightly from ISFR GA for some states (e.g. Andhra Pradesh 162,989 vs 162,922.57). Map-sheet summary boxes in the UT chapter disagree with the district table for some UTs (e.g. Dadra & Nagar Haveli 52.33 vs 39.71 km2; the Puducherry map box repeats Delhi's figures); the district table was used. 2008-09 values are the atlas's revised figures. Wasteland includes classes (snow/glacial, dense scrub, degraded forest, shifting cultivation) that are not usable for solar or wind siting.

### Mean annual GHI (NASA POWER, derived)

- Publisher: NASA Langley Research Center, POWER project. Climatology API v2.10.1, community RE, parameter ALLSKY_SFC_SW_DWN (all-sky surface shortwave downward irradiance, i.e. GHI), source CERES SYN1deg, 1-degree grid, climatology period January 2001 to December 2025.
- URL pattern: https://power.larc.nasa.gov/api/temporal/climatology/regional?parameters=ALLSKY_SFC_SW_DWN&community=RE&longitude-min=..&longitude-max=..&latitude-min=..&latitude-max=..&format=JSON (12 tiles: longitude 68-78, 78-88, 88-98 by latitude 6-16, 16-26, 26-36, 30-38; raw JSON in `raw/nasa_power/`).
- Units: kWh/m2/day (annual mean, "ANN").
- Variable: `ghi_mean_kwh_m2_day`.
- Method: each grid value is treated as the mean of the 1 x 1 degree cell centred on it. For each state/UT polygon the state value is the mean of cell ANN values weighted by (intersection area of cell and polygon in deg2) x cos(cell-centre latitude). Polygons: geoBoundaries gbOpen IND ADM1 (36 units, boundaries from DataMeet India/Election Commission, CC BY 2.5 IN), https://github.com/wmgeolab/geoBoundaries/raw/9469f09/releaseData/gbOpen/IND/ADM1/geoBoundaries-IND-ADM1.geojson (raw: `raw/geoboundaries_ind_adm1/`; API metadata JSON saved alongside). Names were matched after removing diacritics.
- Caveats: 1-degree satellite product, so small UTs (Chandigarh, Delhi, Puducherry, DNH&DD, Lakshadweep, Goa) take the value of the surrounding cells, and values over high mountains (Ladakh, J&K, Himachal, Sikkim, Arunachal) are likely biased low by snow/cloud discrimination. Not an official Indian (NIWE/NISE) number. The DataMeet boundaries are not the official Survey of India boundaries.

### Wind power density at 100 m (Global Wind Atlas, derived)

- Publisher: Technical University of Denmark (DTU Wind Energy) and World Bank Group (ESMAP). Global Wind Atlas v4, country GeoTIFF for India, mean wind power density at 100 m, ~250 m grid (0.0025 deg), EPSG:4326, CC BY 4.0.
- URL: https://globalwindatlas.info/api/gis/country/IND/power-density/100 (served from https://gwa.cdn.nazkamapps.com/country_tifs_v4/IND_power-density_100m.tif; raw: `raw/global_wind_atlas/IND_power-density_100m.tif`, 220 MB).
- Units: W/m2.
- Variables: `wind_power_density_100m_mean_w_m2` (cos(latitude)-weighted mean of pixels whose centres lie inside the state polygon, via rasterio.mask) and `wind_power_density_100m_median_w_m2` (unweighted median). Same geoBoundaries polygons as for GHI.
- Coverage rule: the share of each polygon covered by valid raster pixels was computed (pixel area vs polygon area, equirectangular approximation). The GWA India raster ends at 33.28 N and follows a boundary that omits most of Arunachal Pradesh (about 20% covered), J&K (about 1%) and Ladakh (about 0%); values below 90% coverage were left blank. Uttarakhand (96%) and Himachal (99%) are kept.
- Caveats: this is the resource at 100 m hub height, not NIWE's assessed potential. Means in hilly states (Himachal, Sikkim, Kerala, Uttarakhand) are pulled up by ridge-top pixels; the median is the more representative figure there. No official state-level wind power density class table was found.

---

## 7. nuclear_sites.csv

Columns: `plant_site, location, state, unit_or_project, status, status_as_of, units, unit_capacity_mw, total_capacity_mw, reactor_type, developer, commercial_operation_date, source_document, source_url, note`. 40 rows: 24 operating units (one row per unit), RAPS-1 (extended shutdown), and 15 project/site rows.

- Operating units: NPCIL, "All Plants" page, https://www.npcil.nic.in/content/302_1_AllPlants.aspx (fetched November 2026; raw: `raw/npcil/302_1_AllPlants.html`). The HTML table "NPCIL Nuclear Power Plants Details and Installed Capacity" was parsed: plant location ("Tarapur, Maharashtra" split into site and state; "Tamilnadu" and "Uttar pradesh" normalised), unit, reactor type (BWR/PHWR/PWR), capacity (MWe), date of commercial operation (converted to ISO dates). Total 24 units, 8,780 MW, which matches the page's printed total.
- R&M flag and RAPS-1: Department of Atomic Energy, reply to Lok Sabha Starred Question No. 251 answered 17.12.2025, "Nuclear power plants and projects in the country", https://cdnbbsr.s3waas.gov.in/s35b8e4fd39d9786228649a8a8bec4e008/uploads/2026/04/202604201352695736.pdf (raw: `raw/dae_parliament/LS_SQ251_17122025_nuclear_plants_projects.pdf`). Annexure-I marks TAPS-1, TAPS-2, MAPS-1 and KGS-1 with "$" (in project mode for renovation and modernisation/refurbishment, 760 MW) and RAPS-1 (100 MW) with "*" (extended shutdown, excluded from 8,780 MW).
- Under construction/commissioning and sanctioned projects: same reply, Annexure-II (eight reactors, 6,600 MW under construction/commissioning; ten reactors, 7,000 MW under pre-project activities). Reactor type and developer come from the NPCIL project pages: RAPP-7&8 https://www.npcil.nic.in/writereaddata/CMS/202405141058097640439RAPP_7_8_english.pdf, KKNPP-3&4 .../202405141058485326841KKNPP_3_4_english.pdf, KKNPP-5&6 .../202405141059343507842KKNPP_5_6_english.pdf, GHAVP-1&2 .../202405141048396829946GHAVP_1_2_english.pdf, Kaiga-5&6 .../202603170449172769922Kaiga_5_6_English.pdf, Fleet Mode Reactors .../202405141100203976607Fleet_Mode_Reactors_english.pdf (raw: `raw/npcil/`). Kaiga-5&6 is shown as "under construction" because the NPCIL page (March 2026) reports first pour of concrete on 1 March 2026; the December 2025 reply listed it under pre-project activities (both noted). Mahi Banswara 1-4 are being implemented by ASHVINI (NPCIL 51% / NTPC 49% JV). PFBR type FBR is taken from its name; its developer is not named in the cited sources.
- Sites accorded in-principle approval: DAE reply to Rajya Sabha Unstarred Question No. 3044 answered 27.03.2025, https://cdnbbsr.s3waas.gov.in/s35b8e4fd39d9786228649a8a8bec4e008/uploads/2025/04/202504011203905396.pdf (raw: `raw/dae_parliament/RS_DAE_answer_2025-04_upload_202504011203905396.pdf`), Annexure: Kovvada ("Kovavada" in source) 6 x 1208 MW (Andhra Pradesh), Jaitapur 6 x 1730 MW (Maharashtra), Mithi Virdi 2 x 1000 MW (Gujarat), Bhimpur 4 x 700 MW (Madhya Pradesh), Haripur 6 x 1000 MW (West Bengal). Reactor types are not stated there and are left blank. Older replies give different unit sizes for Jaitapur and Kovvada; the 2025 figures were used.
- Build check: every project row carries a pattern (e.g. "KKNPP-3&4 2 X 1000") that the script requires to match the text of the cited PDF, and reactor types are checked against the NPCIL project PDF text.
- Totals by status: operating 8,780 MW (24 units, of which 760 MW in R&M mode); extended shutdown 100 MW; under construction/commissioning 6,600 MW plus Kaiga-5&6 1,400 MW; sanctioned pre-project 5,600 MW; in-principle sites 28,428 MW.
- Caveats: capacities are gross MWe as published. Small modular reactors, Bharat Small Reactors and private-sector plans under the Nuclear Energy Mission have no sites yet and are not listed.

### 100 GW nuclear by 2047 target (recorded in policy_targets.csv)

- Official announcement: Union Budget 2025-26, Budget Speech of the Finance Minister, 1 February 2025, Part A, paragraph 61 under "Nuclear Energy Mission for Viksit Bharat" (PDF page 15). Exact sentence: "Development of at least 100 GW of nuclear energy by 2047 is essential for our energy transition efforts." Paragraph 62 sets up a Nuclear Energy Mission for small modular reactors (outlay Rs 20,000 crore; at least 5 indigenously developed SMRs operational by 2033).
- URL: https://www.indiabudget.gov.in/budget2025-26/doc/budget_speech.pdf (raw: `raw/budget2025_26/budget_speech_2025_26.pdf`). Publisher: Ministry of Finance, Government of India.
- Supporting: the DAE reply to Lok Sabha Starred Q.251 (17.12.2025) states that the Nuclear Energy Mission envisages 100 GW by 2047 from 8.78 GW installed (excluding RAPS-1), with DAE PSUs expected to contribute about 58-60 GW; the DAE reply to Lok Sabha Unstarred Q.558 (23.07.2025), https://cdnbbsr.s3waas.gov.in/s35b8e4fd39d9786228649a8a8bec4e008/uploads/2025/08/202508111697044173.pdf (raw: `raw/dae_parliament/LS_USQ558_23072025_nuclear_roadmap.pdf`), projects 22,480 MW by 2031-32 on completion of the projects under construction and sanctioned.

---

## 8. policy_targets.csv

Columns: `category, item, scope, fiscal_year, target_year, value, unit, source_document, source_location, source_url, quote, note`.

`category` is either "national target" (9 rows) or "NEP 2023 projection" (138 rows). These rows are for justification and benchmarking only.

### National targets

Each `quote` was checked by machine against the text of the archived source file. The build stops if a quote is missing.

| item | value | source |
|---|---|---|
| Non-fossil electricity capacity by 2030 | 500 GW | National Statement by the PM at COP26, Glasgow, 1 Nov 2021, https://www.pmindia.gov.in/en/news_updates/national-statement-by-pm-at-cop26-summit-in-glasgow/ (raw: `raw/policy/pmindia_COP26_national_statement.html`); restated in PIB (MoEFCC) release of 3 Feb 2022, https://pib.gov.in/PressReleasePage.aspx?PRID=1795071 (raw: `raw/policy/PIB_PRID_1795071.html`) |
| Net zero | by 2070 | same two sources |
| 50% of energy requirements from renewable energy by 2030; 1 billion tonnes reduction in projected emissions to 2030 | 50%; 1 bn t | COP26 national statement (Panchamrit items 2 and 3) |
| NDC: about 50% cumulative electric power installed capacity from non-fossil sources by 2030 (with technology transfer and low-cost international finance) | 50% | India's Updated First NDC under the Paris Agreement (2021-2030), August 2022 submission to UNFCCC, item 4, page 2 of 3, https://unfccc.int/sites/default/files/NDC/2022-08/India%20Updated%20First%20Nationally%20Determined%20Contrib.pdf (raw: `raw/policy/India_Updated_First_NDC_Aug2022.pdf`) |
| NDC: emissions intensity of GDP 45% below 2005 by 2030 | 45% | same NDC, item 3 |
| Nuclear capacity by 2047 | at least 100 GW | Union Budget 2025-26, Budget Speech, 1 Feb 2025, Part A para 61, PDF page 15: "Development of at least 100 GW of nuclear energy by 2047 is essential for our energy transition efforts." https://www.indiabudget.gov.in/budget2025-26/doc/budget_speech.pdf (raw: `raw/budget2025_26/budget_speech_2025_26.pdf`). Para 62 sets up the Nuclear Energy Mission for small modular reactors (Rs 20,000 crore; at least 5 operational by 2033). |

Notes on the targets:
- The COP26 text is the PMO English rendering of a Hindi speech. The 45% intensity pledge reads "less than 45 percent" in that rendering, so the NDC wording is used for that item.
- No NDC submitted after August 2022 was checked.

### NEP 2022-32 benchmarks

- **Document:** CEA, National Electricity Plan (Volume I, Generation) 2022-32, Gazette 18.05.2023. URL and archive copy as in section 1.
- **Pages were read with pdfplumber and parsed by regular expressions:**

  | Rows | Table | PDF page (printed page) | Unit |
  |---|---|---|---|
  | All-India consumption, T&D losses (MU, %), energy requirement (ex-bus), annual load factor, peak demand, for 2021-22, 2026-27 and 2031-32 | Table 4.1 | 122 (4.2) | as printed |
  | Region-wise energy requirement (MU) and peak demand (MW) | Table 4.2 | 123 (4.3) | MU, MW |
  | Source-wise likely gross generation, 2026-27 | Table 5.11 | 147 (5.21) | BU |
  | Source-wise likely gross generation, 2031-32 | Table 5.12 | 148 (5.22) | BU |
  | Region-wise likely installed capacity by resource, end 2026-27 | Table 5.7 | 138 (5.12) | MW |
  | All-India projected installed capacity, end 2031-32 | Table 5.8 | 139 (5.13) | MW |
  | BESS 2026-27 (8,680 MW / 34,720 MWh) | Major Highlights item x | 63 | MW, MWh |
  | Non-fossil share of installed capacity (57.4% and 68.4%) | Major Highlights item xxvii | 64 | percent |

  In Table 5.7, a printed "-" is recorded as 0, with a note.
- **Checks:**
  - Generation components sum exactly to the printed totals: 2,025.0 BU and 2,665.7 BU.
  - Table 5.7 resource rows sum to the printed region totals.
- **Headline figures:**
  - Energy requirement: 1,907,835 MU (2026-27) and 2,473,776 MU (2031-32). Peak demand: 277,201 MW and 366,393 MW.
  - Gross generation 2031-32: 2,665.7 BU. This is coal + lignite 1,334.8; PV 665.6; wind 258.1; hydro 246.2 (including imports); nuclear 117.6; gas 33.4; other RE 10.0.
  - Installed capacity 2031-32: 900,422 MW, plus BESS of 47,244 MW / 236,220 MWh.
- **Caveats:**
  - The demand is the 20th Electric Power Survey projection for utilities only. Demand met by captive plants is excluded; Table 4.5 projects captive self-consumption of 368,549 MU in 2026-27 and 601,660 MU in 2031-32, and these are not in the CSV.
  - Per page 5.7 of the NEP, the demand includes energy efficiency, EV and green hydrogen impacts but excludes rooftop solar. Rooftop generation of 17.6 BU (2026-27) and 55.8 BU (2031-32) was added back in the planning study.
  - Major Highlights item iv prints 2031-32 energy requirement as "2473.7 BU", while Tables 4.1 and 5.2 give 24,73,776 MU (2,473.8 BU). The table value is stored.
  - Gross generation (2,665.7 BU) is measured at generator terminals, while the requirement (2,473.8 BU) is ex-bus. The planning study also adds rooftop solar. The two figures are therefore not directly comparable.

---

## 9. nep2023_state_demand.csv (supplementary benchmark)

Columns: `state, state_as_reported, grid_region, row_type, fiscal_year, energy_requirement_mu, energy_requirement_bu, peak_demand_mw, source, source_url, note`.

- **Source:** NEP 2022-32 (as in section 1):
  - Annexure 4.1 "State/UT wise Electrical Energy Requirement (MU) at Power Station Bus Bar (Utilities)", PDF page 125.
  - Annexure 4.2 "State/UT wise Peak Electricity Demand (in MW) at Power Station Bus Bar (Utilities)", PDF page 126.
  - Both from the 20th EPS, for 2021-22 (base), 2026-27 and 2031-32.
- **Extraction:** text lines of the form "name, then three numbers" were parsed. Indian digit grouping (e.g. 1,14,636) was removed. `energy_requirement_bu` = MU / 1000.
- **`row_type` values:**
  - `state`: one row per canonical unit.
  - `component`: Dadra Nagar Haveli and Daman & Diu as printed.
  - `utility`: DVC.
  - `region_total` and `national_total`.
- **Merged UT:** an aggregated `state` row for Dadra and Nagar Haveli and Daman and Diu sums the two energy values. Its peak is left blank, because state peaks are non-coincident.
- **Checks:** state energy values sum to each printed regional total. State peaks do not sum to regional peaks (for example, Northern 2031-32: sum of state peaks 138,216 MW against a regional peak of 127,553 MW), as expected for non-coincident peaks.

---

## 10. new_loads.csv (secondary; a separate scenario, never in the headline)

Columns: `load, year, twh_low, twh_central, twh_high, basis, overlaps_history, source_name, source_url, source_location, notes`. Five rows, one per load.

**Purpose.** The midpoint panel asked which factors besides income drive the 2047 requirement. This file records published estimates of electricity for loads that the historical generation trend may not capture. It is read only as a labelled scenario outside the headline. Every source here is **secondary**; none is NPP or the Renewable India dataset.

**Conventions**
- **Units and measure.** `twh_low`, `twh_central` and `twh_high` are TWh per year in `year`. They are end-use electricity for that load, as the sources state it. They are not grossed up for T&D losses or auxiliary use, so they are not on the project's generation basis. `basis` says where a source states generation instead (NITI's CPS hydrogen figure).
- **Totals, not increments.** Each value is the load's total in that year. For loads already present today, `notes` gives the source's base-year level where it is printed.
- **Year.** Every row is 2050, and `basis` says so. No source gives a 2047 value across the range for any load, and nothing was interpolated to 2047. The two 2047 figures found are kept in `notes` for context: Climate Policy Initiative's FY2047 electric-vehicle upper bound (1,070 TWh) and the NITI Overview's data-centre figure labelled 2047 (394 TWh).
- **Low, central and high.** Low is the lowest published figure for that year and scope, and high is the highest. Central is the median when there are three figures. With two figures, central is the Indian government study (NITI Aayog) where one exists, otherwise the source's baseline scenario. No value was invented or averaged.
- **Several sources in one row.** In `source_name`, `source_url` and `source_location`, parts are separated by ` | ` in the same order. Each part of `source_name` and `source_location` is tagged with the role it supplies (`[low]`, `[central]`, `[high]`).
- **`overlaps_history`.** `no` means the load is absent from the historical generation series. `partial` means it is already in the series and part of its growth sits inside the income elasticity. `yes` is not used. Each row's `notes` justifies its flag.
- **Hydrogen units.** All three hydrogen values are published in TWh, so no Mt-to-TWh conversion was made. As a check, the implied electricity per kilogram is recorded in `notes`: 54 to 59 kWh/kg for NITI and 45 kWh/kg for TERI.

**Sources** (raw copies in `raw/new_loads/`; PDFs and HTML are not committed)

| source | publisher and date | URL | raw file | used for |
|---|---|---|---|---|
| Scenarios Towards Viksit Bharat and Net Zero: An Overview (Vol. 1) | NITI Aayog, Feb 2026 | https://niti.gov.in/sites/default/files/2026-02/Scenarios-Towards-Viksit-Bharat-and-Net-Zero-%20An-Overview-Vol1.pdf | `NITI_2026_Scenarios_Viksit_Bharat_Net_Zero_Overview_Vol1.pdf` | green hydrogen low and high; context |
| Scenarios Towards Viksit Bharat and Net Zero, Sectoral Insights: Power (Vol. 7) | NITI Aayog, Feb 2026 | https://niti.gov.in/sites/default/files/2026-02/Scenarios-Towards-Viksit-Bharat-and-Net-Zero-Sectoral-Insights-Power.pdf | `NITI_2026_Sectoral_Insights_Power_Vol7.pdf` | data centres low and central; context |
| Scenarios Towards Viksit Bharat and Net Zero, Sectoral Insights: Buildings (Vol. 5) | NITI Aayog, Feb 2026 | https://niti.gov.in/sites/default/files/2026-02/Scenarios-Towards-Viksit-Bharat-and-Net-Zero-Sectoral-Insights-Buildings.pdf | `NITI_2026_Sectoral_Insights_Buildings_Vol5.pdf` | data-centre derivation; cooling context |
| Scenarios Towards Viksit Bharat and Net Zero, Sectoral Insights: Transport (Vol. 3) | NITI Aayog, Feb 2026 | https://niti.gov.in/sites/default/files/2026-02/Scenarios-Towards-Viksit-Bharat-and-Net-Zero-Sectoral-Insights-Transport.pdf | `NITI_2026_Sectoral_Insights_Transport_Vol3.pdf` | transport context |
| India's Electricity Transition Pathways to 2050: Scenarios and Insights (Rodrigues et al.) | TERI, Feb 2024 (the report's suggested citation says 2023) | https://teriin.org/sites/default/files/2024-02/Power_Sector_2050_0.pdf | `TERI_2024_Power_Sector_2050.pdf` | EV central and high; hydrogen central; cooling low and central; electric cooking |
| The Future of Cooling: Opportunities for energy-efficient air conditioning | IEA, May 2018 | https://iea.blob.core.windows.net/assets/0bb45525-277f-4c9c-8d0c-9c0cb5e7d525/The_Future_of_Cooling.pdf | `IEA_2018_The_Future_of_Cooling.pdf` | cooling high |
| India Transition Factbook 2026 | BloombergNEF, 13 Oct 2026 | https://assets.bbhub.io/professional/sites/44/BNEF-India-Transition-Factbook-2026.pdf | `BNEF_India_Transition_Factbook_2026.pdf` | data centres high; context |
| What Is Fuelling India's Road Transport Sector? Projecting District-Level Energy Demand | CEEW, June 2025 | https://www.ceew.in/sites/default/files/how-will-fuel-demand-grow-with-changing-fuel-mix-in-different-vehicle-segments.pdf | `CEEW_2025_Road_Transport_Energy_Demand.pdf` | EV low |
| Driving Jobs, Economic Growth, and Climate Action: The Role of Clean Mobility in India | Climate Policy Initiative, March 2026 | https://www.climatepolicyinitiative.org/wp-content/uploads/2026/03/Economic-Implications_Report.pdf | `CPI_2026_Clean_Mobility_Economic_Implications.pdf` | EV context (FY2047) |
| India data centre capacity to reach 12 GW by 2030 (press release) | Wood Mackenzie, 27 July 2026 | https://www.woodmac.com/press-releases/india-data-centre-capacity-to-reach-12-gw-by-2030/ | `WoodMackenzie_press_release_2026-07-27.html` | data-centre context (2040) |

**Values (TWh in 2050)**

| load | low | central | high | overlaps_history | where |
|---|---|---|---|---|---|
| electric_vehicles | 420 (CEEW main projection) | 456 (TERI Baseline) | 773 (TERI Low Carbon) | partial | CEEW p. 41 and Figure 23, p. 42; TERI Table 13, p. 40 |
| green_hydrogen | 470 (NITI CPS) | 896 (TERI Low Carbon, 20 Mt) | 1,392 (NITI NZS) | no | NITI Overview p. 55; TERI p. 41 |
| data_centres | 400 (NITI) | 400 (NITI) | 636 (BNEF ETS) | partial | NITI Power p. 41; BNEF p. 35 |
| space_cooling | 582 (TERI Low carbon, residential) | 784 (TERI Baseline, residential) | 1,350 (IEA Baseline, all buildings) | partial | TERI Table 9, p. 28; IEA p. 61 and Figure 3.6 |
| electric_cooking | 132 (TERI Baseline) | 132 (TERI Baseline) | 152 (TERI Low Carbon) | partial | TERI p. 28 |

Page numbers are printed pages, with PDF pages in the CSV's `source_location`.

**Extraction and checks**
- Text was read from each PDF with `pdftotext -layout`, and every value was located by its printed and PDF page.
- TERI Table 9 prints its row labels out of line in the extracted text. Baseline is 784 TWh and Low carbon 582 TWh, confirmed by the printed residential totals (1,483 and 1,239 TWh) and cooling shares (53% and 47%) on the same page.
- TERI Table 13: road plus rail equals the printed transport total in both scenarios (456 + 72 = 528; 773 + 72 = 845 TWh).
- NITI data centres: 45 GW of average load times 8,760 hours is 394 TWh, which the Power volume rounds to about 400 TWh.
- Read but not used: the LBNL CLEAN-India paper (2023) shows its load split only in a figure; the NITI Aayog and RMI hydrogen report (2022) gives hydrogen in Mt, not electricity; TERI's room-AC study (2022) gives 2047 room-AC electricity only in figures; Barbar et al. (Scientific Data, 2021) publishes its cooling and EV results as a dataset, not in the text.

**Caveats**
- **Consumption, not generation.** To compare with the generation forecast, each value needs grossing up for T&D losses and auxiliary use. This is not done here.
- **Captive and off-grid supply.** TERI assumes hydrogen electrolysers run on off-grid renewables and leaves that electricity out of grid demand (p. 41). Data-centre operators also use onsite captive and offsite group-captive plants (BNEF, p. 34). Such supply would not appear in NPP utility generation, which is the only generation the organisers' ruling allows (D12).
- **Double counting.** For `partial` loads the totals include today's level and trend growth that the income elasticity already carries. Adding a `partial` total on top of the headline counts that part twice.
- **Scope.** The space-cooling low and central are residential only, while the high covers all buildings and dates from 2018. The EV row is road transport only. NITI's transport figure (about 550 TWh in 2050) includes rail and metro and is kept only as context.
- **Inconsistencies inside NITI.** The Overview dates the 45 GW (394 TWh) data-centre load to 2047 (p. 97), while the Buildings and Power volumes date it to 2050. The Buildings executive summary (p. xxii) prints residential cooling of 129 TWh (2020) and 915 and 604 TWh (2070), while its chapter (p. 51) prints 123, 834 and 596 TWh. The CSV uses the 2050 dating and the chapter values.

---

## Coverage and gaps (all files)

- **Population:**
  - State projections end in 2036. Only national projections run to 2050 (UN WPP).
  - No official state projection to 2047 was found.
- **Urbanisation:**
  - The national series to 2050 uses WUP 2018 (national definitions; archived copy) and WUP 2025 (degree of urbanisation; a different concept).
  - The annual WUP 2018 file could not be retrieved; it was rate-limited by the archive.
- **Economy:**
  - No constant-price GSDP for Ladakh, Lakshadweep or DNH&DD.
  - Latest-year gaps as listed in section 4.
  - National real GDP and state GSDP are on different base years.
- **Resource potential** (section 5):
  - No small hydro site counts and no waste-to-energy potential.
  - Pumped storage covers only the 21 states CEA lists.
  - Wind at 120 m is available only with J&K and Ladakh combined.
- **Climate and land** (section 6):
  - Wasteland is not split between J&K and Ladakh.
  - Wind power density is blank for Arunachal Pradesh, J&K and Ladakh.
  - GHI and wind density are derived from global gridded products (NASA POWER, Global Wind Atlas), not from an official Indian state table, which was not found.
- **Nuclear** (section 7): reactor types for the in-principle sites are not stated in the sources.
- **New loads** (section 10): no source gives a 2047 value across the range, so every row is 2050. Data centres rest on two sources and electric cooking on one.
- **Census 2011 direct tables:** censusindia.gov.in timed out. Census 2011 values are taken as reproduced in the Technical Group report (population and urban share), and areas are taken from ISFR 2023.
