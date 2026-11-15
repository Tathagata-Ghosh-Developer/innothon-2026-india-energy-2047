# National Power Portal (NPP) API collection

`npp.postman_collection.json` is a Postman collection (format v2.1) of the public NPP endpoints this project reads or watches, with tests for each.
It runs with Newman, Postman's open-source command-line runner; no Postman account is needed.

NPP is one of the two challenge datasets the organisers allow for the core 2047 prediction (the other is the organisers' Renewable India dataset).
Within NPP the project uses generation only; installed capacity and consumption or demand never enter the forecast.
`NPP_SERVICES.md` lists every service found behind the NPP pages and applies that rule to each; `data/raw/npp/SOURCES.md` describes the extracted data.

## Files

| Path | Content |
| --- | --- |
| `npp.postman_collection.json` | The collection: 9 requests with tests, collection variables, a pre-request script for pacing and the Merit date |
| `npp-mock.postman_environment.json` | Environment for the offline run: points `baseUrl` at the local mock and fixes the Merit date to the recorded day |
| `fixtures/` | One recorded real response per request, served by the mock (see "Offline run") |
| `mock/server.js` | Static mock of the endpoints (Node.js 18 or later, no dependencies) |
| `NPP_SERVICES.md` | Inventory of all NPP services, their parameters and whether the project may use them |

## Requests

All requests are `GET` (the report file uses `HEAD`), need no login, and send a browser User-Agent.
Base URL `https://npp.gov.in`.

| Request | What it returns | Project files it feeds |
| --- | --- | --- |
| `dashBoard/get_generation_chart_list` | All-India generation by source per fiscal year, GWh, FY1946-47 to FY2023-24 (`linechartforGeneration`: `financial_year, thermal_total, hydro, nuclear, renewable_energy_sources`) | `data/raw/npp/npp_national_generation_fy.csv` (raw copy in `data/raw/npp/downloads/npp_dashboard/`), read by the India-only forecast core (`src/pipeline/pipeline.cpp`, `src/forecast/india.cpp`) |
| `dashBoard/getAllZoneGen?ID=IND` | Latest reporting day, All India: programme and actual conventional generation (MU), same day last year, year to date; also coal stock and plant load factor | None (no history) |
| `dashBoard/getAllZoneStateGen?zoneId={{zoneId}}` | Latest day, one row per state of a region: programme and actual generation (MU), same day last year, year to date | None (no history, see `NPP_SERVICES.md`) |
| `dashBoard/getZoneStateWiseGen?stateId={{stateId}}&zoneId={{zoneId}}&genDate={{genDate}}` | Latest day, one row per station of the region, and the state total; `genDate` is ignored by the server | None (no history) |
| `dashBoard/getAllZone` | Installed capacity by sector and by source (MW) and peak demand met | None. Capacity and demand are excluded by the rules; the request only watches the service |
| `dashBoard/getBMapData?ID=IND` | Map lists (stations, states and zones with ids, districts, projects), about 0.75 MB; only status, type and time are tested | None (state and zone ids for the requests above) |
| `dashBoard/demandmet2chartdata?date={{date}}` | Merit real-time generation snapshots (MW) for thermal, gas, hydro, nuclear, solar and wind, one IST day | `data/raw/npp/downloads/merit/merit_generation_snapshots_*.csv.gz`, then `npp_merit_generation_hourly.csv` (read by `src/allocation/storage.cpp`), `npp_merit_generation_daily.csv`, `npp_res_generation_monthly.csv` |
| `dashBoard/demandmet1chartdata?date={{date}}` | Merit real-time All-India demand met (MW) | Raw only: `data/raw/npp/downloads/merit/merit_demandmet_snapshots_*.csv.gz` (demand is not a forecasting input) |
| `HEAD public-reports/cea/monthly/generation/18_col_act/{{yyyy}}/{{MON}}/18_col_act-2_{{yyyy}}-{{MON}}.xls` | CEA monthly 18-column report, Sub-report 2 (region wise); the test checks status, `application/vnd.ms-excel` and size | `data/raw/npp/downloads/cea_18col_sr2/`, then `npp_region_generation_monthly.csv` and `npp_region_generation_fy.csv` (read by `apps/energy2047.cpp`); Sub-report 16 (same pattern, `-16_`) feeds `npp_state_generation_fy.csv` |

## Tests

Every request checks HTTP 200, the content type and the response time (30 s, 60 s for the map data).
The JSON requests also check:

- **Schemas** (`pm.response.to.have.jsonSchema`): required fields and types, for example `linechartforGeneration` entries with `financial_year` (`YYYY-YY`), `hydro`, `thermal_total`, `renewable_energy_sources`, `nuclear`, and Merit items with integer `updated_on`, `name_of_data` from the documented names, numeric `value_of_data`.
- **Content**: the chart starts in FY1946-47, is annual from FY2006-07 and still ends in FY2023-24 (`chartLatestFy`); its FY2023-24 values equal the committed CSV; all six Merit sources are present, every timestamp falls inside the requested IST day and values are in MW; capacities add up across sectors and sources; state and station rows add up to the published totals.
- **Freshness**: the daily figures are at most `maxStaleDays` (30) days old.
- **Behaviour the project relies on**: `getZoneStateWiseGen` still ignores `genDate` (a failure would mean NPP now serves past days there).

A failure in the weekly monitor therefore means something worth a look: a changed field, a new or missing source, a revised FY2023-24 value, a new fiscal year in the chart (the archived CSV can then be extended), stale daily data, or an endpoint that stopped answering.

## Variables

Collection variables; override any of them with `--env-var name=value` (or an environment file).

| Variable | Default | Meaning |
| --- | --- | --- |
| `baseUrl` | `https://npp.gov.in` | Server |
| `userAgent` | a desktop Chrome string | Sent with every request |
| `delayMs` | `2000` | Pause before each request (polite pacing) |
| `date` | `auto` | Merit day `YYYY-MM-DD`; `auto` = yesterday in IST |
| `meritMaxBackoffDays` | `3` | With `auto`, earlier days tried when a day has no Merit data (NPP has gap days, listed in `SOURCES.md`) |
| `zoneId`, `stateId`, `genDate` | `100002`, `27`, `01-Apr-2025` | Western region, Maharashtra, and the day sent as `genDate` |
| `yyyy`, `MON` | `2026`, `SEP` | Month of the report file (`MON` = `JAN`..`DEC`; earliest Jul-2019) |
| `chartLatestFy` | `2023-24` | Last fiscal year expected in the chart |
| `maxResponseMs`, `maxResponseMsLarge` | `30000`, `60000` | Response time limits |
| `maxStaleDays` | `30` | Maximum age of the daily figures |

## How to run

Live, against npp.gov.in (9 requests, about 25 s with the pacing):

```sh
npx --yes newman run api/npp.postman_collection.json
npx --yes newman run api/npp.postman_collection.json --env-var date=2026-11-14   # a given Merit day
npx --yes newman run api/npp.postman_collection.json --folder "Merit real-time"  # one folder
```

The collection also opens in the Postman app (Import, then the file).

### Offline run

`mock/server.js` serves the responses in `fixtures/` on `http://127.0.0.1:3647`; the environment file points the collection at it.
For the Merit endpoints it serves the recorded day and answers `[]` for any other day, as NPP does for days without data.
The report file answers `HEAD` only, with the recorded headers.

```sh
node api/mock/server.js &
npx --yes newman run api/npp.postman_collection.json -e api/npp-mock.postman_environment.json
```

Fixtures (recorded on 2026-11-16 IST, between 04:00 and 04:40):

| Fixture | Request | Note |
| --- | --- | --- |
| `fixtures/dashBoard/get_generation_chart_list.json` | `get_generation_chart_list` | identical to `data/raw/npp/downloads/npp_dashboard/get_generation_chart_list_2026-11-15.json` |
| `fixtures/dashBoard/getAllZoneGen.json` | `getAllZoneGen?ID=IND` | |
| `fixtures/dashBoard/getAllZoneStateGen.json` | `getAllZoneStateGen?zoneId=100002` | |
| `fixtures/dashBoard/getZoneStateWiseGen.json` | `getZoneStateWiseGen?stateId=27&zoneId=100002` | |
| `fixtures/dashBoard/getAllZone.json` | `getAllZone` | |
| `fixtures/dashBoard/getBMapData.json` | `getBMapData?ID=IND` | **trimmed**: every list cut to its first 3 entries (0.75 MB to 12 kB); the test only checks status and type |
| `fixtures/dashBoard/demandmet2chartdata/2026-11-14.json` | `demandmet2chartdata?date=2026-11-14` | full day, 2,358 snapshots |
| `fixtures/dashBoard/demandmet1chartdata/2026-11-14.json` | `demandmet1chartdata?date=2026-11-14` | full day, 393 snapshots |
| `fixtures/public-reports/.../18_col_act-2_2026-SEP.xls.headers.json` | `HEAD` of the Sep-2026 Sub-report 2 file | status and headers only |

To refresh a fixture, save the live response under the same path (for example `curl -A "Mozilla/5.0" -o api/fixtures/dashBoard/getAllZone.json https://npp.gov.in/dashBoard/getAllZone`) and run the offline check again.
The Postman CLI can also build a local mock (`postman mock generate`, no account needed), but only from examples stored inside the collection; the plain mock keeps the collection small and the fixtures as separate files.

## Continuous integration

- `.github/workflows/ci.yml`, job `api-offline`: on every push and pull request to `main`, runs the collection against the mock. It sends no request to NPP, so it cannot fail because NPP is slow or down.
- `.github/workflows/npp-api-monitor.yml`: every Monday at 03:30 UTC (09:00 IST), and on demand (Actions tab, "Run workflow", optional Merit date), runs the collection against the live site and uploads an HTML and JUnit report (artifact `npp-api-report`). It is separate from CI and never blocks it.
  If GitHub's runners cannot reach npp.gov.in (some government sites filter traffic from abroad), the monitor fails with connection errors rather than test failures; the offline job is unaffected.

A Postman cloud monitor is optional and not set up here.
It needs the owner's Postman account: import the collection in the Postman app, then
`npx --yes postman-cli login` and
`npx --yes postman-cli monitor create -c <collection-uid> --schedule "30 3 * * 1" --timezone Asia/Kolkata --delay 2000`.
Check the monitoring quota of the free plan first; a run uses 9 to 12 requests.

## Last local run

Live run on 2026-11-15 23:01 UTC (2026-11-16 04:31 IST), Newman 6.2.3, Windows, Merit date `auto` (2026-11-15):

```text
┌─────────────────────────┬─────────────────────┬────────────────────┐
│                         │            executed │             failed │
├─────────────────────────┼─────────────────────┼────────────────────┤
│              iterations │                   1 │                  0 │
├─────────────────────────┼─────────────────────┼────────────────────┤
│                requests │                   9 │                  0 │
├─────────────────────────┼─────────────────────┼────────────────────┤
│            test-scripts │                   9 │                  0 │
├─────────────────────────┼─────────────────────┼────────────────────┤
│      prerequest-scripts │                   9 │                  0 │
├─────────────────────────┼─────────────────────┼────────────────────┤
│              assertions │                  61 │                  0 │
├─────────────────────────┴─────────────────────┴────────────────────┤
│ total run duration: 24.1s                                          │
├────────────────────────────────────────────────────────────────────┤
│ total data received: 1.12MB (approx)                               │
├────────────────────────────────────────────────────────────────────┤
│ average response time: 584ms [min: 48ms, max: 1400ms, s.d.: 347ms] │
└────────────────────────────────────────────────────────────────────┘
```

The offline run against the mock gave the same counts (9 requests, 61 assertions, 0 failed) in under a second.
