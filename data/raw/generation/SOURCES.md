# India electricity generation data: sources and extraction notes

All files in this folder hold electricity **generation** (energy, GWh). No installed capacity, demand,
consumption or net-import series are included in the tidy outputs, even where the raw downloads contain them.

Access date for every download: **2026-11-15**.

## Folder layout

| Path | Content |
| --- | --- |
| `downloads/owid/` | Raw Our World in Data files (untouched) |
| `downloads/ember/` | Raw Ember files (untouched) |
| `downloads/cea/` | Raw Central Electricity Authority PDFs (renamed only, see table below) |
| `national/` | All-India series |
| `region/` | Grid-region series (Northern, Western, Southern, Eastern, North-Eastern) |
| `state/` | State / UT series, plus `state_region_map.csv` |

## Common columns of the tidy CSVs

| Column | Meaning |
| --- | --- |
| `year` | Calendar year (Jan-Dec). Only in `*_cy.csv` files. |
| `fiscal_year` | Indian fiscal year April-March, written `2023-24`. Only in `*_fy.csv` files. |
| `fy_end_year` | Calendar year in which the fiscal year ends (`2023-24` -> 2024). |
| `month` | `YYYY-MM`. Only in the monthly file. |
| `region` | `Northern`, `Western`, `Southern`, `Eastern`, `North-Eastern`, or `All India`. |
| `state` | State / UT name, `Central Sector (unallocated)` (CEA only, see caveats), or `ALL` for a region-level or national aggregate row. |
| `row_type` | CEA files only: `state`, `central_sector`, `region_total`, `central_sector_total`, `india_total`. |
| `source` | Canonical source name (see mapping below). |
| `source_detail` | Category label exactly as used by the original publisher. |
| `generation_gwh` | Gross generation in GWh (1 GWh = 1 million units, MU). |
| `is_subtotal` | `True` if the row is a sum of other rows in the same file (e.g. `total`, `thermal_total`, region or national aggregates). Sum only rows with `is_subtotal == False` to avoid double counting. |
| `scope` | `utilities`, `captive`, or a note on coverage (see each file). |
| `source_id` | Identifier of the publication and table the number was read from. |
| `note` | Row-level caveat, if any. |

### Canonical source names

`coal, lignite, gas, oil_diesel, nuclear, hydro_large, hydro_small, solar, wind, biomass, other` are used where
the publisher's split allows. Four extra labels are used where it does not, and are documented here:

* `hydro` = all hydro, large and small together (Ember, OWID, and CEA captive tables do not separate them).
* `res_total` = CEA "RES" (renewable energy sources other than large hydro: wind, solar, biomass, bagasse,
  small hydro <= 25 MW, waste to energy). It is a leaf row where the publisher gives no split, and a subtotal
  (`is_subtotal == True`) where the split rows are present.
* `thermal_total` = CEA total thermal (steam + diesel + gas). A leaf row only for FY2007-08 to FY2015-16
  (except FY2011-12) in `cea_national_generation_fy.csv`, where CEA prints no split.
* `total` = total generation of the row's scope (always `is_subtotal == True`).

`lignite` is never populated: none of the sources separate lignite from coal. CEA "Steam" and "Coal/Lignite"
and Ember "Coal" all include lignite and are mapped to `coal`.

Mapping by publisher:

| Publisher label | Canonical |
| --- | --- |
| CEA Hydro (utilities) | `hydro_large` (stations above 25 MW; small hydro is inside RES) |
| CEA Steam / Coal/Lignite | `coal` |
| CEA Diesel | `oil_diesel` |
| CEA Gas | `gas` |
| CEA Nuclear | `nuclear` |
| CEA RES | `res_total` |
| CEA Small Hydro | `hydro_small` |
| CEA Wind / Solar | `wind` / `solar` |
| CEA Biomass, Bagasse | `biomass` (two rows, distinguished by `source_detail`) |
| CEA Waste to Energy | `other` |
| CEA captive Hydro | `hydro` |
| CEA captive Railways, and captive totals with no breakup | `other` |
| Ember Coal / Gas / Nuclear / Solar / Wind | same names |
| Ember Other fossil (oil and other fossil) | `oil_diesel` |
| Ember Hydro | `hydro` |
| Ember Bioenergy | `biomass` |
| Ember Other renewables | `other` |
| OWID coal / gas / oil / nuclear / hydro / solar / wind / biofuel / other_renewable_exc_biofuel / electricity_generation | `coal / gas / oil_diesel / nuclear / hydro / solar / wind / biomass / other / total` |

