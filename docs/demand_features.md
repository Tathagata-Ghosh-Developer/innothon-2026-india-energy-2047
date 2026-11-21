# Demand features: every driver of the 2047 requirement

This document lists every factor that can change India's generation requirement in FY2047-48.
For each driver it gives the mechanism and the sign of its effect, how the model treats it, its official source, and its size in TWh or the reason it has no size.
It then gives the named scenarios, the joint distribution, how the scenarios cover every combination, and the overlap rules.

- **The headline does not change.** It is the two-dataset forecast: a median of 4,688 TWh in FY2047-48 (P5 3,190, P95 7,021), estimated on NPP generation and the Renewable India dataset's population and GSDP. These two are the only primary sources.
- **The features are a labelled layer outside the headline.** Every feature size comes from an official source (a ministry, a statutory body such as CEA, BEE or RBI, NITI Aayog, or a national statistics office) and is labelled secondary.
- **Generation only.** Every size is a change in FY2047-48 utility generation (NPP scope), in TWh. Where a source gives capacity (GW) or end-use electricity, the row converts it explicitly to generation with a stated capacity factor and loss gross-up. Installed capacity and consumption are never forecast metrics.
- Years are fiscal years labelled by their start year: 2023 is FY2023-24 and 2047 is FY2047-48.

The data are `data/external/demand_features.csv` (one row per driver, with URLs, pages and access dates; `data/external/SOURCES.md`, section 11).
The model's outputs are `outputs/demand_features_catalogue.csv`, `outputs/feature_scenarios_2047.csv` and the `features` block of `outputs/site_summary.json`.
The decision record is [decisions.md](decisions.md), D14.

## 1. How a driver is treated

Each driver gets one of five treatments (column `in_model`).

| Treatment | Count | Meaning | Why |
| --- | --- | --- | --- |
| Core | 7 | Estimated in the headline forecast | The driver is in the two challenge datasets, or its effect is already in the 2006-2024 history the elasticity is estimated on |
| Driver | 3 | An existing uncertainty or setting of the core forecast | The forecast already carries it as a prior, a setting or the model error |
| Layer | 11 | A signed TWh adjustment, outside the headline | An official source sizes its effect beyond the historical trend |
| Excluded by rule | 1 | Not estimated or reported | The organisers' written ruling of 16 November 2026 excludes it (captive generation) |
| Context | 22 | Listed, not quantified | No official figure exists, the effect is inside the trend or inside another row, or it changes the supply mix, not the quantity; each entry says which |

**Conventions of the layer rows.**

