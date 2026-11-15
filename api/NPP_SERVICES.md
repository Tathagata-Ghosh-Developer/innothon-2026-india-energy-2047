# National Power Portal: service inventory

This file lists the services behind the public pages of the National Power Portal (<https://npp.gov.in>, run by the Central Electricity Authority, CEA), what each returns, and whether this project may use it.
`data/raw/npp/SOURCES.md` describes the data the project actually extracted; `api/README.md` describes the test collection.

## How the inventory was built

- Pages read on 2026-11-16: `landing-home`, `dashBoard/gc-map-dashboard` (generation map), `dashBoard/cp-map-dashboard` (capacity map), `dashBoard/gc-map-dashboard-meritchart` (Merit real-time chart), `publishedReports` and `dgrReports`, with the scripts they load.
  Every AJAX call in those pages and scripts was traced to its URL, HTTP method and parameters.
- Each JSON service was then called once or twice with the parameters the page sends (sequential requests at least 1.5 s apart, browser User-Agent, no cookies or login), on 2026-11-16 (IST).
  The response shapes below come from those calls; `api/fixtures/` keeps one recorded response per service in the collection.
- All dashboard services are plain `GET` requests with query parameters (jQuery `$.ajax` with a `data` object), answer `application/json`, and need no authentication.
  Paths are relative to `https://npp.gov.in/dashBoard/` unless a full URL is shown.

Ids used as parameters (from `getBMapData`):
zones (regions) `100001` Northern, `100002` Western, `100003` Southern, `100004` Eastern, `100005` North Eastern, `100006` Bhutan import;
states use the `state_id` values of `getBMapData.stateList` (for example `27` Maharashtra, `29` Karnataka, `9` Uttar Pradesh).

## What the competition rules allow

NPP is one of the two challenge datasets for the core 2047 prediction (the other is the organisers' Renewable India dataset).
Within NPP, the project uses **generation** only.
Installed capacity must not be used ("does not guarantee energy", kickoff 13 Nov 2026), and consumption or demand series are not used as forecasting inputs (`docs/decisions.md`, `docs/methodology.md`).
The last column below applies that rule to each service.

## Dashboard services

| Service | Parameters | What it returns | Kind | May the project use it? | In collection |
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
| `res/getWbDbData` | `selectedDate` (epoch ms) | HTTP 404 on 2026-11-15 and 2026-11-16 (map cache of an older page script, `web-db.js`) | Site plumbing | Not applicable | No |

## Site plumbing and third-party calls (documented, never called)

| Call | Made by | Purpose | Handling |
| --- | --- | --- | --- |
| `https://npp.gov.in/public/nppVisitCount` (GET, no parameters) | landing page on load | visitor counter shown on the home page | Not called by this project, because a request may count as a visit |
| `https://npdms.gov.in/api2/userLog?pageUrl=...&pageName=...` (GET) | dashboard, Merit and report pages | page-view logging on a separate NPP domain | **Never called**; not in the collection |
| `services.arcgisonline.com`, `js.arcgis.com` | the map dashboards | third-party basemap tiles and the ArcGIS JavaScript API | Not called; not an NPP data service |

## Report files

Static files linked from `publishedReports` and `dgrReports`, each as `.xls` and `.pdf`, under `https://npp.gov.in/public-reports/cea/`:

| Path pattern | Content | Kind | Use |
| --- | --- | --- | --- |
| `monthly/generation/18_col_act/{YYYY}/{MON}/18_col_act-{N}_{YYYY}-{MON}.xls` | Monthly generation, actuals, 20 sub-reports (Sub-report 2 region wise, 16 station wise); Jul-2019 to Sep-2026 at access | Generation | **Used** (regional and state files in `data/raw/npp/`); Sub-report 2 is in the collection (HEAD) |
| `monthly/generation/18_col_ten/{YYYY}/{MON}/18_col_ten-{N}_{YYYY}-{MON}.xls` | Same, tentative figures (latest: Oct-2026) | Generation | Allowed, not used |
| `daily/dgr/{DD-MM-YYYY}/dgr{N}-{YYYY-MM-DD}.xls` | Daily generation reports 1 to 17, conventional stations | Generation | Allowed, not used. Archive depth checked with HEAD requests on `dgr2`: 2018-04-01 and 2019-04-01 answer 200; 2017-04-01, 2017-10-01, 2016-04-01 and 2013-04-01 answer 404 |
| `daily/fuel/{DD-MM-YYYY}/dailyCoal1-{YYYY-MM-DD}.xls`, `monthly/fuel/{YYYY}/{MON}/monthCoal1-…`, `monthImportCoal1-…`, `monthlyGasSupply-…`, `monthly/fuel/tent/…/dailyCummCoal-…` | Coal stock, coal and gas supply | Fuel | Not used |
| `monthly/installcap/{YYYY}/{MON}/capacity1-{YYYY}-{MM}.xls`, `capacity2-{Region}-{YYYY}-{MM}.xls` | Installed capacity, All India and by region | Capacity | **No** |
| `monthly/transmission/{tc,tl}/{YYYY}/{MON}/trans{N}-{YYYY}-{MM}.xls` | Transformation capacity and transmission lines | Transmission | Not used |

## State-wise daily generation: is there a history?

No, not from the dashboard services.

- `getZoneStateWiseGen` ignores `genDate`: requests with `13-Nov-2026`, `13-Nov-2025` and `01-Apr-2020` returned byte-identical bodies, all for the latest reporting day (12 Nov 2026 at access).
  `getAllZoneStateGen` behaves the same with an added `genDate`.
- Each response holds one day, plus the same day last year and the totals since 1 April for this year and last year.
  That is enough for a year-to-date figure per state or station, not for a daily series.
- A state-wise daily history would have to be built by saving the latest day every day from now on, or read from the daily generation report files (`dgr`), which go back to April 2018.
  Both cover conventional stations above 25 MW only (no solar or wind), and the project already has state-wise annual generation from Sub-report 16 (`npp_state_generation_fy.csv`).
- The collection's test "genDate ... is still ignored" fails if NPP starts honouring `genDate`, so the weekly monitor would show that a history has become available.