---

## Raw downloads

| Local file | URL | Bytes | SHA-256 (first 16 hex) |
| --- | --- | --- | --- |
| `downloads/owid/owid-energy-data.csv` | https://raw.githubusercontent.com/owid/energy-data/master/owid-energy-data.csv | 9229369 | 266f2e2baad79753 |
| `downloads/owid/owid-energy-codebook.csv` | https://raw.githubusercontent.com/owid/energy-data/master/owid-energy-codebook.csv | 45793 | 3cc9b7db0d921496 |
| `downloads/ember/release_generation_yearly_global.csv` | https://files.ember-energy.org/public-downloads/generation/outputs/release_generation_yearly_global.csv (linked from https://ember-energy.org/data/yearly-electricity-data/) | 16120810 | 08932763076fa781 |
| `downloads/ember/india_yearly_full_release_long_format.csv` | https://files.ember-energy.org/public-downloads/india_yearly_full_release_long_format.csv (linked from https://ember-energy.org/data/india-electricity-data/) | 1601047 | 747c3c05d76a45b6 |
| `downloads/ember/india_monthly_full_release_long_format.csv` | https://files.ember-energy.org/public-downloads/india_monthly_full_release_long_format.csv | 23472555 | 5874ab042dcf9f34 |
| `downloads/ember/ember_subnational_data_methodology.pdf` | https://files.ember-energy.org/public-downloads/ember_subnational_data_methodology.pdf | 441192 | 0d5c13d7c9a8656a |
| `downloads/cea/cea_gr2020_fy2018-19.pdf` | https://cea.nic.in/api/dashboardReports/download-pdf/en/ec9312d2-5755-41e0-8823-301d06ef6b19 | 12579375 | 36b6bf2763a902b5 |
| `downloads/cea/cea_gr2021_fy2019-20.pdf` | https://cea.nic.in/api/dashboardReports/download-pdf/en/68bbc2f7-f338-4af3-a1a9-a6b4a04bf09a | 5217296 | 71d412b6490cf5e7 |
| `downloads/cea/cea_gr2022_fy2020-21.pdf` | https://cea.nic.in/api/dashboardReports/download-pdf/en/613eff40-43fb-4a8b-a0c6-efed3d1bc698 | 33522133 | de1b977197da8698 |
| `downloads/cea/cea_gr2023_fy2021-22.pdf` | https://cea.nic.in/api/dashboardReports/download-pdf/en/8249480b-e957-4509-8cd6-f795316f0c69 | 14743380 | 42a580c456740485 |
| `downloads/cea/cea_gr2024_fy2022-23.pdf` | https://cea.nic.in/api/dashboardReports/download-pdf/en/b95c9bef-0b40-43e4-b0a5-5c8c67dea3fa | 18845268 | c3b0c58a741f5775 |
| `downloads/cea/cea_gr2025_fy2023-24.pdf` | https://cea.nic.in/api/dashboardReports/download-pdf/en/aec4372e-3df2-4782-9887-b75d10407572 | 12151904 | f710de8ee99efb4a |
| `downloads/cea/cea_growth_of_electricity_sector_1947-2025.pdf` | https://cea.nic.in/api/dashboardReports/download-pdf/en/e15a21a7-2864-4611-b0f8-75956818ea68 | 7567010 | 2cd402537db758d4 |

