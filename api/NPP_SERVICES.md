# National Power Portal: service inventory

This file lists the services behind the public pages of the National Power Portal (<https://npp.gov.in>, run by the Central Electricity Authority, CEA), what each returns, and whether this project may use it.
`data/raw/npp/SOURCES.md` describes the data the project actually extracted; `api/README.md` describes the test collection.

## How the inventory was built

- Pages read on 2026-11-16: `landing-home`, `dashBoard/gc-map-dashboard` (generation map), `dashBoard/cp-map-dashboard` (capacity map), `dashBoard/gc-map-dashboard-meritchart` (Merit real-time chart), `publishedReports` and `dgrReports`, with the scripts they load.
  Every AJAX call in those pages and scripts was traced to its URL, HTTP method and parameters.
- Each JSON service was then called once or twice with the parameters the page sends (sequential requests at least 1.5 s apart, browser User-Agent, no cookies or login), on 2026-11-16 (IST).
  The response shapes below come from those calls; `api/fixtures/` keeps one recorded response per service in the collection.
- Re-checked on 2026-11-17 (IST): the same pages, plus `dashBoard/trans-map-dashboard` (transmission map), `sitemap`, the report archive pages (`dgrReports`, `monthlyGenerationReportsAct`, `monthlyGenerationReportsTen`, `dailyCoalReports`, `monthlyCoalReports`, `monthlyTransmissionReports`, `installedCapacityReports`) and `site-search`, and every script they load (`web-db.js`, `cp-map-dashboard.js`, `task.query.js`, `giskey.js`, the `*_language.js` files, `npp-report-datepicker.js`, `main.js`).
  Unlinked report files and service names were probed with `HEAD` (files) or one `GET` each (services), 1.5 to 2 s apart; the results are in the sections below.
- All dashboard services are plain `GET` requests with query parameters (jQuery `$.ajax` with a `data` object), answer `application/json`, and need no authentication.
  Paths are relative to `https://npp.gov.in/dashBoard/` unless a full URL is shown.

Ids used as parameters (from `getBMapData`):
zones (regions) `100001` Northern, `100002` Western, `100003` Southern, `100004` Eastern, `100005` North Eastern, `100006` Bhutan import;
states use the `state_id` values of `getBMapData.stateList` (for example `27` Maharashtra, `29` Karnataka, `9` Uttar Pradesh).

## What the competition rules allow

NPP is one of the two challenge datasets for the core 2047 prediction (the other is the organisers' Renewable India dataset).
Within NPP, the project uses **generation** only.
Installed capacity must not be used ("does not guarantee energy", kickoff 13 Nov 2026), and consumption or demand series are not used as forecasting inputs (`docs/decisions.md`, `docs/methodology.md`).
The tables below give each service a rules category (generation, consumption, capacity, fuel or plumbing; demand counts as consumption) and apply the rule in the column "May the project use it?".

## Dashboard services

| Service | Parameters | What it returns | Rules category | May the project use it? | In collection |
| --- | --- | --- | --- | --- | --- |
| `get_generation_chart_list` | none | All-India generation by source per fiscal year, GWh: `linechartforGeneration` (32 years, FY1946-47 to FY2023-24, sparse before FY2006-07), `initialChartData` (first 4 years) | Generation | **Yes, used.** Feeds `data/raw/npp/npp_national_generation_fy.csv`, the input of the India-only forecast core | Yes |
| `getAllZoneGen` | `ID=IND` (ignored) | Latest reporting day, All India: programme and actual generation in MU for the day, the same day last year and the year to date since 1 April (`headerMap.GN_OBJ`, `dailyPGen`); also transmission totals (`TR_OBJ`), feeder monitoring (`UD_OBJ`, `RD_OBJ`), monitored capacity (`CP_OBJ`), coal stock (`dailyColeStock`), plant load factor by fiscal year (`plfMonthWise`) | Generation, mixed with capacity and fuel | Generation fields yes, but only the latest day (no history). `CP_OBJ` capacity: no. Not used | Yes |
| `getAllZoneStateGen` | `zoneId` | Latest reporting day for one region: one row per state in `dailyGenStn` (programme and actual MU for the day, same day last year, year to date and last year to date), the region total in `dailyPGen`, `stateList`, state installed capacity in `station_details`, coal stock and PLF. Conventional stations above 25 MW only | Generation (capacity in `station_details`) | Generation fields yes, latest day only. Not used | Yes |
| `getZoneStateWiseGen` | `stateId`, `zoneId`, `genDate` (`dd-Mon-yyyy`, for example `13-Nov-2026`) | Latest reporting day: one row per **station of the whole region** in `dailyGenStn` (129 stations for Western), the selected state's total in `dailyPGen`, and the state's stations with coordinates and unit capacity in `station_details`. `genDate` is ignored (see below) | Generation (capacity in `station_details`) | Generation fields yes, latest day only. Not used | Yes |
| `getAllZone` | `ID=IND` (ignored) | All-India installed capacity, MW: by sector (`installed_Capacity_List`), by source including solar, wind, small hydro and bio power (`installed_Capacity`), monitored, online and outage capacity (`monthlyAllIndiaGen`); peak requirement and peak demand met (`dailyDemmandCp`) | Capacity and demand | **No** (capacity and demand). Kept in the collection only to watch the service | Yes |
| `getAllStateWithZone` | `zoneId` | Installed capacity by source and sector, same layout as `getAllZone`. For `zoneId=100002` it returned the All-India figures, so the zone filter appears not to be applied | Capacity | **No** | No |
| `getStateCpt` | `stateId`, `zoneId` | Installed capacity of one state by source and sector, and its monitored and online capacity (Maharashtra: 61,199 MW installed) | Capacity | **No** | No |
| `get_installed_capacity_list` | none | All-India installed capacity by source at the end of each fiscal year, MW: `linechartforCapacity` (34 points, Dec 1947 to Mar 2026) | Capacity | **No** | No |
| `get_hydro_list` | none | Despite its name, **electricity consumption** by category per fiscal year, GWh: domestic, commercial, industrial, traction, agriculture, miscellaneous and total (`linechartforHydro`, 28 points, 1947 to 2023-24) | Consumption | **No** | No |
| `getBMapData` | `ID=IND` (ignored) | Map reference lists, about 0.75 MB: stations with coordinates and capacity (`station_details`, `thermal_station_map_details`, `hydro_station_map_details`), 40 states with ids, 6 zones with ids, 555 districts, transmission map items, hydro and thermal project details. No generation values | Reference data (contains capacity) | Ids and names as a lookup only; capacity columns no. Not used | Yes (status and type only) |
| `demandmet2chartdata` | `date=YYYY-MM-DD` (one IST day) | Merit real-time snapshots every 2 to 5 minutes: `{updated_on, name_of_data, value_of_data}` for thermal, gas, hydro, nuclear, solar and wind generation, MW. Days without data return `[]`; data from 2025-09-18 | Generation | **Yes, used.** Feeds `npp_merit_generation_hourly.csv` (read by `src/allocation/storage.cpp`), the daily file and the monthly solar and wind file | Yes |
| `demandmet1chartdata` | `date=YYYY-MM-DD` | Merit real-time All-India demand met, MW, same format (`name_of_data = DEMAND MET`) | Demand | **No** as a forecasting input. Archived raw only | Yes |
| `res/getWbDbData` | `selectedDate` (epoch ms) | HTTP 404 on 2026-11-15, 2026-11-16 and 2026-11-17 (map cache of an older page script, `web-db.js`) | Plumbing | Not applicable | No |
| `getTransmissionLine` (new, 2026-11-17) | `ID=IND` (sent by the page) | Transmission map (`trans-map-dashboard`): lines commissioned (circuit km) and transformation capacity added (MVA) by voltage level, 220 kV to ±800 kV, for the current and the previous fiscal year (`mapTLine.transLineList`); yearly additions FY2022-23 to FY2026-27 (`yTransList`); the same `headerMap` as `getAllZoneGen`. About 10 kB | Capacity (transmission network) | **No** (not generation). Kept in the collection only to watch the service | Yes |
| `get-transmission-intial-data` | none | Referenced by the transmission page but disabled in its script (the URL is prefixed `xxxx`), so the page never calls it | Plumbing | Not applicable | No |

Names probed on 2026-11-17 that answer HTTP 404 (no such service): `get_res_list`, `get_renewable_list`, `get_res_generation_list`, `getAllZoneResGen`, `getResGen`, `demandmet0chartdata`, `demandmet3chartdata`, `demandmet4chartdata`, `demandmet5chartdata`, and the pages `dashBoard/re-map-dashboard` and `dashBoard/res-map-dashboard`.
`get_generation_chart_list` still ends in FY2023-24 on 2026-11-17.

## Site plumbing and third-party calls (documented, never called)

| Call | Made by | Purpose | Handling |
| --- | --- | --- | --- |
| `https://npp.gov.in/public/nppVisitCount` (GET, no parameters) | landing page on load | visitor counter shown on the home page | Not called by this project, because a request may count as a visit |
| `https://npdms.gov.in/api2/userLog?pageUrl=...&pageName=...` (GET) | dashboard, Merit and report pages | page-view logging on a separate NPP domain | **Never called**; not in the collection |
| `services.arcgisonline.com`, `js.arcgis.com` | the map dashboards | third-party basemap tiles and the ArcGIS JavaScript API | Not called; not an NPP data service |
| `/resources/bmap/js/map_3.json` (GET, 2.0 MB) | landing page and map dashboards | state boundaries as ArcGIS feature JSON (`STNAME`, `STCODE11`, `State_LGD`, geometry); no energy values | Plumbing (reference). Read once with a byte range to identify it; not used |
| `/resources/bmap/js/language.json`, `language_org.json`, `language_state_key.json`, `language_zone.json` | landing page | English and Hindi labels for the page, organisations, states and zones | Plumbing |
| `/dgrReports`, `/monthlyGenerationReportsAct`, `/monthlyGenerationReportsTen`, `/dailyCoalReports`, `/monthlyCoalReports`, `/monthlyTransmissionReports`, `/installedCapacityReports` | report archive pages | HTML pages that `POST` a chosen day or month (`dg_date` or a month field, with a `_csrf` token) back to themselves and list the files for that period. The files follow the static patterns below, so the forms are not needed | Plumbing. Not called with `POST` |
| `/site-search?s=<term>` (GET) | every page header | site search; "renewable" gives 0 results and "solar" only the capacity dashboard | Plumbing |
| `https://npdms.gov.in`, `https://drips-cea.npp.gov.in`, `/login` | login menu | NPDMS (generation and transmission users), DRIPS (disaster resource inventory) and project monitoring; all need a login | Not public; not called |

## Report files

Static files linked from `publishedReports` and the archive pages, each as `.xls` and `.pdf`, under `https://npp.gov.in/public-reports/cea/`.
Folders do not list their contents (HTTP 404), so only the patterns below are known.

| Path pattern | Content | Rules category | Use |
| --- | --- | --- | --- |
| `monthly/generation/18_col_act/{YYYY}/{MON}/18_col_act-{N}_{YYYY}-{MON}.xls` | Monthly generation, actuals, 20 sub-reports (titles below); Jul-2019 to Sep-2026 on 2026-11-17 (Oct-2026 answers 404) | Generation (also monitored capacity, target and PLF columns, which are not used) | **Used**: Sub-report 2 (regional files) and 16 (state file) in `data/raw/npp/`; Sub-report 2 is in the collection (HEAD) |
| `monthly/generation/18_col_ten/{YYYY}/{MON}/18_col_ten-{N}_{YYYY}-{MON}.xls` | Same 20 sub-reports, tentative figures, about a month ahead of the actuals; Jul-2019 to Oct-2026 on 2026-11-17 (Nov-2026 answers 404) | Generation | **Used since 2026-11-17**: Sub-report 2 of Oct-2026 feeds `npp_region_generation_tentative.csv`; in the collection (HEAD) |
| `daily/dgr/{DD-MM-YYYY}/dgr{N}-{YYYY-MM-DD}.xls` | Daily generation reports, sub-reports 1 to 17 (titles below), conventional stations above 25 MW; the file for day D appears on the evening of D+1 (IST) | Generation for 1, 2, 3, 4, 8, 9, 17; capacity, outages or reservoirs for the others | **Used since 2026-11-17**: Sub-report 1 at fiscal-year ends and month ends feeds `npp_region_generation_dgr_fy.csv` and `npp_region_generation_dgr_monthly.csv`; in the collection (HEAD, latest day and archive start). Archive: the first file is 22 Mar 2018 (21, 20 and 18 Mar 2018 and every earlier day probed answer 404); 31 Mar 2020 to 1 Apr 2020 are missing |
| `daily/fuel/{DD-MM-YYYY}/dailyCoal1-{YYYY-MM-DD}.xls`, `monthly/fuel/{YYYY}/{MON}/monthCoal1-…`, `monthImportCoal1-…`, `monthlyGasSupply-…`, `monthly/fuel/tent/…/dailyCummCoal-…` | Coal stock, coal and gas supply | Fuel | Not used |
| `monthly/installcap/{YYYY}/{MON}/capacity1-{YYYY}-{MM}.xls`, `capacity2-{Region}-{YYYY}-{MM}.xls` | Installed capacity, All India and by region | Capacity | **No** |
| `monthly/transmission/{tc,tl}/{YYYY}/{MON}/trans{N}-{YYYY}-{MM}.xls` | Transmission lines (`tl`: trans2, trans4) and transformation capacity (`tc`: trans6, trans7), commissioned monthly and yearly; the "progress status" and "RFD progress status" items have no file | Capacity (transmission network) | Not used |

Unlinked names probed with `HEAD` on 2026-11-17, all HTTP 404: `18_col_act-21` and `18_col_act-0` (Sep-2026, Mar-2024, Jul-2019), `dgr7`, `dgr10A`, `dgr10a` and `dgr18` (16 Nov 2026; the page lists sub-reports 7, 10A and 18 with no file), and `monthly/generation/res/…`.

18-column sub-reports (same numbering for actual and tentative):

| N | Title | Rules category | Use |
| --- | --- | --- | --- |
| 1 | All India | Generation | Cross-check only |
| 2 | Region wise | Generation | **Used** (regional files) |
| 3 | Thermal, hydro and nuclear consolidated | Generation | Allowed, not used |
| 4 | Sector wise | Generation | Allowed, not used |
| 5, 6, 7 | Thermal stations: central, state, private and IPP | Generation | Allowed, not used |
| 8, 9, 10 | Hydro stations by sector; central-sector nuclear | Generation | Allowed, not used |
| 11 | Coal, lignite and multi-fuel stations | Generation | Allowed, not used |
| 12, 13, 14 | Gas and liquid-fuel stations: central, state, private | Generation | Allowed, not used |
| 15 | Diesel | Generation | Allowed, not used |
| 16 | Region, state and station wise | Generation | **Used** (state file) |
| 17 | Summary All India: gas-based stations by region (monitored capacity 20,122 MW in Sep-2026; generation MU and PLF) | Generation | Allowed, not used (the state file already has gas by state) |
| 18, 19, 20 | Gas-based stations: central, state, private | Generation | Allowed, not used |

Daily generation report sub-reports (titles as listed on `publishedReports` on 16 Nov 2026):

| N | Title | Rules category | Use |
| --- | --- | --- | --- |
| 1 | All India / region wise power generation overview (thermal, nuclear, hydro by region; day and 1 April to date; RES appears as installed capacity only) | Generation (capacity columns not used) | **Used** (new DGR files) |
| 2 | Region, state, sector, type, station and unit wise generation | Generation | Allowed, not used |
| 3 | All India summary | Generation | Allowed, not used |
| 4 | Region wise, utility wise summary | Generation | Allowed, not used |
| 5 | Region wise availability (above 25 MW) | Capacity | No |
| 6 | Hydro reservoirs | Fuel (water storage) | Not used |
| 7 | Outages of hydro units | Capacity | No file published |
| 8, 9 | Overview and station report of NTPC stations | Generation | Allowed, not used |
| 10, 10A, 11 | Outages: coal, lignite and nuclear; other units (no file); units of 500 MW and more | Capacity | No |
| 12, 13, 14, 15 | Units out of the grid more than 15 days, more than a year; recommissioned; gone out | Capacity | No |
| 16 | Capacity utilisation | Capacity | No |
| 17 | Category wise, fuel wise generation (coal, lignite, natural gas, diesel, HSD, naphtha, nuclear, hydro, Bhutan import; All India) | Generation | Allowed, not used |
| 18 | Overview of power generation (web pages) | n/a | No file published |

## State-wise daily generation: is there a history?

No, not from the dashboard services.

- `getZoneStateWiseGen` ignores `genDate`: requests with `13-Nov-2026`, `13-Nov-2025` and `01-Apr-2020` returned byte-identical bodies, all for the latest reporting day (12 Nov 2026 at access).
  `getAllZoneStateGen` behaves the same with an added `genDate`.
- Each response holds one day, plus the same day last year and the totals since 1 April for this year and last year.
  That is enough for a year-to-date figure per state or station, not for a daily series.
- A state-wise daily history would have to be built by saving the latest day every day from now on, or read from the daily generation report files (`dgr`), which go back to 22 March 2018.
  Both cover conventional stations above 25 MW only (no solar or wind), and the project already has state-wise annual generation from Sub-report 16 (`npp_state_generation_fy.csv`).
- A region-wise daily history exists in DGR Sub-report 1 ("Today's Actual" by region and source), one 15 to 17 kB file per day from 22 March 2018: about 3,150 files, roughly 50 MB, 90 minutes at 1.5 s per request.
  It was not downloaded: the forecast is annual and the monthly regional series (Sub-report 2) already covers Jul-2018 onwards.
  Only the fiscal-year ends and five month ends were fetched (see "New generation data" below).
- The collection's test "genDate ... is still ignored" fails if NPP starts honouring `genDate`, so the weekly monitor would show that a history has become available.

## Renewable generation by region or state: not on NPP

Checked again on 2026-11-17; the answer is still no.

- The 18-column reports (sub-reports 1 to 20, actual and tentative) and the daily reports (sub-reports 1 to 17) cover conventional stations above 25 MW and print "It excludes generation from RES".
  DGR Sub-report 1 has an "R.E.S" row, but only with installed capacity (251,149 MW on 16 Nov 2026) and no generation.
  DGR Sub-report 2 (16 Nov 2026) and 18-column Sub-report 16 (Sep-2026) contain no solar, wind, biomass, bagasse or small-hydro station.
- No unlinked report exists under the names tried (sub-report 21 or 0, DGR 18, a `res` folder), and the site search finds nothing for "renewable".
- No dashboard service returns renewable generation by region or state (names probed above).
  `getAllZoneGen`, `getAllZoneStateGen` and `getZoneStateWiseGen` carry conventional generation only; `getAllZone`, `getAllStateWithZone` and `getStateCpt` carry renewable installed capacity, which the rules exclude.
- Merit real-time solar and wind are All India only; extra parameters are ignored and `demandmet0/3/4/5chartdata` do not exist.
- The other challenge dataset (Renewable India, `data/raw/competition/renewable_energy_dataset.csv`) has state installed capacity of biomass, small hydro, solar, wind and waste to energy, not generation, so it cannot fill the gap either.

Within the two challenge datasets, renewable generation therefore exists only for All India: the NPP chart total to FY2023-24, and Merit solar and wind from 18 Sep 2025.

### What the gap does to regional shares (context only)

The project already holds regional renewable generation from two sources outside the challenge datasets: the CEA General Review (`data/raw/generation/region/cea_region_generation_fy.csv`, FY2017-18 to FY2023-24) and Ember (`ember_region_generation_cy.csv`, CY2019 to CY2024).
They are not NPP and must not enter the core prediction; they only show how far a conventional-only regional share is from a share of total generation.

| Region | RES FY2023-24 (CEA GR), GWh | Share of India RES | RES in the region's own generation | Share of conventional, CEA GR FY2023-24 | Share of total incl. RES, CEA GR FY2023-24 | Change | Ember CY2024 share without solar, wind, bio, other | Ember CY2024 share of total | Change |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Northern | 64,791 | 28.7% | 14.1% | 26.1% | 26.4% | +0.3 pp | 24.7% | 25.2% | +0.5 pp |
| Western | 69,478 | 30.8% | 11.4% | 35.7% | 35.0% | -0.7 pp | 37.1% | 36.5% | -0.6 pp |
| Southern | 87,320 | 38.7% | 23.1% | 19.3% | 21.8% | +2.5 pp | 19.6% | 21.9% | +2.3 pp |
| Eastern | 3,599 | 1.6% | 1.3% | 17.5% | 15.4% | -2.1 pp | 17.3% | 15.3% | -2.0 pp |
| North-Eastern | 647 | 0.3% | 3.0% | 1.4% | 1.3% | -0.1 pp | 1.3% | 1.2% | -0.1 pp |

- The West's share of total generation is 0.6 to 0.7 percentage points below its share of conventional generation; the shift was similar in FY2018-19 (-0.5 pp) and FY2022-23 (-0.6 pp).
  Applied to NPP's FY2025-26 conventional share of 36.0%, the West would hold about 35.3% of total generation.
- A conventional-only split understates the South by about 2.5 pp and overstates the East by about 2 pp; these two regions carry most of the error, not the West.
- For the mix: the South already makes nearly a quarter of its generation from renewables (23%) and holds 39% of India's renewable generation, while the East makes about 1%.
  The East's coal-heavy base is where a 2047 mix changes most, and where new solar or imports over the inter-regional corridors would have to come from.

## New generation data (2026-11-17) and how it helps the forecast and the mix

Three tidy files were added. None is read by the model, so `outputs/` and the midpoint numbers do not change.
Provenance and checks: `data/raw/npp/SOURCES.md`.

### `npp_region_generation_dgr_fy.csv` (DGR Sub-report 1, fiscal years)

- 216 rows: FY2017-18, FY2018-19 and FY2020-21 to FY2025-26 complete (there is no 31 Mar 2020 file, so FY2019-20 comes only from Sub-report 2), plus FY2026-27 from 1 Apr to 16 Nov 2026 (`partial = True`).
  GWh, five regions x thermal, nuclear, hydro and total, Bhutan import, and All India.
- New: **FY2017-18 by region**, the only NPP source for that year: Northern 312,528, Western 442,134, Southern 243,671, Eastern 186,343, North-Eastern 15,984 GWh (All India with Bhutan import 1,205,579).
  The regional history now spans nine fiscal years (FY2017-18 to FY2025-26) instead of eight.
- Agreement with Sub-report 2 for the seven years both cover: All-India totals within 0.07%, region totals within 0.35%, region x source within 1.6% (Western hydro, FY2018-19).
- Growth of conventional generation, FY2017-18 to FY2025-26: Northern 2.48%/yr, Western 2.79%/yr, Southern 2.96%/yr, Eastern 4.74%/yr, North-Eastern 3.11%/yr; five regions 3.07%/yr (2.98%/yr from FY2018-19).
- Western share of conventional generation: 36.8% (FY2017-18), 37.8% (FY2018-19), 37.5% (FY2023-24), 36.0% (FY2025-26), 35.0% (1 Apr to 16 Nov 2026). The Eastern share rose from 15.5% to 17.7% between FY2017-18 and FY2025-26.
- How it helps: one more year for regional trends and the regional transfer gap, and evidence that the West's share is falling while the East (coal) and the South grow faster, so a 2047 split should not freeze today's conventional shares.

### `npp_region_generation_dgr_monthly.csv` (DGR Sub-report 1, months)

- 120 rows: Apr, May and Jun 2018, Oct 2026, and 1 to 16 Nov 2026 (partial), as differences of the "Apr 1 Till Date" actual between month-end reports.
- New: **April to June 2018 by region**, missing from `npp_region_generation_monthly.csv`. All India: 103,455, 110,609 and 103,472 GWh.
  With them FY2018-19 has all twelve months; their sum is 1,247,887 GWh, 0.12% below the Sub-report 2 fiscal-year total (the daily first prints of April to June 2018 are 0.45% below the revised quarter).
- How it helps: complete monthly seasonality per region from April 2018, and the most recent half-month (1 to 16 Nov 2026: 65,545 GWh All India).

### `npp_region_generation_tentative.csv` (18-column Sub-report 2, tentative)

- 48 rows: October 2026, and April to October 2026, by region and source, a month before the actual report.
- Reliability: for Oct-2025, Mar-2026 and Sep-2026 the tentative All-India month was within 0.23% of the actual first print, region totals within 0.75%, region x source within 5.6% (Eastern hydro, Mar-2026).
  The DGR month and year-to-date sums agree with it within 0.12%.
- April to October 2026 against April to October 2025 (conventional, tentative): Northern +6.9%, Western +7.6%, Southern +12.1%, Eastern +11.6%, North-Eastern +24.0%; five regions +9.2%.
  All India thermal +12.5%, nuclear +19.2%, hydro -13.7%; October alone +20.5%.
  FY2025-26 was 2.4% below FY2024-25 (1,537,356 against 1,574,689 GWh), so part of the rise is a rebound.
- A naive full-year figure (April to October 2026 divided by the April-to-October share of FY2025-26, 59.6%) is about 1,676 TWh of conventional generation including Bhutan import in FY2026-27, 9% above FY2025-26.
- How it helps: an early third holdout point once FY2026-27 closes, and the latest regional shares of conventional generation (April to October 2026: West 34.9%, North 25.9%, South 20.0%, East 17.5%, North-East 1.7%).

### Other generation data on NPP, not fetched

- DGR Sub-report 1 daily "Today's Actual" by region (22 Mar 2018 onwards, about 3,150 files) and DGR Sub-report 2 by station: daily conventional history, useful for daily shapes, not needed for annual totals.
- DGR Sub-report 17 at fiscal-year ends: All-India split of thermal into coal, lignite, natural gas and diesel (FY2017-18 onwards).
- 18-column sub-reports 3 to 15 and 17 to 20: sector and fuel splits of the same conventional totals; Sub-report 17 gives gas-based generation by region.
- 18-column Sub-report 16, tentative, Oct-2026: state-level April to October 2026.
