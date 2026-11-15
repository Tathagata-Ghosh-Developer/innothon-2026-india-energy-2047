# National Power Portal (NPP) generation data: sources and extraction notes

NPP (https://npp.gov.in, run by the Central Electricity Authority, CEA) is one of the two datasets allowed for the
core 2047 energy prediction. Everything in this folder comes from NPP only.

All tidy files hold **energy generated** (GWh; 1 GWh = 1 million units, MU) or, for the real-time Merit series,
generation power (MW) together with the energy derived from it. Installed or monitored capacity, plant load factor,
programme / target, consumption and demand columns of the raw reports are **not** carried into the tidy files.

Access date for every download and API call: **2026-11-15**.

## Folder layout

| Path | Content |
| --- | --- |
| `npp_national_generation_fy.csv` | All-India fiscal-year generation by source from the NPP dashboard chart, FY1946-47 to FY2023-24 |
| `npp_region_generation_monthly.csv` | Monthly generation by grid region and source (CEA 18-column Sub-report 2), Jul-2018 to Sep-2026 |
| `npp_region_generation_fy.csv` | Fiscal-year generation by grid region and source (same report), FY2018-19 to FY2025-26, plus FY2026-27 Apr-Sep (partial) |
| `npp_state_generation_fy.csv` | Fiscal-year generation by state and fuel, aggregated from station rows (CEA 18-column Sub-report 16) |
| `npp_res_generation_monthly.csv` | Monthly All-India solar and wind generation integrated from the NPP Merit real-time snapshots, Sep-2025 to Nov-2026 |
| `npp_merit_generation_hourly.csv` | Hourly All-India generation by source (Merit real-time snapshots), 2025-09-18 to 2026-11-15 |
| `npp_merit_generation_daily.csv` | Daily All-India generation by source (same), 2025-09-18 to 2026-11-15 |
| `downloads/` | Raw files (see the last section) |

## Conventions shared by the tidy files

| Column | Meaning |
| --- | --- |
| `fiscal_year` | Indian fiscal year, April to March, written `2023-24` |
| `fy_start_year` | Calendar year in which the fiscal year **starts** (`2023-24` -> 2023). Note that `data/raw/generation/` uses `fy_end_year` instead; `fy_end_year = fy_start_year + 1`. |
| `month` | `YYYY-MM` (calendar month) |
| `region` | `Northern`, `Western`, `Southern`, `Eastern`, `North-Eastern`, `Bhutan import`, `All India` |
| `generation_gwh` | Gross generation in GWh as printed by CEA (stations above 25 MW, see caveats) |
| `is_subtotal` | `True` if the row is the sum of other rows of the same file (region `total`, every `All India` row, Merit `total`, `solar_plus_wind`). Sum only `is_subtotal == False` rows to avoid double counting. |
| `partial` | `True` for a fiscal year that is not complete (FY2026-27 Apr-Sep) |
| `period` | Months covered by a fiscal-year value: `Apr-Mar`, or `Apr-Sep` for the partial year |
| `origin` | Which column of the report the value was read from (see each file) |
| `source_id` | `npp_cea_18col_sr{N}_{YYYY-MON}_{column}` for report values, or the dashboard / API name |

## Tidy files

### `npp_national_generation_fy.csv` (32 rows)

Columns: `fiscal_year, fy_start_year, thermal_gwh, hydro_gwh, nuclear_gwh, res_gwh, total_gwh, source_id`.

* Source: dashboard API `https://npp.gov.in/dashBoard/get_generation_chart_list` (GET, no parameters), array
  `linechartforGeneration`, fields `financial_year, thermal_total, hydro, nuclear, renewable_energy_sources`, GWh.
  The chart is drawn on https://npp.gov.in/dashBoard/gc-map-dashboard.
* Coverage: FY1946-47 to FY2023-24. Years before FY2006-07 are sparse (1946-47, 1949-50, 1955-56, 1960-61, 1965-66,
  1968-69, 1973-74, 1978-79, 1979-80, 1984-85, 1989-90, 1991-92, 1996-97, 2001-02); annual from FY2006-07. The API
  returns nothing after FY2023-24.
* `total_gwh` = thermal + hydro + nuclear + RES, computed here. Bhutan import is **not** included.
* `thermal_gwh`, `hydro_gwh`, `nuclear_gwh` are the monitored conventional stations (above 25 MW) of the 18-column
  reports (they match Sub-report 2 within 0.008 %, see validation). `hydro_gwh` is large hydro only; small hydro is
  inside `res_gwh`. `res_gwh` is a single total (no split into solar, wind, biomass, small hydro).

### `npp_region_generation_monthly.csv` (2,376 rows)

Columns: `month, fiscal_year, fy_start_year, region, source, generation_gwh, is_subtotal, origin, source_id`.

* Source: CEA monthly "18 column" generation report, **Sub-report 2 "Summary - Region wise"**, one file per month.
* `source`: `thermal` (coal, lignite, gas, liquid fuel and diesel stations together), `nuclear`, `hydro` (large hydro
  above 25 MW), `bhutan_import`, `total`. Eastern and North-Eastern have no nuclear rows. Region `Bhutan import` has one
  row (`bhutan_import`); the `All India` rows include Bhutan import in `total`.
* `origin = current_month`: column 4 "ACTUAL" of the month's own report (first print), Jul-2019 to Sep-2026
  (87 months, 2,088 rows).
* `origin = same_month_last_year`: column 5 "ACTUAL SAME MONTH LAST YEAR" of the report published 12 months later. Used
  only where the month's own report is not on NPP: Jul-2018 to Jun-2019 (12 months, 288 rows).
* Coverage: 99 months, Jul-2018 to Sep-2026, no gaps. Apr-Jun 2018 are **not** available month by month (see caveats);
  FY2018-19 is therefore incomplete in this file but complete in the fiscal-year file.

### `npp_region_generation_fy.csv` (216 rows)

Columns: `fiscal_year, fy_start_year, region, source, generation_gwh, is_subtotal, partial, period, origin, source_id`.

* FY2019-20 to FY2025-26: column 9 "ACTUAL" April-to-March cumulative of each March report (`origin = cumulative_actual`).
* FY2018-19: column 10 "ACTUAL SAME PERIOD LAST YEAR" of the Mar-2020 report (`origin = cumulative_same_period_last_year`).
* FY2026-27: column 9 of the Sep-2026 report, April to September only (`partial = True`, `period = Apr-Sep`).
* 24 rows per fiscal year (same region x source grid as the monthly file), 9 fiscal years.

### `npp_state_generation_fy.csv` (656 rows)

Columns: `fiscal_year, fy_start_year, region, state, source, generation_gwh, is_subtotal, partial, period, origin, source_id`.

* Source: CEA 18-column **Sub-report 16 "Region wise, State wise and Station wise details"**, March files 2020 to 2026
  and the Sep-2026 file. Every station row was assigned the fuel printed on the organisation sub-total row that
  follows it, then station values were summed to state x fuel. Stations are reported in the state where they are
  located (central-sector stations included), so these are generation-by-location figures.
* `source`: `coal_lignite` (coal and lignite stations; the 2025 and 2026 reports no longer separate lignite, so the
  two are combined for every year), `gas` (stations under "NATURAL GAS", including combined-cycle plants marked
  "(Liq.)" that run on liquid fuel), `diesel`, `nuclear`, `hydro` (large hydro above 25 MW). Bhutan import is not a
  state and is not in this file.
* `is_subtotal` is always `False` (no total rows are stored); state totals = sum over `source`.
* Coverage: FY2018-19 (from the last-year column of the Mar-2020 report, `origin = cumulative_same_period_last_year`)
  to FY2025-26, plus FY2026-27 Apr-Sep (`partial = True`). 33 states/UTs per year up to FY2021-22 and 34 from
  FY2022-23 (Lakshadweep diesel added). Rows with zero generation are kept when the report lists the station.
* State names follow `data/raw/generation/state/state_region_map.csv` spelling (e.g. `Chhattisgarh`,
  `Andaman and Nicobar Islands`). `Jammu and Kashmir` and `Ladakh` are separate, as printed by CEA.
  Andaman and Nicobar Islands is listed under the Eastern region and Lakshadweep under the Southern region, as in the report.

### `npp_res_generation_monthly.csv` (45 rows)

Columns: `month, fiscal_year, fy_start_year, region, source, energy_gwh_observed, energy_gwh_estimated, coverage,
days_with_data, days_in_month, is_complete, is_subtotal, source_id`.

* The only renewable generation series on NPP after FY2023-24 (see "Renewable generation on NPP" below). National
  only (`region = All India`); `source` = `solar`, `wind`, `solar_plus_wind` (subtotal).
* Built from the hourly Merit series: `energy_gwh_observed` = energy integrated over the minutes that have data;
  `coverage` = covered minutes / minutes in the month; `energy_gwh_estimated` = observed / coverage (uniform scale-up
  for missing time). `is_complete` = coverage >= 0.99 (Nov-2025, Apr-2026, May-2026).
* Sep-2025 starts on 2025-09-18 and Nov-2026 stops on 2026-11-15; both have low coverage and large scale-ups.
* Biomass, bagasse, small hydro and other RES are **not** in Merit, so this is not total RES.

### `npp_merit_generation_hourly.csv` (64,141 rows)

Columns: `datetime_ist, source, mean_mw, energy_gwh, coverage, n_obs, is_complete, is_subtotal`.

* Source: real-time "All India Generation (MW)" snapshots behind https://npp.gov.in/dashBoard/gc-map-dashboard-meritchart
  (API below). `source`: `thermal`, `gas`, `hydro`, `nuclear`, `solar`, `wind`, and `total` (sum of the six, only for
  hours where all six are present, `is_subtotal = True`).
* `datetime_ist` = start of the hour, Indian Standard Time (UTC+05:30), `YYYY-MM-DD HH:MM`.
* `mean_mw` = time-weighted mean power over the covered minutes of the hour (snapshots linearly interpolated on a
  1-minute grid; gaps longer than 30 minutes are not interpolated). `energy_gwh` = `mean_mw` x 1 h / 1000.
  `coverage` = covered minutes / 60. `n_obs` = number of raw snapshots in the hour. `is_complete` = coverage >= 0.999.
* Coverage: 2025-09-18 12:00 to 2026-11-15 23:00 IST, 9,163 hours with data (9,056 with all six sources and full
  coverage). Hours with no data at all are absent from the file.

### `npp_merit_generation_daily.csv` (2,793 rows)

Columns: `date, source, energy_gwh, mean_mw, coverage, n_obs, is_complete, estimation, is_subtotal`.

* `energy_gwh` = daily mean power over the covered minutes x 24 h / 1000. `coverage` = covered minutes / 1,440.
  `estimation` = `integrated` when the day is complete (coverage >= 0.999, 332 days) and
  `scaled_from_partial_coverage` otherwise (67 days; 12 of them below 50 % coverage). Partial-day values for solar are
  biased when the missing hours are daytime or night-time only; filter on `is_complete` or `coverage` before use.
* Coverage: 399 days with data between 2025-09-18 and 2026-11-15 (7 rows per day). 25 days return no data (below).

## Endpoints and URL patterns

All requests are plain HTTPS GET with a browser User-Agent; no login, cookie or token is needed.

| What | URL / parameters | Notes |
| --- | --- | --- |
| 18-column monthly report, actual | `https://npp.gov.in/public-reports/cea/monthly/generation/18_col_act/{YYYY}/{MON}/18_col_act-{N}_{YYYY}-{MON}.xls` (also `.pdf`) | `{MON}` = `JAN`..`DEC`, `N` = 1..20. Earliest month on NPP: Jul-2019 (earlier months return HTTP 404). Latest at access: Sep-2026. |
| 18-column monthly report, tentative | `https://npp.gov.in/public-reports/cea/monthly/generation/18_col_ten/{YYYY}/{MON}/18_col_ten-{N}_{YYYY}-{MON}.xls` | Latest at access: Oct-2026 (not used). |
| Daily generation reports (DGR) | `https://npp.gov.in/public-reports/cea/daily/dgr/{DD-MM-YYYY}/dgr{N}-{YYYY-MM-DD}.xls` (also `.pdf`) | `N` = 1..17. Archive page: https://npp.gov.in/dgrReports. Not used (conventional stations only, see below). |
| Dashboard generation chart | `https://npp.gov.in/dashBoard/get_generation_chart_list` | No parameters. JSON keys `linechartforGeneration`, `initialChartData`. |
| Dashboard header / daily figures | `https://npp.gov.in/dashBoard/getAllZoneGen?ID=IND` | `ID` is ignored. Daily and cumulative conventional generation, capacity, coal stock, PLF. Not used. |
| Dashboard map data | `https://npp.gov.in/dashBoard/getBMapData?ID=IND` | Station list (thermal, hydro, nuclear only), state and zone lists, projects. No generation. Not used. |
| Merit real-time generation | `https://npp.gov.in/dashBoard/demandmet2chartdata?date=YYYY-MM-DD` | JSON list of `{updated_on, name_of_data, value_of_data}`; `updated_on` = epoch milliseconds (UTC), `name_of_data` in `THERMAL GENERATION, GAS GENERATION, HYDRO GENERATION, NUCLEAR GENERATION, SOLAR GENERATION, WIND GENERATION`, `value_of_data` = MW. One IST calendar day per request. Returns `[]` for days without data and for other date formats (`DD-MM-YYYY` gives `[]`). Extra parameters (`region`, `state`, `ID`, `type`) are ignored: there is no regional or state split. First day with data: **2025-09-18** (from 12:39 IST). |
| Merit real-time demand met | `https://npp.gov.in/dashBoard/demandmet1chartdata?date=YYYY-MM-DD` | Same format, `name_of_data = DEMAND MET`, MW. Stored raw only (not a generation series). |

The page JavaScript sends `X-Requested-With: XMLHttpRequest` and a `Referer` of the Merit page; the API also answers
without them. `res/getWbDbData`, referenced in an old script, returns 404.

18-column sub-reports (Sep-2026 titles): 1 All India summary; 2 Region wise; 3 Thermal, hydro and nuclear
consolidated; 4 Sector wise; 5-7 Thermal by central / state / private sector; 8-10 Hydro by sector and nuclear;
11 Fuel wise coal / lignite / multi fuel; 12-14 Fuel wise gas and liquid fuel; 15 Fuel wise diesel; 16 Region, state
and station wise; 17 Summary all India (MU and PLF by region); 18-20 Sector wise details (central, state, private).

## Extraction procedure (reproducible)

One-off Python scripts (Python 3.13, `xlrd` 2.0 for the `.xls` files, `pandas`) were run outside the repository; only
their outputs are stored here. Steps:

1. **Downloads.** For every month Jul-2019 to Sep-2026, fetch Sub-reports 1 and 2 with the 18-column URL pattern; fetch
   Sub-report 16 for March 2020 to March 2026 and Sep-2026. Probing earlier months (2013-2019 before July) returns 404.
2. **National chart.** Read `linechartforGeneration`; write one row per fiscal year; `total_gwh` = sum of the four sources.
3. **Sub-report 2.** Each file has one sheet with the region blocks `NORTHERN REGION` ... `ALL INDIA REGION`, each with
   `THERMAL / NUCLEAR / HYDRO / Total` rows (the `BHUTAN IMP. REGION` row has no label). Physical columns are stable
   across all 87 files and were checked against the column-number row (`1`..`18`): index 4 = month ACTUAL,
   5 = ACTUAL SAME MONTH LAST YEAR, 9 = cumulative ACTUAL (April to month), 10 = cumulative ACTUAL SAME PERIOD LAST YEAR.
   Monthly values: column 4 of each file; for months without their own file, column 5 of the file 12 months later.
   Fiscal years: column 9 of March files, column 10 of the Mar-2020 file for FY2018-19, column 9 of Sep-2026 for the
   partial FY2026-27.
4. **Sub-report 16.** Two layouts exist (25 columns in 2020-2024, 27 columns in 2025-2026); columns are located from the
   column-number row. Rows are: region header, state header, sector header (`STATE SECTOR`, `PVT SECTOR`,
   `CENTRAL SECTOR`), station rows (no fuel label), organisation sub-total rows (fuel label `COAL`, `LIGNITE`,
   `NATURAL GAS`, `DIESEL`, `HYDRO`, `NUCLEAR`), sector sub-totals, `TOTAL <FUEL>` and `TOTAL <STATE>` rows. Station
   rows are buffered and given the fuel of the next organisation sub-total; stations listed without numbers are
   skipped. Station cumulative ACTUAL (and, for Mar-2020, the last-year column) are summed to state x fuel.
5. **Merit.** For every date from 2025-09-18 to 2026-11-15 (424 days) request `demandmet2chartdata` and
   `demandmet1chartdata` (4 parallel requests at most). Convert `updated_on` to IST and floor to the minute; drop
   6 duplicate (minute, source) snapshots. Glitch screening: 1 negative hydro value set missing; 167 timestamps whose
   six-source total departs by more than 8 % from the centred 2-hour rolling median of the total are set missing for
   all six sources (short telemetry drop-outs and spikes, e.g. 2025-10-29 15:31-16:00 when thermal fell from
   93 GW to 57 GW for 30 minutes). Small negative solar values at night (522 snapshots, median -25 MW) are kept.
   Interpolate each source linearly on a 1-minute grid across gaps of at most 30 minutes, then average to hours,
   days and months as described above.

## Validation

* **Monthly vs cumulative (Sub-report 2).** For every complete fiscal year FY2019-20 to FY2025-26 and every
  region x source, the sum of the 12 monthly values equals the March cumulative ACTUAL within **0.001 %**
  (largest All-India total gap 0.65 GWh, FY2021-22). FY2026-27 Apr-Sep: 0.000 %. FY2018-19: Jul-Mar monthly sum plus
  Apr-Jun 2018 derived from the Jul-2019 report (cumulative last year minus Jul-2018 = 318,985.01 GWh) equals the
  FY2018-19 total exactly.
* **Regions vs All India.** Every month, the five regions sum to the All-India value of each source within 0.02 GWh;
  region totals plus Bhutan import sum to the All-India total within 0.03 GWh; sources sum to each region total within
  0.05 GWh.
* **Sub-report 1 vs Sub-report 2.** The All-India monthly ACTUAL of Sub-report 1 equals the All-India rows of
  Sub-report 2 within 0.13 GWh (435 comparisons).
* **Revisions.** For the 75 months that appear both as first print and as "same month last year" one year later
  (1,800 rows), 48 rows differ by more than 0.5 GWh: all are Eastern-region thermal (and the Eastern / All-India
  totals) for Apr-2024 to Mar-2025, revised down by up to 116 GWh a month (at most 0.086 % of the All-India total).
  The FY2024-25 Eastern thermal total is 261,751.09 GWh in the Mar-2025 report and 260,930.52 GWh (-0.31 %) in the
  last-year column of the Mar-2026 report. The tidy files keep the first print.
* **National chart vs Sub-report 2.** Chart thermal, hydro and nuclear equal the Sub-report 2 All-India fiscal-year
  values within 0.008 % for FY2018-19 to FY2023-24.
* **States (Sub-report 16).** In every file the station sums equal the organisation sub-total rows, the `TOTAL <STATE>`
  rows and the `TOTAL <FUEL>` rows (diesel is counted inside `TOTAL THERMAL`) exactly; state sums by region equal the
  Sub-report 2 region thermal / nuclear / hydro values to 0.00 GWh for all nine fiscal years, including FY2018-19
  from the last-year column.
* **Merit vs Sub-report 2** (months with every day present: Nov-2025, Mar-2026 to Jul-2026, Sep-2026): Merit
  thermal + gas is 5-6 % below Sub-report 2 thermal, Merit nuclear 8-11 % below, Merit hydro between -2 % and +6 %.
  Merit is a real-time telemetry series, probably net of auxiliary consumption and possibly incomplete in station
  coverage; it is suitable for daily / hourly shapes and for solar and wind, not as a replacement for the CEA gross
  figures.

## Renewable generation on NPP

* The 18-column monthly reports and the daily generation reports (DGR 1-17) cover only conventional stations above
  25 MW; both print "It excludes generation from RES". DGR 1 shows RES installed capacity only; DGR 17 (category and
  fuel wise) splits thermal into coal, lignite, natural gas, diesel, high-speed diesel and naphtha but has no RES rows.
* The dashboard APIs `getAllZoneGen` and `getBMapData` contain no RES generation.
* RES generation on NPP exists in two places only:
  1. the dashboard chart (`npp_national_generation_fy.csv`): All-India annual RES total, ending FY2023-24;
  2. the Merit real-time series (`npp_merit_*`, `npp_res_generation_monthly.csv`): All-India **solar** and **wind**
     separately, from 2025-09-18. No region or state split exists on NPP for either.
* So FY2024-25 is not covered by any NPP RES series, and FY2025-26 only from 18 September 2025 (with gaps).

## Caveats

* **Station scope.** The 18-column reports and the NPP chart's conventional series cover stations **above 25 MW**
  monitored by CEA (utilities). Small hydro (25 MW and below), all RES and captive plants are excluded. The
  All-India `total` in Sub-report 2 includes Bhutan import; the chart's `total_gwh` here does not.
* **Bhutan import** is electricity imported from hydro stations in Bhutan. It is reported as its own region and is not
  generation inside India; exclude it when modelling domestic generation.
* **Apr-Jun 2018** monthly values are not recoverable: the earliest report on NPP is Jul-2019 and its last-year column
  reaches back only to Jul-2018. The Apr-Jun 2018 quarter total can be derived (Jul-2019 report: column 10 minus
  column 5), but is not stored.
* **First print vs revised values.** Monthly values are first prints; CEA revises some months later (see validation).
* **NPP chart RES vs other CEA publications.** The chart's RES is 15-25 % lower than the RES totals in the CEA General
  Review / Growth of Electricity Sector publications used in `data/raw/generation/` (FY2018-19: 97,924 vs 126,759 GWh;
  FY2022-23: 172,488 vs 203,555 GWh; FY2023-24: 189,490 vs 225,835 GWh). The scope behind the chart's RES is not
  documented on NPP.
* **Merit categories.** Merit `thermal` appears to be coal and lignite, with `gas` separate. Merit has no category for
  biomass, small hydro or other RES. Whether Merit solar includes rooftop solar is not documented.
* **Merit archive.** The organisers mentioned about one year of history; at access the API served data back to
  2025-09-18. Older days may disappear, which is why the raw snapshots are archived in `downloads/merit/`. The last
  day (2026-11-15) was still in progress at access.
* **Merit gaps.** Snapshot interval: median 4 minutes (69 % of steps 4 min, 15 % 2 min, 14 % 3 min, 99.6 % of steps
  at most 5 min); timestamps carry seconds and were floored to the minute. 49 gaps longer than 30 minutes (1,048 hours
  in total, 10 longer than a day). Days with an empty response (25): 2025-10-09 to 2025-10-14, 2025-12-06 to
  2025-12-09, 2026-01-10, 2026-01-11, 2026-01-13, 2026-01-14, 2026-01-17 to 2026-01-19, 2026-01-31 to 2026-02-04,
  2026-08-08, 2026-08-09, 2026-10-04.
* **Units.** CEA reports print GWh (= MU). Merit values are MW snapshots; energy is derived by time integration.

## Raw downloads (`downloads/`, about 11 MB, every file below 1 MB)

| Path | Content |
| --- | --- |
| `downloads/npp_dashboard/get_generation_chart_list_2026-11-15.json` | Response of `dashBoard/get_generation_chart_list` |
| `downloads/cea_18col_sr2/18_col_act-2_{YYYY}-{MON}.xls` | Sub-report 2, 87 files, Jul-2019 to Sep-2026 (original files, original names) |
| `downloads/cea_18col_sr16/18_col_act-16_{YYYY}-{MON}.xls` | Sub-report 16, Mar-2020 to Mar-2026 and Sep-2026 (8 files) |
| `downloads/merit/merit_generation_snapshots_{YYYY-MM}.csv.gz` | All `demandmet2chartdata` responses of the month, flattened without any change: `requested_date, updated_on, name_of_data, value_of_data` (916,188 snapshot values) |
| `downloads/merit/merit_demandmet_snapshots_{YYYY-MM}.csv.gz` | All `demandmet1chartdata` responses (All-India demand met, MW), same layout. Raw only; not a generation series. |

Not copied (URLs only, see the patterns above): Sub-report 1 (Jul-2019 to Sep-2026, used only for the cross-check),
the other 18-column sub-reports, the DGR files, `getAllZoneGen` and `getBMapData` responses, and the 848 individual
Merit JSON responses (88 MB uncompressed; their full content is in the monthly `.csv.gz` files).