CEA General Review ("All India Electricity Statistics, General Review") PDFs are listed at
https://cea.nic.in/reports/dashboardView?headingId=107&subHeadingId=1001&subSubHeadingId=10084&title=General%20Review&lang=en .
That page labels them with years 2019 to 2024, but the cover of each PDF shows edition year = label + 1, and
each edition reports the fiscal year two years before its edition year (General Review 2025 covers FY2023-24).
Local files were renamed to `cea_gr<edition>_fy<fiscal year>.pdf`. The Growth book is listed at
https://cea.nic.in/divisionPage/508?lang=en ("Growth of Electricity Sector in India (1947-2025)").

### Licences

* OWID: README states "All visualizations, data, and code produced by Our World in Data are completely open
  access under the Creative Commons BY license"; third-party data keep the original licence (here Ember and
  the Energy Institute Statistical Review).
* Ember: site footer states "All content is released under a Creative Commons Attribution Licence (CC-BY-4.0)."
* CEA: no licence is stated in the PDFs or on the download pages (Government of India publications).

---

## Output files

### national/owid_national_generation_cy.csv  (434 rows)

* Source: OWID energy-data, `owid-energy-data.csv`. Units in source: TWh.
* Coverage: India, calendar years 1965-2025. Hydro, nuclear, solar and wind from 1965; coal, gas, oil and
  total from 1985; biomass (`biofuel_electricity`) from 2000. Before 1985 there is no total.
* Extraction: rows with `country == "India"`; columns `coal_electricity, gas_electricity, oil_electricity,
  nuclear_electricity, hydro_electricity, solar_electricity, wind_electricity, biofuel_electricity,
  other_renewable_exc_biofuel_electricity, electricity_generation`; melted to long form; blank cells dropped;
  TWh x 1000 = GWh. `electricity_demand`, `net_elec_imports` and all capacity or per-capita columns were not used.
* Caveats: OWID's codebook cites Ember (2026 release) and the Energy Institute Statistical Review (2025). For
  2020-2025 OWID's solar and total differ slightly from the current Ember file (an earlier Ember vintage).
  Scope matches Ember national (includes captive estimates).

### national/ember_national_generation_cy.csv  (369 rows)

* Source: Ember yearly electricity data (global), `release_generation_yearly_global.csv`. Units: TWh.
* Coverage: India, calendar years 1985-2025.
* Extraction: rows with `Area == "India"` and `Electricity source` in Coal, Gas, Other fossil, Nuclear, Hydro,
  Solar, Wind, Bioenergy, Total generation; column `Generation (TWh)` x 1000 = GWh. Aggregate sources (Clean,
  Fossil, Renewables, Wind and solar, "Hydro, bioenergy and other renewables") and the non-generation rows
  `Demand` and `Net imports` were dropped.
* Caveats: calendar year. Ember's subnational methodology note says the national yearly series "includes
  estimation for captive generation", so it is larger than CEA utility generation (CY2024: 2,035,115 GWh here vs
  1,804,424 GWh as the sum of Ember's state series). Hydro includes small hydro. "Other fossil" (mapped to
  `oil_diesel`) covers oil products and other fossil fuels. Values for 1985-1999 match OWID (same upstream).

### national/cea_national_generation_fy.csv  (501 rows)

All-India fiscal-year series from CEA, two scopes: `utilities` and `captive`. Units in source: GWh.

Utilities (`scope == utilities`), fiscal years 1947, 1950, 1955-56, 1960-61, 1965-66, 1968-69, 1973-74,
1978-79, 1979-80, 1984-85, 1989-90, 1991-92, 1996-97, 2001-02, then every year 2006-07 to 2024-25:

1. Growth book (`cea_growth_of_electricity_sector_1947-2025.pdf`), **Table 3** "Gross Electricity Generation in
   India - Mode-wise (Utilities)", PDF page 29. Columns Coal/Lignite, Gas, Diesel, Thermal Total, Hydro,
   Nuclear, RES, Total, one row per printed fiscal year (25 rows, 1947 to 2024-25). Indian digit grouping
   (`1,23,456`) removed. `source_id = cea_growth_1947_2025_table_3`.