- **Sign.** +1 raises utility generation; -1 lowers it. The layer applies sign x size.
- **Low, central, high.** Low and high are the smallest and largest official cases for 2047 after the rules below. Central is the current-policy case where one exists (NITI Aayog's Current Policy Scenario, the National Green Hydrogen Mission, scheme targets), otherwise the figure dated 2047. Where a row has only one official anchor, central equals low and the high case is a stated continuation.
- **Beyond the trend only.** The core forecast already carries every existing load growing with total generation. Only the part beyond that is a feature (section 7).
- **Year conversion.** A 2050 value is scaled linearly from the FY2023-24 base: level 2047 = base + (level 2050 - base) x 24/27. A value for an earlier year is held flat to 2047 unless the entry states a continuation.
- **Gross-up to generation** (CEA General Review 2025, FY2023-24): loads at low tension x1.2936 = 1 / ((1 - 0.1763)(1 - 0.0615)); high-tension loads x1.1029 = 1 / ((1 - 0.0339)(1 - 0.0615)); energy at the bus bar x1.0655 = 1 / (1 - 0.0615).
- **Overlap flag** (`overlap_history`): yes, partial or no overlap with the FY2006-07 to FY2023-24 trend.

## 2. All 44 drivers at a glance

Sizes are FY2047-48 TWh as the layer applies them (low to high, central in brackets); "-" means not quantified.

| # | Driver | Group | Sign | Treatment | Size, TWh |
| --- | --- | --- | --- | --- | --- |
| 1 | Population | Demographic | + | Core | - |
| 2 | Urbanisation | Demographic | + | Driver | - |
| 3 | Income (real GDP) | Economic | + | Core | - |
| 4 | Income elasticity decline | Economic | - | Driver | - |
| 5 | Sectoral structure (GVA) | Economic | - | Context | - |
| 6 | T&D losses | Supply side and system | - | Layer (upper bound) | 0 to 476.3 (0) |
| 7 | Auxiliary consumption | Supply side and system | - | Layer | 91.6 to 110.5 (91.6) |
| 8 | Storage round-trip losses | Supply side and system | + | Layer | 72.2 to 150.1 (150.1) |
| 9 | Cross-border imports | Supply side and system | - | Layer | 10.9 to 15.3 (13.1) |
| 10 | Cross-border exports | Supply side and system | + | Context | - |
| 11 | Rooftop solar | Distributed and behind-the-meter | - | Layer | 24.7 to 134.9 (24.7) |
| 12 | PM-KUSUM C pump solarisation | Distributed and behind-the-meter | - | Layer | 13.0 to 38.6 (37.7) |
| 13 | PM-KUSUM A decentralised plants | Distributed and behind-the-meter | - | Context | - |
| 14 | PM-KUSUM B standalone pumps | Distributed and behind-the-meter | - | Context | - |
| 15 | Captive generation | Distributed and behind-the-meter | + | Excluded by rule | - |
| 16 | UJALA LED lighting | Efficiency | - | Core | - |
| 17 | Standards and labelling | Efficiency | - | Core | - |
| 18 | PAT scheme | Efficiency | - | Core | - |
| 19 | ECBC buildings and others | Efficiency | - | Core | - |
| 20 | Efficiency beyond trend | Efficiency | - | Layer | 0 to 125.3 (0) |
| 21 | National efficiency targets | Efficiency | - | Context | - |
| 22 | Transport electrification | Electrification and new loads | + | Layer | 511.9 to 525.2 (511.9) |
| 23 | Green hydrogen | Electrification and new loads | + | Layer | 137.9 to 682.3 (230.4) |
| 24 | Data centres | Electrification and new loads | + | Layer | 356.2 to 397.1 (397.1) |
| 25 | Space cooling | Electrification and new loads | + | Layer | 0 to 252.3 (0) |
| 26 | Electric cooking | Electrification and new loads | + | Context | - |
| 27 | Railway traction | Electrification and new loads | + | Context | - |
| 28 | Metro rail | Electrification and new loads | + | Context | - |
| 29 | Desalination | Electrification and new loads | + | Context | - |
| 30 | Industrial electrification (green steel) | Electrification and new loads | + | Context | - |
| 31 | Agriculture pumping | Electrification and new loads | + | Context | - |
| 32 | Cold chain refrigeration | Electrification and new loads | + | Context | - |
| 33 | Warming (cooling degree days) | Climate and weather | + | Context | - |
| 34 | Monsoon hydro variability | Climate and weather | + | Context | - |
| 35 | Extreme heat year | Shocks | + | Driver | - |
| 36 | Drought | Shocks | + | Context | - |
| 37 | Pandemic | Shocks | - | Core | - |
| 38 | Electricity tariffs | Prices and policy | - | Context | - |
| 39 | Price elasticity | Prices and policy | - | Context | - |
| 40 | Carbon market (CCTS) | Prices and policy | - | Context | - |
| 41 | Net zero 2070 | Prices and policy | + | Context | - |
| 42 | RPO and RCO trajectory | Prices and policy | + | Context | - |
| 43 | Trend carry-over (losses, deficit) | Historical trend effects | + | Context | - |
| 44 | Household access and supply hours | Historical trend effects | + | Context | - |

## 3. The catalogue, by group

Every source was accessed on 21 November 2026; the CSV gives each URL.
Numbers of the core forecast (elasticity, log points, sensitivity rows) are from the current run (`outputs/`).

### 3.1 Demographic

**1. Population** (`population`). Sign +. Core.

- Mechanism: more people raise total generation (generation per person times population).
- Treatment: estimated in the core forecast. India's path is UN WPP 2024 medium (1,666.6 million in 2047), checked against the challenge dataset's state growth (`outputs/population_consistency.csv`). Population contributes 0.148 log points (14.8 percent) of the central path's growth to FY2047-48.
- Source: Technical Group on Population Projections (NCP, MoHFW), Population Projections for India and States 2011-2036, Tables 8-13 (the Indian official reference, to 2036); UN DESA, World Population Prospects 2024 (intergovernmental, allowed by the organisers' ruling).
- Size: inside the headline, not a feature.

**2. Urbanisation** (`urbanisation`). Sign +. Driver.

- Mechanism: urbanisation beyond what income implies raises electricity per person (urban households use more appliances).
- Treatment: urbanisation that came with income is inside the elasticity. Only the excess enters, with gamma ~ N(1.0, 0.5) truncated at zero: 0.048 log points (4.8 percent) of the central path's growth. Without it (gamma = 0) the median is 4,438 TWh; with the urban share 5 points above the UN path it is 4,938 TWh.
- Source: UN DESA, World Urbanization Prospects 2018, File 2, India row; Technical Group, Table 10 (urban share 31.14 percent in 2011 to 39.58 percent in 2036).
- Size: inside the core forecast's uncertainty, not a feature.

### 3.2 Economic

**3. Income (real GDP)** (`income_real_gdp`). Sign +. Core.

- Mechanism: real income per person drives generation per person, with India's own elasticity (0.835, s.e. 0.065, FY2011-12 to FY2023-24).
- Treatment: estimated in the core forecast from the organiser GSDP (Jammu and Kashmir deflated from current prices, D14). Mean growth of income per person is 4.90 percent a year (ln). Income contributes 0.870 log points (87.5 percent) of the central path's growth. The growth presets move income growth by 1.5 points a year: 6,163 TWh above history, 3,568 TWh below.
- Source: the benchmark is official. Ministry of Finance, Economic Survey 2024-25, Chapter 5, para 5.1 (PDF p. 1): growth of around 8 percent at constant prices for a decade or two to reach Viksit Bharat. That is total GDP, not GDP per person, and the growth presets span it. RBI, Handbook of Statistics on the Indian Economy 2025-26, Table 2.
- Size: inside the headline, not a feature.

**4. Income elasticity decline** (`income_elasticity_decline`). Sign -. Driver.

- Mechanism: the income elasticity may fall as India grows richer, as some end uses saturate.
- Treatment: an existing setting. A decline of 0.2 per unit of ln income gives a median of 4,194 TWh (-10.5 percent). The bootstrap carries the elasticity's own uncertainty (90 percent interval 0.737 to 0.947); one OLS standard error either way moves the median by -6.5 or +7.0 percent.
- Source: no official Indian estimate of a falling elasticity was found. The setting and its effect are the model's (`outputs/sensitivity_2047.csv`).
- Size: not a feature.

**5. Sectoral structure (GVA)** (`sectoral_structure_gva`). Sign -. Context.

- Mechanism: a shift of value added from industry to services lowers electricity per unit of GDP.
- Treatment: context. Structural change over 2006-2024 is inside the elasticity, and no official projection of electricity intensity by sector to 2047 was found.
- Source: MoSPI (NSO), press note on the Provisional Estimates of Annual GDP for 2024-25 (30 May 2025), Statement 4, PDF p. 12: in FY2023-24 the secondary sector was 25.9 percent of GVA at current prices, the tertiary 54.4 percent and the primary 19.7 percent.
- Size: not quantified.

### 3.3 Supply side and system

**6. T&D losses** (`t_and_d_losses`). Sign -. Layer, as an upper bound.

- Mechanism: lower transmission and distribution losses reduce the generation needed for the same use; generation scales with 1 / (1 - loss).
- Treatment: an upper bound that may already be inside the trend (ruling R22). The fall from 28.65 percent in 2006-07 to 17.63 percent in 2023-24 is inside the estimated elasticity (about 0.144 log points of the history), so the trend already implies further falls. The row counts 0 at low and central, its high value only in the downside scenarios (All downside, Efficiency and distributed), and -uniform(0, high) in the joint draws.
- Source: CEA, All India Electricity Statistics: General Review 2025, Table 6.1, PDF p. 103 (17.63 percent in FY2023-24); CEA, 20th Electric Power Survey, Vol. I, Table 4 (p. 9) and Table 6.1 (p. 167): 9.36 percent in 2041-42, ex-bus; NITI Aayog (Feb 2026), Sectoral Insights: Power, Sec. 4.3, p. 74 (about 12 percent by 2030 and 8 percent by 2050); CEA, Growth of Electricity Sector in India 1947-2025, Chart 29, p. 58 (history).
- Size: listed 386.2 / 464.6 / 476.3 TWh (low: the EPS path held at 9.36 percent after 2041-42; central: the NITI path, 8.60 percent in 2047; high: the EPS path extended to 7.43 percent). Effect = 4,703 x (1 - (1 - L2023) / (1 - L2047)), with 4,703 TWh the 20 Nov headline (section 7). As applied: 0 to 476.3 TWh, central 0.

**7. Auxiliary consumption** (`auxiliary_consumption`). Sign -. Layer.

- Mechanism: a mix with more solar and wind (auxiliary use about 2 percent) and less coal (about 7.4 percent) needs less station auxiliary energy per unit sent out.
- Treatment: applied as given. It depends on the Part 2 mix: the Robust plan (about 80 percent non-fossil) would give a larger fall than NITI's mixes.
- Source: CEA General Review 2025, Table 6.1 (p. 103) and Tables 6.5.0-6.5.6 (pp. 107-113): FY2023-24 auxiliary rates of 6.15 percent overall, coal 7.38, gas 4.18, nuclear 8.78, hydro 0.85 and renewables 1.94 percent; NITI Aayog, Sectoral Insights: Power, Table 4.2, p. 79 (2050 generation mix).
- Size: 91.6 / 91.6 / 110.5 TWh. The per-source rates of FY2023-24 on NITI's 2050 mixes give an auxiliary share of 4.28 percent (Current Policy) or 3.89 percent (Net Zero) in 2047; low = central = Current Policy, high = Net Zero.

**8. Storage round-trip losses** (`storage_round_trip_losses`). Sign +. Layer.

- Mechanism: energy lost in battery and pumped-storage cycles must be generated on top of demand.
- Treatment: applied as given; all of it is beyond the trend, because storage losses were negligible in FY2023-24. Do not add it on top of the Part 2 storage back-calculation, which reports its own losses (section 7).
- Source: Ministry of Power, National Framework for Promoting Energy Storage Systems (Aug 2023), para 3.3, p. 6 (CEA's 2047 storage need: 1,840 GWh of batteries and 540 GWh of pumped storage); CEA, National Generation Adequacy Plan 2026-27 to 2035-36 (19 Mar 2026), Fig. 4.5, p. 43 (2035-36: 321 GWh of batteries, 567 GWh of pumped storage) and Annexure-B, p. 71 (round trip 88 percent for batteries, 80 percent for pumped storage); CEA, NEP 2022-32, Sec. 13.5, p. 314 (batteries run one cycle a day).
- Size: 72.2 / 150.1 / 150.1 TWh. Loss = 365 cycles x [battery GWh x (1/0.88 - 1) + pumped-storage GWh x (1/0.80 - 1)] x 1.0655. Low holds the 2035-36 storage to 2047; central = high is the 2047 need (no official upper case). Partial cycling would lower it.

**9. Cross-border imports** (`cross_border_imports`). Sign -. Layer.

- Mechanism: hydro imports from Bhutan and Nepal replace domestic generation; NPP generation excludes imports.
- Treatment: applied as given, as the change from FY2023-24 imports (6.15 TWh).
- Source: CEA General Review 2025, Table 6.1, p. 103 (imports 6,147.23 GWh); CEA, NEP 2022-32, Table 5.5, p. 137 (5,856 MW of hydro imports by 2032); NITI Aayog, Sectoral Insights: Power, p. 66 (large-hydro PLF 32 to 40 percent).
- Size: 10.9 / 13.1 / 15.3 TWh. 2047 imports = 5,856 MW x PLF (32, 36, 40 percent) x 8.76 = 16.4 / 18.5 / 20.5 TWh, less 6.15 TWh, x 1.0655. The India-Nepal long-term power trade agreement is not included: the official brief records it without a quantity.

**10. Cross-border exports** (`cross_border_exports`). Sign +. Context.

- Mechanism: power exported to Bangladesh, Nepal, Bhutan and Myanmar is generated in India and raises NPP generation.
- Treatment: context. No official projection of exports to 2047 was found, and today's exports are inside the history.
- Source: CEA General Review 2025, Table 6.1, p. 103 (exports 11,295.19 GWh) and Table 6.2, p. 104 (Bangladesh 8.53, Nepal 1.42, Bhutan 1.33, Myanmar 0.01 TWh).
- Size: not quantified. At today's level it is 0.2 percent of the 2047 median.

### 3.4 Distributed and behind-the-meter generation

**11. Rooftop solar** (`rooftop_solar`). Sign -. Layer.

- Mechanism: rooftop output is used on site and lowers the demand that utilities must meet; the NEP excludes it from the utility requirement.
- Treatment: applied as given; only the offset beyond today's, which the trend carries, counts. It assumes NPP utility generation excludes rooftop output.
- Source: CEA, 20th EPS, Sec. 1.5.2, p. 27 (50 GW by 2031-32, CUF 17 percent, 75 percent used on site, 55 BU offset); CEA, NEP 2022-32, p. 133 (rooftop 17.6 BU in 2026-27 and 55.8 BU in 2031-32, outside utility demand); MNRE, Renewable Energy Statistics 2023-24 (p. 27) and 2024-25 (p. 28); MNRE, Guidelines for PM-Surya Ghar: Muft Bijli Yojana (16.04.2024), pp. 5-6.
- Size: 24.7 / 24.7 / 134.9 TWh. Offset = GW x 8.76 x 0.17 x 0.75 (the EPS formula). FY2023-24 rooftop gives 13.3 TWh, which the trend carries to 36.7 TWh. Low = central: the EPS 2031-32 level held to 2047 (55.8 TWh). High: the official 2023-24 to 2031-32 path continued linearly to 2047 (141.0 TWh), a stated extrapolation. Each less the trend, x 1.2936. PM-Surya Ghar sits inside the EPS path.

**12. PM-KUSUM C pump solarisation** (`pm_kusum_c_pump_solarisation`). Sign -. Layer.

- Mechanism: solarised grid-connected farm pumps, individual and feeder-level, draw less from the grid.
- Treatment: applied as given. The FY2023-24 level is not in the sources, so it is taken as zero and the increment is slightly overstated. Feeder-level plants feed 11 kV feeders: if CEA counts them as utility generation, the effect on NPP generation is zero.
- Source: MNRE, Comprehensive guidelines for PM-KUSUM (17.01.2024), para 2.1, p. 2 (Component C: 35 lakh pumps, feeder-level included); NITI Aayog, Sectoral Insights: Power, Table 2.1, p. 48 (pumps solarised and sanctioned as on 30.12.2025); CEA, 20th EPS, Sec. 1.5.3, p. 28 (7.5 HP a pump, CUF 17 percent).
- Size: 13.0 / 37.7 / 38.6 TWh. Energy a pump = 7.5 HP x 0.746 kW/HP x 8,760 h x 0.17 = 8,332 kWh a year. Low: 12.0 lakh pumps solarised by 30.12.2025 (10.0 TWh); central: the 35 lakh target (29.2 TWh); high: 35.8 lakh sanctioned (29.9 TWh); each x 1.2936.

**13. PM-KUSUM A decentralised plants** (`pm_kusum_a_decentralised_plants`). Sign -. Context.

- Mechanism: small grid-connected plants on farm land sell to the distribution companies; they change who generates, not how much is needed.
- Treatment: context. CEA's 20th EPS counts only Component C as changing grid demand, and whether Component A output is inside NPP's utility series is not stated.
- Source: MNRE, PM-KUSUM guidelines, para 4.1, p. 3; CEA, 20th EPS, p. 28; NITI Aayog, Sectoral Insights: Power, Table 2.1, p. 48.
- Size: not quantified (supply side).

**14. PM-KUSUM B standalone pumps** (`pm_kusum_b_standalone_pumps`). Sign -. Context.

- Mechanism: off-grid solar pumps replace diesel pumps.
- Treatment: context. They replace diesel, not grid electricity, so CEA's 20th EPS finds no effect on utility generation.
- Source: MNRE, PM-KUSUM guidelines, para 2.1, p. 2 (14 lakh pumps); NITI Aayog, Sectoral Insights: Power, Table 2.1, p. 48; CEA, 20th EPS, p. 28.
- Size: not quantified: no grid effect.

**15. Captive generation** (`captive_generation`). Sign +. Excluded by rule.

- Mechanism: plants inside industries, and shifts between captive and grid supply, change utility generation.
- Treatment: excluded by the organisers' written ruling of 16 November 2026 (NPP generation only, no captive generation; [decisions.md](decisions.md), D12). It is listed so the catalogue is complete. NITI Aayog expects a shift from captive to grid supply, which would raise NPP generation. The co-located (captive) half of hydrogen supply and any captive data-centre supply are outside NPP for the same reason.
- Source: the organisers' ruling; CEA General Review 2025; NITI Aayog, Sectoral Insights: Power, Sec. 3.3.4, p. 61 (29.74 percent of industrial electricity was captive in 2023-24).
- Size: not quantified, by rule.

### 3.5 Efficiency

**16. UJALA LED lighting** (`ujala_led_lighting`). Sign -. Core.

- Mechanism: LED lamps replaced incandescent and CFL lamps, lowering lighting demand.
- Treatment: inside the core forecast. The savings were realised within the 2006-2024 estimation window, so the elasticity already reflects them. The replacement is largely complete, so the trend may embed a gain that will not recur (row 43).
- Source: BEE, India Energy Scenario for 2023-24 (2nd ed., Dec 2024), Table 25, p. 134 (182.00 BU saved in 2023-24 against BEE's no-programme counterfactual); BEE, Impact of Energy Efficiency Measures 2022-23, p. 3.
- Size: inside the history, not a feature.

**17. Standards and labelling** (`standards_and_labelling`). Sign -. Core.

- Mechanism: minimum energy performance standards and star labels for appliances.
- Treatment: inside the core forecast; continuing the programme at its past pace is part of the trend.
- Source: BEE, India Energy Scenario 2023-24, Table 25, p. 134 (89.84 BU saved in 2023-24).
- Size: inside the history.

**18. PAT scheme** (`pat_scheme`). Sign -. Core.

- Mechanism: Perform, Achieve and Trade targets for energy-intensive industries.
- Treatment: inside the core forecast. PAT is being folded into the carbon credit trading scheme (row 40).
- Source: BEE, India Energy Scenario 2023-24, Table 25, p. 134 (40.11 BU saved in 2023-24, cycles I to VI).
- Size: inside the history.

**19. ECBC buildings and others** (`ecbc_buildings_and_others`). Sign -. Core.

- Mechanism: the Energy Conservation Building Code, star rating of buildings and efficient street lighting.
- Treatment: inside the core forecast.
- Source: BEE, India Energy Scenario 2023-24, Table 25, p. 134 (star rating 0.64 BU and street lighting 8.8 BU in 2023-24; the ECBC row has no electricity figure). All BEE schemes together saved 321.39 BU in 2023-24 and 307.33 BU in 2022-23.
- Size: inside the history.

**20. Efficiency beyond trend** (`efficiency_beyond_trend`). Sign -. Layer.

- Mechanism: appliance efficiency faster than current policy lowers residential demand.
- Treatment: applied as given. Low and central are 0, because NITI Aayog's Current Policy Scenario represents current policies and past trends. High is the gap between NITI's Net Zero and Current Policy scenarios. Cooling is left out here (row 25). BEE's UNNATEE target (285 BU in 2031) is already in the 20th EPS baseline, and its part beyond the model's trend cannot be separated, so it is not added.
- Source: NITI Aayog (Feb 2026), Sectoral Insights: Buildings, Sec. 4.5.1, pp. 79 and 81 (lighting 66 / 62, heating 96 / 76 and other appliances 277 / 192 TWh in 2050, Current Policy / Net Zero); NITI Aayog, Sectoral Insights: Power, p. 58; CEA, 20th EPS, Sec. 1.5.6, p. 30.
- Size: 0 / 0 / 125.3 TWh: the Net Zero gap of 109 TWh in 2050, x 24/27 and x 1.2936.

**21. National efficiency targets** (`national_efficiency_targets`). Sign -. Context.

- Mechanism: the NDC emissions-intensity target and BEE's national efficiency targets push demand down.
- Treatment: context. No official 2047 electricity figure beyond the trend can be separated.
- Source: India's Updated First NDC (Aug 2022), item 3 (45 percent cut in emissions intensity by 2030 from 2005); the Prime Minister's national statement at COP26 (1 Nov 2021); CEA, 20th EPS, p. 30.
- Size: not quantified.

### 3.6 Electrification and new loads

**22. Transport electrification** (`transport_electrification`). Sign +. Layer.

- Mechanism: battery electric road vehicles, rail and metro traction add grid load.
- Treatment: applied as given; only the part beyond the trend counts (2047 level less FY2023-24 traction carried by the trend). Rail and metro are inside this row, so rows 27 and 28 are not added on top.
- Source: NITI Aayog (Feb 2026), Sectoral Insights: Transport, Table 4.6, p. 92 and p. 93 (transport electricity 47 Mtoe in the Current Policy Scenario and 48 Mtoe in Net Zero, 2050); NITI Aayog, Sectoral Insights: Power, p. 71; CEA General Review 2025, Table 9.1, p. 137 (railways and tramways 33.92 TWh in FY2023-24); CEA, 20th EPS, p. 26 (a lower near-term path).
- Size: 511.9 / 511.9 / 525.2 TWh. 1 Mtoe = 11.63 TWh, so 546.6 and 558.2 TWh in 2050, and 489.6 and 500.0 TWh in 2047 by 24/27. The trend carries 33.92 x 2.7697 = 93.9 TWh. Each less the trend, x 1.2936. Only two official figures exist, so low = central.

**23. Green hydrogen** (`green_hydrogen`). Sign +. Layer.

- Mechanism: electrolysers add a new load. Only grid-supplied electricity is in NPP's scope.
- Treatment: applied as given; all of it is beyond the trend, because there was no green hydrogen in FY2023-24. Half the electricity is taken from the grid (CEA's 20th EPS; the co-located half is captive and outside NPP). With all supply from the grid the values would double.
- Source: MNRE, National Green Hydrogen Mission (Jan 2023), p. 24 (at least 5 MMT a year by 2030); NITI Aayog (Feb 2026), An Overview, p. 101 (470 TWh in the Current Policy Scenario, 1,392 TWh in Net Zero, 2050); NITI Aayog, Sectoral Insights: Power, p. 62; CEA, 20th EPS, Sec. 1.5.4, p. 29 (50 kWh per kg; half on the grid).
- Size: 137.9 / 230.4 / 682.3 TWh. Low: the 2030 target held (5 MMT x 50 kWh/kg = 250 TWh); central: 470 x 24/27 = 417.8 TWh; high: 1,392 x 24/27 = 1,237.3 TWh. Each x 0.5 grid share and x 1.1029 (connected at extra-high voltage). NITI calls its Current Policy figure additional generation, so the gross-up may double count about 10 percent there.

**24. Data centres** (`data_centres`). Sign +. Layer.

- Mechanism: data-centre IT and cooling load (IT load x utilisation x PUE).
- Treatment: applied as given; only the part beyond the 2024 level carried by the trend counts. Operators also use captive and group-captive supply, which would be outside NPP; no official share was found, so none is removed.
- Source: NITI Aayog (Feb 2026), An Overview, p. 143 (2047: 65 GW of IT load at 50 percent utilisation and PUE 1.4, about 45 GW on average, which is 394 TWh); NITI Aayog, Sectoral Insights: Power, p. 63 (1.4 GW in 2024, 12.26 TWh a year) and pp. 71-72 (about 400 TWh by 2050).
- Size: 356.2 / 397.1 / 397.1 TWh. The trend carries 34.0 TWh. Low: the 2050 value scaled to 2047 (356.9 TWh); central = high: 394 TWh in 2047, the highest official figure. Each less the trend, x 1.1029.

**25. Space cooling** (`space_cooling`). Sign +. Layer.

- Mechanism: air conditioners, coolers and fans in homes and commercial buildings, rising with income and heat.
- Treatment: low and central are 0, because cooling that grows with income is in the trend. High is the India Cooling Action Plan's growth above the trend. It is an upper bound: cooling also grew faster than total demand in 2006-2024, and that is inside the elasticity. Hotter weather is part of the projection, so row 33 is not added on top.
- Source: MoEFCC Ozone Cell, India Cooling Action Plan (Mar 2019), Figure D and the summaries of Chapters 3 and 4, p. 17 (space cooling about 135 TWh in 2017-18 and about 585 TWh in 2037-38 in the reference case; the intervention case about 30 percent lower); NITI Aayog, Sectoral Insights: Buildings, p. 79.
- Size: 0 / 0 / 252.3 TWh. FY2023-24 cooling = 135 x (585/135)^(6/20) = 209.6 TWh, which the trend carries to 389.9 TWh in FY2037-38. The plan's excess in 2037-38 (195.1 TWh) is held flat to 2047 and x 1.2936. The intervention case would cut the high value to roughly 25 TWh.

**26. Electric cooking** (`electric_cooking`). Sign +. Context.

- Mechanism: a shift from LPG and biomass to electric cooking.
- Treatment: context. NITI Aayog gives only shares and a chart without printed values, and the only TWh figures (TERI) are from a think tank.
- Source: NITI Aayog, Sectoral Insights: Power, Table 3.3, p. 61 (cooking under 1 percent electric today; 30 percent in the Current Policy Scenario and 60 percent in Net Zero by 2070); NITI Aayog, Sectoral Insights: Buildings, Fig. 4.9, p. 87.
- Size: not quantified: no official TWh figure.

**27. Railway traction** (`railway_traction`). Sign +. Context.

- Mechanism: railway electrification and traffic growth raise traction demand.
- Treatment: inside row 22 (NITI Aayog's transport electricity includes rail). Railways buying through open access stay inside NPP generation.
- Source: CEA General Review 2025, Table 9.1, p. 137 (33,917.67 GWh, 2.51 percent of utility sales, +12.96 percent); NITI Aayog, Sectoral Insights: Power, p. 59; BEE, Impact of Energy Efficiency Measures 2022-23, p. 184 (non-traction use).
- Size: not quantified separately: counted in transport electrification.

**28. Metro rail** (`metro_rail`). Sign +. Context.

- Mechanism: urban metro networks add traction load.
- Treatment: inside row 22 and CEA's railways and tramways category. No official national metro electricity figure was found.
- Source: NITI Aayog, Sectoral Insights: Power, Table 3.2, p. 60 (metro modal share 1 percent in 2024, 2 percent in 2050).
- Size: not quantified separately.

**29. Desalination** (`desalination`). Sign +. Context.

- Mechanism: seawater desalination for urban and industrial water.
- Treatment: context.
- Source: no official Indian figure for desalination electricity to 2047 was found in the CEA, NITI Aayog and MoHUA material searched.
- Size: not quantified: no official figure.

**30. Industrial electrification (green steel)** (`industrial_electrification_green_steel`). Sign +. Context.

- Mechanism: electric process heat, heat pumps, scrap-based electric arc furnaces and hydrogen-based steel shift industry towards electricity.
- Treatment: context. Industry is 33.3 percent of utility sales and its growth is inside the trend. The official figures include captive supply, which is excluded by rule, and give no grid-only increment for 2047. Hydrogen for steel is inside row 23.
- Source: Ministry of Steel, Greening the Steel Sector in India (Sept 2024), Executive Summary, p. 21 (steel electricity 94 TWh in 2021-22 and 184 TWh in 2030-31, mostly captive in integrated plants); NITI Aayog, Sectoral Insights: Power, Table 3.4, p. 62 and p. 70.
- Size: not quantified: the grid share beyond the trend cannot be separated.

**31. Agriculture pumping** (`agriculture_pumping`). Sign +. Context.

- Mechanism: irrigation pumping grows with groundwater depth and cropping; efficiency and solar pumps lower it.
- Treatment: inside the trend. Solar pumps are row 12; a dry year is row 36.
- Source: CEA General Review 2025, Table 9.1, p. 137 (agriculture 275,484.96 GWh, 20.40 percent of utility sales, +12.97 percent); NITI Aayog, Sectoral Insights: Power, pp. 59 and 70.
- Size: not quantified: inside the history.

**32. Cold chain refrigeration** (`cold_chain_refrigeration`). Sign +. Context.

- Mechanism: pack-houses, cold storage, refrigerated transport and commercial refrigeration.
- Treatment: context: small against the 2047 total, and its growth with farm income is inside the trend.
- Source: MoEFCC Ozone Cell, India Cooling Action Plan, Chapter 4 summary, p. 17 (pack-houses 2.4 TWh in the next decade and 5.2 TWh in the decade after).
- Size: not quantified.

### 3.7 Climate and weather

**33. Warming (cooling degree days)** (`warming_cooling_degree_days`). Sign +. Context.

- Mechanism: warmer years raise cooling demand.
- Treatment: context. No official Indian estimate of electricity per cooling degree day was found; any value would come from the literature only. Warming is part of the cooling projections, so it is inside row 25's high value.
- Source: MoES (2020), Assessment of Climate Change over the Indian Region, Executive Summary, pp. 10-12 (about 0.7 C of warming over 1901-2018; about 4.4 C by 2100 under RCP8.5; summer heat waves three to four times more frequent by 2100); NITI Aayog, Sectoral Insights: Buildings.
- Size: not quantified.

**34. Monsoon hydro variability** (`monsoon_hydro_variability`). Sign +. Context.

- Mechanism: a weak monsoon lowers hydro output and raises pumping demand.
- Treatment: context. Hydro variability changes the mix (Part 2), not the total requirement; the extra pumping of dry years is inside the core forecast's persistent model error.
- Source: NITI Aayog, Sectoral Insights: Power, p. 66 (large-hydro PLF 32 to 40 percent over five years); CEA General Review 2025, Table 6.5.2, p. 109 (hydro 134,053.93 GWh in FY2023-24).
- Size: not quantified.

### 3.8 Shocks

**35. Extreme heat year** (`extreme_heat_year`). Sign +. Driver.

- Mechanism: a hot year raises demand above the trend; FY2023-24 was 0.072 log points above the fitted relation.
- Treatment: an existing uncertainty. Weather-year shocks enter through the AR(1) model error of the core forecast (persistence 0.51, residual s.d. 0.037; [methodology.md](methodology.md), section 2.5).
- Source: the model's own estimate on NPP data (`outputs/india_fit.csv`); MoES (2020), Executive Summary, p. 12 (heat waves).
- Size: not a feature.

**36. Drought** (`drought`). Sign +. Context.

- Mechanism: drought years raise irrigation pumping and cut hydro.
- Treatment: context: part of the model error, not quantified separately.
- Source: CEA General Review 2025, Table 9.1, p. 137 (agriculture +12.97 percent in FY2023-24).
- Size: not quantified.

**37. Pandemic** (`pandemic`). Sign -. Core.

- Mechanism: a pandemic year depresses demand (FY2020-21).
- Treatment: estimated as a dummy in the core regression (-0.055 log points). No pandemic is assumed for 2047.
- Source: the model's own estimate on NPP data (`outputs/india_fit.csv`, row `covid_fy2020`).
- Size: inside the estimation, not a feature.

### 3.9 Prices and policy

**38. Electricity tariffs** (`electricity_tariffs`). Sign -. Context.

- Mechanism: higher real tariffs lower demand.
- Treatment: context. Tariffs are set by CERC and the state commissions, and no official projection of real tariffs to 2047 was found.
- Source: no official source found in the CEA, CERC, NITI Aayog and BEE documents searched.
- Size: not quantified: literature only.

**39. Price elasticity** (`price_elasticity`). Sign -. Context.

- Mechanism: the own-price elasticity of electricity demand.
- Treatment: context. No official estimate from CEA, CERC, RBI or NITI Aayog was found.
- Source: no official source found.
- Size: not quantified: literature only.

**40. Carbon market (CCTS)** (`carbon_market_ccts`). Sign -. Context.

- Mechanism: a carbon price on obligated industries favours efficiency and cleaner supply.
- Treatment: context. No official electricity-demand impact is published; it works through PAT's successor and through generation costs (the mix's carbon-price setting).
- Source: Ministry of Power, Carbon Credit Trading Scheme 2023, Gazette S.O. 2825(E), 28 Jun 2023, p. 6.
- Size: not quantified.

**41. Net zero 2070** (`net_zero_2070`). Sign +. Context.

- Mechanism: the net-zero commitment pushes electrification, which raises demand, and efficiency, which lowers it.
- Treatment: context. It shows up through the Net Zero (high) values of rows 20, 22 and 23. NITI Aayog's scenario totals include captive use and are consumption-side, so they are context only.
- Source: the Prime Minister's national statement at COP26 (1 Nov 2021); NITI Aayog, Sectoral Insights: Power, Table E.1, p. 24.
- Size: not quantified separately.

**42. RPO and RCO trajectory** (`rpo_rco_trajectory`). Sign +. Context.

- Mechanism: renewable consumption obligations set the supply mix, not the quantity.
- Treatment: context (supply side). Its distributed renewable component overlaps rows 11 and 12.
- Source: Ministry of Power, Gazette S.O. 4617(E), 20 Oct 2023, p. 3 (total obligation 43.33 percent in 2029-30, including 4.50 percent distributed renewables).
- Size: not quantified.

### 3.10 Historical trend effects

**43. Trend carry-over (losses, deficit)** (`trend_carryover_losses_deficit`). Sign +. Context.

- Mechanism: the 2006-2024 trend embeds one-off gains that cannot recur at the same pace: falling losses, closing supply deficits and the LED roll-out.
- Treatment: a diagnostic, not added. Falling T&D losses lowered generation growth by about 0.144 log points over the window, so the trend may understate future generation if losses cannot keep falling. Closing supply deficits raised it, so the trend may overstate. The net direction is not established from official figures alone. This row is why T&D losses (row 6) are flagged as overlapping.
- Source: CEA, Growth of Electricity Sector in India 1947-2025, Chart 29, p. 58 (T&D losses 28.65 percent in 2006-07, 17.63 percent in 2023-24); NITI Aayog, Sectoral Insights: Power, p. 37 (energy deficit 11.1 percent in 2008-09, 0.1 percent in 2024-25); BEE, India Energy Scenario 2023-24, Table 25, p. 134.
- Size: not quantified.

**44. Household access and supply hours** (`household_access_supply_hours`). Sign +. Context.

- Mechanism: universal household connection and longer supply hours released suppressed demand.
- Treatment: inside the history and close to saturation, so it will not repeat at the same pace.
- Source: NITI Aayog, Sectoral Insights: Power, p. 37 (urban supply 22.1 to 23.4 hours a day and rural 12.5 to 21.9 hours a day, FY2013-14 to FY2023-24).
- Size: not quantified.

## 4. Named scenarios

Each named scenario adds one constant total to every draw of the core forecast, ramped linearly from the base year to its full value in FY2047-48, and split across regions by each region's share of the requirement (ruling R23).
Every other setting is the default, with the same seed and draws as the headline (`outputs/feature_scenarios_2047.csv`, section `named`).

| Scenario | Rows and levels | Adjustment, TWh | P5 | P50 | P95 | P50 vs Core |
| --- | --- | --- | --- | --- | --- | --- |
| Core (headline) | No feature layer | 0 | 3,190 | 4,688 | 7,021 | |
| Features central | Every row at its central value; T&D losses at 0 | +1,122.4 | 4,313 | 5,810 | 8,143 | +23.9% |
| Electrification high | Transport, green hydrogen, data centres and space cooling high; the rest central | +1,839.9 | 5,030 | 6,528 | 8,860 | +39.3% |
| Efficiency and distributed | Every row that lowers generation at its high value (T&D losses included, at their upper bound); the rest central | +388.6 | 3,579 | 5,076 | 7,409 | +8.3% |
| Climate stress | Space cooling and storage losses high; the rest central | +1,374.7 | 4,565 | 6,063 | 8,395 | +29.3% |
| All upside | Every row that raises generation high, every row that lowers it low (T&D losses at 0) | +1,866.8 | 5,057 | 6,555 | 8,887 | +39.8% |
| All downside | Every row that raises generation low, every row that lowers it high (T&D losses at their upper bound) | +177.3 | 3,368 | 4,865 | 7,198 | +3.8% |

TWh in FY2047-48.
A constant adjustment shifts every quantile by its total (up to histogram binning, about 0.1 TWh).

Regional medians, TWh:

| Scenario | Northern | Western | Southern | Eastern | North-Eastern |
| --- | --- | --- | --- | --- | --- |
| Core (headline) | 1,342 | 1,276 | 1,188 | 730 | 151 |
| Features central | 1,663 | 1,582 | 1,473 | 905 | 187 |
| Electrification high | 1,869 | 1,777 | 1,655 | 1,017 | 210 |
| Efficiency and distributed | 1,453 | 1,382 | 1,286 | 790 | 163 |
| Climate stress | 1,736 | 1,650 | 1,537 | 944 | 195 |
| All upside | 1,877 | 1,784 | 1,662 | 1,021 | 211 |
| All downside | 1,393 | 1,324 | 1,233 | 757 | 156 |
| Joint (section 5) | 1,623 | 1,543 | 1,436 | 883 | 182 |

Every feature scenario raises the requirement, even All downside: transport (at least 512 TWh), data centres (at least 356) and hydrogen (at least 138) outweigh every row that lowers generation at its high value, T&D losses included.

**One at a time.** Each layer row at its central value and at its extreme, the end of its range farthest from central (ties go to high), with every other row off (`outputs/feature_scenarios_2047.csv`, section `one_at_a_time`):

| Row | Central: adjustment, P50 | Extreme: level, adjustment, P50 |
| --- | --- | --- |
| T&D losses | 0; 4,688 | high; -476.3; 4,212 |
| Auxiliary consumption | -91.6; 4,596 | high; -110.5; 4,577 |
| Storage round-trip losses | +150.1; 4,838 | low; +72.2; 4,760 |
| Cross-border imports | -13.1; 4,675 | high; -15.3; 4,673 |
| Rooftop solar | -24.7; 4,663 | high; -134.9; 4,553 |
| PM-KUSUM C pump solarisation | -37.7; 4,650 | low; -13.0; 4,675 |
| Efficiency beyond trend | 0; 4,688 | high; -125.3; 4,563 |
| Transport electrification | +511.9; 5,200 | high; +525.2; 5,213 |
| Green hydrogen | +230.4; 4,918 | high; +682.3; 5,370 |
| Data centres | +397.1; 5,085 | low; +356.2; 5,044 |
| Space cooling | 0; 4,688 | high; +252.3; 4,940 |

TWh in FY2047-48.

## 5. Joint distribution and the P90 plan test

The joint scenario adds an independent draw of every layer row to each core draw (ruling R24):

- each row is sign x triangular(low, central, high);
- T&D losses are -uniform(0, 476.3), because their size is an upper bound (ruling R22);
- the draws use their own random stream, so the core draws are the headline's.

| FY2047-48, TWh | P5 | P10 | P50 | P90 | P95 | Mean |
| --- | --- | --- | --- | --- | --- | --- |
| Core (headline) | 3,190 | 3,471 | 4,688 | 6,408 | 7,021 | 4,843 |
| Core plus features (joint) | 4,130 | 4,423 | 5,668 | 7,404 | 8,017 | 5,819 |
| Feature total alone | 659 | | 973 | | 1,308 | 976 |

- The joint median is 980 TWh (+20.9 percent) above the headline.
- It sits below Features central (+1,122 TWh) because the joint draws give T&D losses a mean of -238 TWh, where Features central counts them at 0.
- Joint regional medians are in the table of section 4.

**P90 plan test** (`outputs/feature_scenarios_2047.csv`, section `p90_plan`). The Robust policy's least-cost plan is solved at three requirements:

| Requirement planned for | TWh | Robust plan unmet, TWh |
| --- | --- | --- |
| Core P90 (the headline stress test) | 6,408 | 639 |
| Core plus features, joint P50 | 5,668 | 0 |
| Core plus features, joint P90 | 7,404 | 1,635 |

The Robust plan meets the joint median in full. At the joint P90 it leaves 1,635 TWh unmet (the LP status is optimal). The levers that close the core P90 gap (more usable solar land, [results.md](results.md), section 3.3) were tested only at the core P90.

## 6. How the scenarios cover every combination

- **Every combination of feature levels lies between All downside and All upside.** The layer is a sum of independent signed rows, each between its low and high value (T&D losses between 0 and their upper bound). The smallest possible total takes each row at the end that lowers generation: All downside, +177.3 TWh. The largest takes each at the end that raises it: All upside, +1,866.8 TWh. Every joint draw lies inside this envelope; the test `joint_quantiles_bracket` checks it on 2^18 draws.
- **The named scenarios mark the corners that matter.** Electrification high and Climate stress push the load rows up; Efficiency and distributed pushes the offset rows to their limit; Features central is the official central case; the joint distribution weighs all combinations by their ranges.
- **Crossed with growth.** The three growth presets (Base, High growth, Low growth; [assumptions_and_uncertainty.md](assumptions_and_uncertainty.md), section 6) change the core forecast. A named feature scenario adds the same ramped total to any of them, so it shifts that preset's quantiles by its adjustment; the joint scenario adds the same feature draws. `outputs/feature_scenarios_2047.csv` reports the scenarios on the Base growth preset; the dashboard applies any feature scenario on top of any growth preset.
- **Crossed with policy.** The four policy presets (Base, Low carbon, Robust, Energy security) take whatever requirement the growth and feature settings give. `outputs/presets.csv` holds the 3 x 4 growth-by-policy matrix without features, the P90 plan test above solves the Robust plan with them, and the dashboard solves any policy for any feature scenario.
- So every combination of growth preset, feature levels and policy preset is either computed directly or bounded by the envelope on the chosen growth preset.

## 7. Overlap rules

- **The trend rule.** The core forecast carries every existing load growing with total generation: x2.7697 from FY2023-24 (1,698.03 TWh) to FY2047-48, and x1.8604 to FY2037-38. Only level 2047 - level 2023 x 2.7697 is beyond the trend. For supply-side rows (T&D losses, auxiliary use, imports) the rule is the change in rate or level from FY2023-24. The multiples are those of the 20 Nov headline (4,703 TWh); on the current headline the multiple is 2.7607, which changes no row by more than 2 TWh, so the values were not rescaled.
- **Rows that overlap the history** (`overlap_history` = yes). Only T&D losses is a layer row with full overlap. Its size is an upper bound on the effect beyond the trend, so it is 0 at central, uniform from 0 in the joint draws and at its high value only in the downside scenarios (ruling R22). The other "yes" rows are core, driver or context rows and add nothing.
- **Partly overlapping rows** (`partial`) are already cut to the part beyond the trend. Where no official figure separates that part (space cooling, efficiency beyond trend), low and central are 0 and high is the whole excess over the trend.
- **Rows inside other rows.** Rail and metro are inside transport electrification. Hotter weather is inside space cooling. Hydrogen for steel is inside green hydrogen. The distributed renewable component of the RCO overlaps rooftop solar and PM-KUSUM. None of these is added twice.
- **Part 2 storage.** The storage back-calculation of the plan ([results.md](results.md), section 3.6) reports its own round-trip losses (136 TWh in the Robust plan). If those are added to the requirement, the storage row must not be added again.
- **Gross-up interaction.** New loads are grossed up at FY2023-24 loss rates. With T&D losses active they are overstated by up to about 10 percent.
- **Captive and off-grid supply.** Hydrogen takes CEA's 50 percent grid share; the co-located half is captive and outside NPP. No official captive share exists for data centres, so none is removed.
- **Independence.** The rows are summed, and drawn, independently, exactly as the CSV gives them; no official source gives their correlations.

## 8. Limits

The full list is in [decisions.md](decisions.md), D14. The ones that touch this catalogue:

- Only T&D losses and data centres have official values for or near 2047; the other rows hold or scale official values for 2030 to 2050.
- The unit factors are official but fixed (section 3 gives each).
- Whether feeder-level solar (PM-KUSUM C) and Component A plants are inside NPP utility generation is not stated in the sources. If they are, row 12's effect is zero.
- Features are split across regions by each region's share of the requirement; no official regional split exists (ruling R23).
- The notes of the core and driver rows in `data/external/demand_features.csv` quote the 20 Nov run (elasticity 0.828, 4,703 TWh); section 3 gives the current values.
