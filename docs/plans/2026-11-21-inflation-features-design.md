# Inflation and the full set of demand features: design

Date: 21 November 2026. Written after the final presentation.

## Why

There are two goals.
- **Inflation.** Every money figure must sit on one stated real price base, converted with official price indices.
- **Demand drivers.** Every factor that can change India's 2047 electricity generation requirement must be listed and quantified from official sources, and the scenario set must span all of them.

## What exists today

- **Forecast (Part 1).** It is estimated on real income per person, using the organiser dataset's GSDP at constant prices (2004-05 base up to 2010, 2011-12 base from 2011). Inflation is therefore already removed from the income driver, but the docs do not state this explicitly.
  - Drivers in the forecast: population, urbanisation, a persistent model error and a COVID-year dummy.
  - Drivers available as settings: an elasticity decline and a new-loads add-on.
- **Mix costs (Part 2).**
  - The default costs are IEA WEO 2025 projections, already in real 2024 USD.
  - The CEA cost basis mixes price bases. It uses real 2021-22 INR capex, a nominal CERC WACC (12.585%), FY2026-27 fuel and biomass prices, and the FY2021-22 RBI exchange rate. This is disclosed as a caveat (D13, R16).
- **New loads.** `data/external/new_loads.csv` adds green hydrogen only (no overlap with history). EVs, data centres, cooling and cooking are reported but never added.

## Rules that do not change

- **Headline.** The 2047 requirement stays estimated on the two challenge datasets. Every new feature is a labelled scenario layer, never the headline.
- **Sources.** Every new figure comes from an official source: a ministry, a statutory body (CEA, BEE, RBI, MoSPI), NITI Aayog, or a national statistics office (US BLS/BEA for USD price indices). Each has a URL, a page or table, and an access date, and is labelled secondary.
- **Metrics.** Installed capacity and consumption are never forecast metrics. Capacity figures may appear only after an explicit CUF conversion to TWh, as a feature size.

## Design

### A. Inflation: one real price base
1. **Price indices.** New file `data/external/macro/price_indices.csv`. All are official:
   - India CPI (MoSPI) and WPI (Office of the Economic Adviser), annual;
   - India GDP deflator (MoSPI National Accounts);
   - RBI INR/USD reference rate, annual averages;
   - the RBI inflation target (4%, tolerance band 2-6%);
   - US CPI-U (BLS) and the US GDP deflator (BEA).
2. **Price base.** Real 2024 USD, matching IEA WEO 2025. A `price_base` module converts any cost from (currency, price year) to real 2024 USD:
   - first deflate within its own currency with the GDP deflator (WPI as a sensitivity);
   - then convert at the 2024 RBI rate.
3. **Nominal to real WACC.** Use the Fisher relation, `r_real = (1 + r_nom) / (1 + pi) - 1`, with pi equal to the RBI target. The target's band gives the sensitivity rows (pi = 2%, 4%, 6%).
4. **What changes.** The CEA cost basis moves fully onto real 2024 USD, which removes the D13 caveat. The default IEA plan does not change, so the headline mix numbers stay as presented.
5. **Nominal outlook.** Add an information row to the outputs: the 2047 annual system cost in nominal 2047 INR and USD at the RBI target inflation, clearly labelled.

### B. An exhaustive catalogue of demand features
1. **Catalogue.** New file `docs/demand_features.md`. It lists every driver, about 35, grouped as demographic, economic, structural, prices and inflation, technology and efficiency, electrification and new loads, climate and weather, supply-side and system effects, distributed and behind-the-meter generation, trade, policy, and shocks.
   - For each driver it states the mechanism, the sign of the effect, how the model represents it (estimated core / driver uncertainty / setting / feature layer / excluded by rule, with the reason), the official source and the quantified size.
2. **Feature data.** New file `data/external/demand_features.csv`. It holds every quantifiable feature as a signed TWh adjustment to 2047 utility generation (low, central, high) with:
   - an overlap flag (yes / partial / no) against the historical trend;
   - the scaling used, for example 2050 to 2047;
   - the source.
3. **Feature layer** (C++). It replaces the new-loads-only add-on:
   - signed adjustments;
   - one-at-a-time effects;
   - named scenarios: Core (headline), All features central, Electrification high, Efficiency and distributed generation, Climate stress, All upside, All downside;
   - a joint Monte Carlo over all feature ranges (triangular, independent), giving a "core + features" distribution.
   - Overlap rule: loads already inside the historical relationship are added only for their part beyond the trend. That part is stated per feature; the default for a `partial` feature is 0 unless an official figure separates the increment.
4. **Outputs.** New `outputs/feature_scenarios_2047.csv`, plus `site_summary.json` keys, a dashboard view and a landing-page section with the catalogue and the scenario envelope.

### C. Coverage of all scenarios
- **Existing scenarios:** the growth presets (3) × the policy presets (4).
- **New feature scenarios:** the named set above, plus the joint distribution.
- **The envelope:** All downside to All upside bounds every combination of feature levels.
- **The P90 plan test** extends to the "core + features" P90.

## Order
1. Research, in parallel: (a) the price indices; (b) the feature data.
2. Implementation, test first: the price base and the feature layer.
3. Outputs, docs, landing page and dashboard.
4. Review.
5. Push, deploy and verify live.