2. General Review 2025, **Table 1.3** "Growth of Gross Electricity Generation - Mode-wise, Utilities Only",
   PDF page 28: columns Hydro, Thermal (steam, gas and diesel together), Nuclear, RES, Total. Used only for the
   fiscal years missing from Table 3 (2007-08 to 2010-11 and 2012-13 to 2015-16); here thermal is a single
   `thermal_total` leaf. `source_id = cea_general_review_2025_table_1.3`.
3. Growth book **Chart 15B** "Electricity Generation from R.E.S. - Mode-wise/Year-wise", PDF page 39: the data
   table under the chart gives Small Hydro, Wind, Biomass, Waste to Energy, Solar (MU = GWh) for FY2014-15 to
   FY2024-25. Added as split rows; for those years `res_total` is marked as a subtotal. The split matches the RES
   column of Table 3 / Table 1.3 within 5 GWh (Table 3 prints whole GWh). `source_id = cea_growth_1947_2025_chart_15b`.

Captive (`scope == captive`), plants in industries with demand of 1 MW and above up to FY2020-21 and 0.5 MW and
above from FY2021-22 (series break, stated in the source):

4. General Review 2025, **Table 1.6** "Growth of Electricity Generation From Captive Power Plants...", PDF page 31.
   36 printed rows from 1950 to 2023-24. Columns Hydro, Steam, Diesel, Gas, RES, Sub-total, Railways, Total
   from 1978-79 on (annual from 2000-01); for 1950 to 1973-74 only the total is printed (stored as `other` =
   "breakup not available", plus `total`). `NA` cells (Railways from 2000-01) skipped.
   `source_id = cea_general_review_2025_table_1.6`.
5. Growth book **Table 7** (PDF page 64) only for rows absent from Table 1.6: 1947 (total only) and 2024-25
   (all columns marked "@ Estimated" in the source; `note` says so). `source_id = cea_growth_1947_2025_table_7`.

Caveats:
* Fiscal year April-March. The 1947 and 1950 rows are labelled as printed; the Growth book says capacity for
  those two years is "as on 31st December", so these two generation rows may not be April-March years.
* CEA note: RES generation up to 2013-14 is normative, actual from 2014-15.
* For 1950 to 1968-69 the printed Total (and Table 1.3 Thermal) leaves out gas and diesel, so components exceed
  the Total by 200-393 GWh; affected rows carry a `note`. Values are kept as printed.
* Captive 2002-03: the printed components sum to 1,000 GWh more than the printed sub-total/total (RES is printed
  as 1176.43 vs 155-188 in neighbouring years); kept as printed, flagged in `note`.
* Small differences between editions: FY2022-23 total is 1,617,905 (Growth Table 3), 1,617,903 (GR2025 Table 1.3)
  and 1,617,902.96 (GR2024 Table 4.4). FY2024-25 values come from the newest publication and may be revised.
* Gross generation (includes auxiliary consumption). Imports (e.g. from Bhutan) are not generation in India and
  are not part of any CEA generation table used here.

### state/cea_state_generation_fy_utilities.csv  (2,336 rows)

* Source: General Review editions 2020-2025, **Table 4.4** "All India Gross Electricity Generation
  Mode-wise/Region-wise/State-wise, Utilities Only". Units GWh.
* Pages (1-based PDF page): GR2020 p75 (FY2018-19), GR2021 p75 (FY2019-20), GR2022 p73 (FY2020-21),
  GR2023 p76 (FY2021-22), GR2024 p87 (FY2022-23), GR2025 p76 (FY2023-24).
* Columns read: Hydro, Steam, Diesel, Gas, Total Thermal, Nuclear, RES, Total, for each state/UT row, each
  "Central Sector XX" row, each "Sub Total (XX)" row, "Central Sector All India" and "Total All India".
  Labels wrapped over two text lines were re-joined. The region of each state is the sub-total it is listed
  under.
* FY2017-18: totals only, from the previous-year column of GR2020 **Table 4.3** ("All India Gross Electricity
  Generation State-wise, Utilities", p74). Rows have `source == total`, `note` says so.
* Checks run on every row and table: steam + diesel + gas = total thermal; hydro + thermal + nuclear + RES =
  total; states + central sector = region sub-total; five regions = All India. Largest difference found:
  0.01 GWh (rounding in the source).
* Coverage: FY2017-18 (totals) and FY2018-19 to FY2023-24 (by mode); 36 states/UTs per year up to FY2021-22,
  35 from FY2022-23 (Dadra and Nagar Haveli and Daman and Diu merged), plus 5 regional central-sector rows,
  5 regions and India.
* Caveats:
  * **Central-sector stations (NTPC, NHPC, NPCIL, DVC, BBMB, etc.) are not allocated to states.** They appear as
    `state == "Central Sector (unallocated)"` per region (`row_type == central_sector`). A state's value is
    state-sector plus private-sector generation located in it. Region rows are complete.
  * RES is one undivided column (`res_total`); for the split by type see `cea_state_res_by_type_fy.csv`.
  * `hydro_large` = conventional hydro above 25 MW; small hydro is inside RES.
  * Name changes kept as printed: `Dadra and Nagar Haveli` and `Daman and Diu` separate up to FY2020-21,
    merged afterwards. J&K is reported as "Jammu & Kashmir" (FY2018-19, FY2020-21) or "J&K and Ladakh"
    (other years); all mapped to `Jammu and Kashmir and Ladakh`. Odisha/Orissa normalised to Odisha.
  * Utilities only (captive plants excluded).

### state/cea_state_res_by_type_fy.csv  (588 rows)

* Source: GR2024 **Table 4.7** (p90, FY2022-23) and GR2025 **Table 4.7** (p79, FY2023-24), "State-wise
  Electricity Generation by Renewable Power". Columns Wind, Solar, Biomass, Bagasse, Small Hydro, Waste to
  Energy, Total; rows per state, per region, All India. Units GWh. Earlier editions have no such table.
* Checks: row sums and region/India sums match within 0.02 GWh; region and India totals equal the RES column
  of Table 4.4 exactly.
* Caveat: unlike Table 4.4, this table allocates central-sector RES to the host state, so state RES here can
  exceed the state `res_total` in the utilities file (e.g. Rajasthan FY2023-24: 47,148.96 vs 43,130.66). Jammu
  and Kashmir and Ladakh are separate rows here.

### state/cea_state_captive_generation_fy.csv  (1,832 rows)

* Source: General Review 2020-2025, **Table 5.7** "State-wise All India Electricity Generation and Consumption
  by Captive Power Plants", "Energy Generated" part only (consumption columns ignored). Pages: GR2020 p94,
  GR2021 p94, GR2022 p91, GR2023 p98, GR2024 p112, GR2025 p94.
* Columns: Hydro, Steam, Diesel, Gas, Solar, Wind, Total (FY2018-19 to FY2021-22); Biomass added from FY2022-23.
* Coverage: FY2018-19 to FY2023-24, states, regions, India. Row, region and India sums agree within 0.02 GWh;
  All-India totals equal Table 1.6.
* Caveats: plant threshold 1 MW up to FY2020-21, 0.5 MW from FY2021-22. `coal` here is the captive "Steam"
  column (mainly coal, may include other steam fuels).

### state/ember_state_generation_cy.csv  (2,176 rows)

* Source: Ember `india_yearly_full_release_long_format.csv`. Units GWh (no conversion).
* Extraction: `Category == "Electricity generation"`, `Unit == "GWh"`, `Subcategory` in Fuel/Total; variables
  Bioenergy, Coal, Gas, Hydro, Nuclear, Other Fossil, Other Renewables, Solar, Wind, Total Generation. The
  `Others` placeholder state (all values zero or blank) was dropped. "India Total" is stored as
  `region == All India, state == ALL`. Region assigned from `state_region_map.csv`; Ember's separate Jammu and
  Kashmir and Ladakh rows were assigned to Northern.
* Coverage: calendar years 2019-2024, 36 states/UTs.
* Caveats (from Ember's methodology PDF): built from CEA monthly generation reports (conventional) and CEA
  renewable generation reports; **excludes captive generation**; "Hydro includes small hydro and conventional
  hydro"; Solar includes rooftop and off-grid where reported; Bioenergy includes bagasse, biomass cogeneration
  and waste-to-energy; Other Fossil includes diesel; Dadra & Nagar Haveli and Daman & Diu treated as one UT
  throughout; J&K and Ladakh reported together until July 2021. Central-sector plants are counted in the
  state where they are located, so state values differ from the CEA utilities file by design.

### state/ember_state_generation_monthly.csv  (30,086 rows)

* Source: Ember `india_monthly_full_release_long_format.csv`, same filters as above. Coverage January 2019 to
  November 2025.

### state/ember_state_generation_fy.csv  (2,184 rows)

* Built from the monthly file: months April to March summed into Indian fiscal years; only fiscal years with
  all 12 months kept (FY2019-20 to FY2024-25). FY2025-26 is incomplete and was excluded.
* Cross-check: Ember FY sum of states vs CEA utility total: FY2019-20 1,383,496 vs 1,383,417; FY2023-24
  1,734,063 vs 1,734,375 (within 0.03%).

### state/state_region_map.csv  (37 rows)

* State/UT to grid region, derived from the row grouping of CEA Table 4.4 (each state is listed above its
  region's sub-total). Includes pre- and post-merger names for Dadra and Nagar Haveli / Daman and Diu.

### region/cea_region_generation_fy.csv  (294 rows)

* The `region_total` and `india_total` rows of `cea_state_generation_fy_utilities.csv` (region sub-totals
  include central-sector stations). FY2017-18 totals only; FY2018-19 to FY2023-24 by mode.

### region/ember_region_generation_cy.csv and region/ember_region_generation_fy.csv  (300 rows each)

* Sums of the Ember state rows by region and source (CY2019-2024; FY2019-20 to FY2024-25). Utilities only.

---

## Sources tried that did not work

* NITI Aayog India Climate & Energy Dashboard (https://iced.niti.gov.in/energy/electricity/generation):
  connection to port 443 timed out; no data obtained.
* National Power Portal monthly generation reports (e.g.
  `https://npp.gov.in/public-reports/cea/monthly/generation/18 col act/2025/JUL/18 col act-16_2025-JUL.pdf`,
  also tried 2024/MAR, 2018/MAR, 2015/MAR): HTTP 404.
* Old CEA URLs (`cea.nic.in/wp-content/uploads/general/...`, `cea.nic.in/all-india-electricity-statistics-general-review/`):
  HTTP 404 after the site redesign; the same reports are now served via `/api/dashboardReports/download-pdf/`.
* General Review 2015-2018 (FY2013-14 to FY2016-17): the Internet Archive only holds the front matter
  (12-15 page contents PDFs, `cea.nic.in/wp-content/uploads/2020/04/<year>.pdf`); the data tables are not
  available, so state-level history before FY2017-18 could not be obtained. These files were not kept.
* No accessible source with region-wise generation before FY2017-18 was found; the Growth book has no
  regional generation table.

## Quick reference numbers (for cross-checking)

| Series | Period | GWh |
| --- | --- | --- |
| Ember national (incl. captive estimate) | CY2015 | 1,310,560 |
| Ember national | CY2024 | 2,035,115 |
| Ember national | CY2025 | 2,080,163 |
| OWID | CY2025 | 2,081,600 |
| CEA utilities | FY2015-16 | 1,167,584 |
| CEA utilities | FY2023-24 | 1,734,375 |
| CEA utilities | FY2024-25 | 1,824,214 |
| CEA captive | FY2023-24 | 223,903 |
| CEA captive (estimated) | FY2024-25 | 235,098 |
| Ember sum of states (utilities) | CY2024 | 1,804,424 |
