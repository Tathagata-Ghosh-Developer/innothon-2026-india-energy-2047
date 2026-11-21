# Demand features: every driver of the 2047 requirement

This document lists every factor that can change India's generation requirement in FY2047-48.
For each driver it gives the mechanism and the sign of its effect, how the model treats it, its official source, and its size in TWh or the reason it has no size.
It then gives the named scenarios, the joint distribution, how the scenarios cover every combination, the overlap rules and the official conferences and texts reviewed (section 9).

- **The headline does not change.** It is the two-dataset forecast: a median of 4,688 TWh in FY2047-48 (P5 3,190, P95 7,021), estimated on NPP generation and the Renewable India dataset's population and GSDP. These two are the only primary sources.
- **The features are a labelled layer outside the headline.** Every feature size comes from an official source (a ministry, a statutory body such as CEA, BEE or RBI, NITI Aayog, or a national statistics office) and is labelled secondary. Intergovernmental texts (UNFCCC, G20, IEA, IRENA) give context only; none sizes a row.
- **Generation only.** Every size is a change in FY2047-48 utility generation (NPP scope), in TWh. Where a source gives capacity (GW) or end-use electricity, the row converts it explicitly to generation with a stated capacity factor and loss gross-up. Installed capacity and consumption are never forecast metrics.
- Years are fiscal years labelled by their start year: 2023 is FY2023-24 and 2047 is FY2047-48.

The data are `data/external/demand_features.csv` (one row per driver, with URLs, pages and access dates; `data/external/SOURCES.md`, section 11).
The model's outputs are `outputs/demand_features_catalogue.csv`, `outputs/feature_scenarios_2047.csv` and the `features` block of `outputs/site_summary.json`.
The decision record is [decisions.md](decisions.md), D14 and D15 (22 November 2026: the official-conference review, two raised highs and the Nuclear Energy Mission).

## 1. How a driver is treated

Each driver gets one of five treatments (column `in_model`).

| Treatment | Count | Meaning | Why |
| --- | --- | --- | --- |
| Core | 7 | Estimated in the headline forecast | The driver is in the two challenge datasets, or its effect is already in the 2006-2024 history the elasticity is estimated on |
| Driver | 3 | An existing uncertainty or setting of the core forecast | The forecast already carries it as a prior, a setting or the model error |
| Layer | 11 | A signed TWh adjustment, outside the headline | An official source sizes its effect beyond the historical trend |
| Excluded by rule | 2 | Not estimated or reported | The organisers' written ruling of 16 November 2026 excludes it (captive generation, and the captive small reactors of the nuclear mission) |
| Context | 50 | Listed, not quantified | No official figure exists, the effect is inside the trend or inside another row, or it changes the supply mix, not the quantity; each entry says which |

**Conventions of the layer rows.**

- **Sign.** +1 raises utility generation; -1 lowers it. The layer applies sign x size.
- **Low, central, high.** Low and high are the smallest and largest official cases for 2047 after the rules below. Central is the current-policy case where one exists (NITI Aayog's Current Policy Scenario, the National Green Hydrogen Mission, scheme targets), otherwise the figure dated 2047. Where a row has only one official anchor, central equals low and the high case is a stated continuation.
- **Beyond the trend only.** The core forecast already carries every existing load growing with total generation. Only the part beyond that is a feature (section 7).
- **Year conversion.** A 2050 value is scaled linearly from the FY2023-24 base: level 2047 = base + (level 2050 - base) x 24/27. A value for an earlier year is held flat to 2047 unless the entry states a continuation.
- **Gross-up to generation** (CEA General Review 2025, FY2023-24): loads at low tension x1.2936 = 1 / ((1 - 0.1763)(1 - 0.0615)); high-tension loads x1.1029 = 1 / ((1 - 0.0339)(1 - 0.0615)); energy at the bus bar x1.0655 = 1 / (1 - 0.0615).
- **Overlap flag** (`overlap_history`): yes, partial or no overlap with the FY2006-07 to FY2023-24 trend.

## 2. All 73 drivers at a glance

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
| 8 | Storage round-trip losses | Supply side and system | + | Layer | 72.2 to 173.6 (150.1) |
| 9 | Storage VGF and co-location | Supply side and system | + | Context | - |
| 10 | Cross-border imports | Supply side and system | - | Layer | 10.9 to 49.3 (13.1) |
| 11 | Cross-border exports | Supply side and system | + | Context | - |
| 12 | CEA 2047 capacity mix | Supply side and system | + | Context | - |
| 13 | Nuclear Energy Mission | Supply side and system | + | Context | - |
| 14 | Coal pipeline to 2035 | Supply side and system | + | Context | - |
| 15 | Biomass co-firing mandate | Supply side and system | + | Context | - |
| 16 | CCUS energy penalty | Supply side and system | + | Context | - |
| 17 | Brahmaputra hydro master plan | Supply side and system | + | Context | - |
| 18 | Transmission plan to 2035-36 | Supply side and system | + | Context | - |
| 19 | Resource adequacy framework | Supply side and system | + | Context | - |
| 20 | RE-INVEST 2024 commitments | Supply side and system | + | Context | - |
| 21 | Critical minerals supply chain | Supply side and system | + | Context | - |
| 22 | Rooftop solar | Distributed and behind-the-meter | - | Layer | 24.7 to 134.9 (24.7) |
| 23 | PM-KUSUM C pump solarisation | Distributed and behind-the-meter | - | Layer | 13.0 to 38.6 (37.7) |
| 24 | PM-KUSUM A decentralised plants | Distributed and behind-the-meter | - | Context | - |
| 25 | PM-KUSUM B standalone pumps | Distributed and behind-the-meter | - | Context | - |
| 26 | PM-KUSUM 2.0 | Distributed and behind-the-meter | - | Context | - |
| 27 | Captive generation | Distributed and behind-the-meter | + | Excluded by rule | - |
| 28 | Captive nuclear (BSR, BSMR) | Distributed and behind-the-meter | - | Excluded by rule | - |
| 29 | UJALA LED lighting | Efficiency | - | Core | - |
| 30 | Standards and labelling | Efficiency | - | Core | - |
| 31 | PAT scheme | Efficiency | - | Core | - |
| 32 | ECBC buildings and others | Efficiency | - | Core | - |
| 33 | Efficiency beyond trend | Efficiency | - | Layer | 0 to 125.3 (0) |
| 34 | National efficiency targets | Efficiency | - | Context | - |
| 35 | Smart metering (RDSS) | Efficiency | - | Context | - |
| 36 | Sustainable lifestyles (Mission LiFE) | Efficiency | - | Context | - |
| 37 | Transport electrification | Electrification and new loads | + | Layer | 511.9 to 525.2 (511.9) |
| 38 | EV sales targets | Electrification and new loads | + | Context | - |
| 39 | Green hydrogen | Electrification and new loads | + | Layer | 137.9 to 682.3 (230.4) |
| 40 | Green hydrogen hubs | Electrification and new loads | + | Context | - |
| 41 | Data centres | Electrification and new loads | + | Layer | 356.2 to 697.8 (397.1) |
| 42 | Data-centre tax holiday to 2047 | Electrification and new loads | + | Context | - |
| 43 | AI compute (GPUs) | Electrification and new loads | + | Context | - |
| 44 | Telecom networks | Electrification and new loads | + | Context | - |
| 45 | Consumer electronics and devices | Electrification and new loads | + | Context | - |
| 46 | Semiconductor and electronics manufacturing | Electrification and new loads | + | Context | - |
| 47 | Digital public infrastructure | Electrification and new loads | + | Context | - |
| 48 | Space cooling | Electrification and new loads | + | Layer | 0 to 252.3 (0) |
| 49 | Electric cooking | Electrification and new loads | + | Context | - |
| 50 | Railway traction | Electrification and new loads | + | Context | - |
| 51 | Metro rail | Electrification and new loads | + | Context | - |
| 52 | Desalination | Electrification and new loads | + | Context | - |
| 53 | Industrial electrification (green steel) | Electrification and new loads | + | Context | - |
| 54 | Agriculture pumping | Electrification and new loads | + | Context | - |
| 55 | Cold chain refrigeration | Electrification and new loads | + | Context | - |
| 56 | Electricity share of energy in 2047 | Electrification and new loads | + | Context | - |
| 57 | Warming (cooling degree days) | Climate and weather | + | Context | - |
| 58 | Monsoon hydro variability | Climate and weather | + | Context | - |
| 59 | Extreme heat year | Shocks | + | Driver | - |
| 60 | Drought | Shocks | + | Context | - |
| 61 | Pandemic | Shocks | - | Core | - |
| 62 | Energy security and fuel-price shocks | Shocks | + | Context | - |
| 63 | Electricity tariffs | Prices and policy | - | Context | - |
| 64 | Price elasticity | Prices and policy | - | Context | - |
| 65 | Time-of-day tariffs | Prices and policy | - | Context | - |
| 66 | Fossil-fuel subsidy phase-out | Prices and policy | + | Context | - |
| 67 | LNG and energy independence | Prices and policy | + | Context | - |
| 68 | Carbon market (CCTS) | Prices and policy | - | Context | - |
| 69 | EU CBAM (steel, aluminium) | Prices and policy | + | Context | - |
| 70 | Net zero 2070 | Prices and policy | + | Context | - |
| 71 | RPO and RCO trajectory | Prices and policy | + | Context | - |
| 72 | Trend carry-over (losses, deficit) | Historical trend effects | + | Context | - |
| 73 | Household access and supply hours | Historical trend effects | + | Context | - |

## 3. The catalogue, by group

Sources were accessed on 21 November 2026, and those of the rows added or revised in the official-conference review on 22 November 2026; the CSV gives each URL and date.
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
- Source: Ministry of Power, National Framework for Promoting Energy Storage Systems (Aug 2023), para 3.3, p. 6 (CEA's 2047 storage need: 1,840 GWh of batteries and 540 GWh of pumped storage); CEA, National Generation Adequacy Plan 2026-27 to 2035-36 (19 Mar 2026), Fig. 4.5, p. 43 (2035-36: 321 GWh of batteries, 567 GWh of pumped storage) and Annexure-B, p. 71 (round trip 88 percent for batteries, 80 percent for pumped storage); CEA, NEP 2022-32, Sec. 13.5, p. 314 (batteries run one cycle a day); CEA, Road Map for Achieving the Goal of 100 GW of Nuclear Capacity by 2047 (released 11.10.2025), Table 5.3, PDF p. 41 (2047: 702.9 GWh of pumped storage and 1,983.9 GWh of energy storage systems). Conference context: the COP29 Global Energy Storage and Grids Pledge (1,500 GW worldwide by 2030), the IEA's 2024 ministerial communique (para 14) and COP30 decision 2/CMA.7 (para 12(r)) give no India share, so the row keeps the Ministry of Power and CEA values.
- Size: 72.2 / 150.1 / 173.6 TWh. Loss = 365 cycles x [battery GWh x (1/0.88 - 1) + pumped-storage GWh x (1/0.80 - 1)] x 1.0655. Low holds the 2035-36 storage to 2047; central is the 2047 need quoted by the Ministry of Power. High (since 22 Nov 2026; 150.1 before, when no official upper case had been found) is the road map's 2047 storage, its energy storage systems read as batteries: 365 x [1,983.9 x (1/0.88 - 1) + 702.9 x (1/0.80 - 1)] GWh x 1.0655 = 173.6 TWh. Partial cycling would lower it. The road map's 475.7 GW and 2,686.8 GWh of storage are close to the Robust plan's back-calculation (448 GW, 3,048 GWh; [results.md](results.md), section 3.6).

**9. Storage VGF and co-location** (`storage_vgf_and_colocation`). Sign +. Context.

- Mechanism: viability gap funding for batteries, the waiver of inter-state transmission charges for storage and CEA's co-location advisory bring storage forward; they change storage losses and the mix's flexibility, not the requirement.
- Treatment: context, covered. The 2047 storage and its losses are row 8; the plan's own storage is the Part 2 back-calculation ([results.md](results.md), section 3.6).
- Source: PIB (Ministry of Power) 2135450 (10.06.2025: viability gap funding for 30 GWh of batteries, Rs 5,400 crore, on top of 13.2 GWh; the transmission-charge waiver for storage extended to 30.06.2028), 2222473 (03.02.2026), 2205959 (18.12.2025: CEA's storage need of about 336 GWh by 2029-30 and 411 GWh by 2031-32; CEA's advisory of February 2025, storage of at least 10 percent of solar capacity for two hours) and 2064702 (14.10.2024, CEA's vision for pumped storage in 2047).
- Size: not quantified: near-term steps; the 2047 storage is sized in row 8.

**10. Cross-border imports** (`cross_border_imports`). Sign -. Layer.

- Mechanism: hydro imports from Bhutan and Nepal replace domestic generation; NPP generation excludes imports.
- Treatment: applied as given, as the change from FY2023-24 imports (6.15 TWh).
- Source: CEA General Review 2025, Table 6.1, p. 103 (imports 6,147.23 GWh); CEA, NEP 2022-32, Table 5.5, p. 137 (5,856 MW of hydro imports by 2032: Bhutan 4,956, Nepal 900); NITI Aayog, Sectoral Insights: Power, p. 66 (large-hydro PLF 32 to 40 percent); PMIndia, press statement by the Prime Minister during the visit of the Prime Minister of Nepal (01.06.2023): "a target of importing 10,000 MW of electricity from Nepal in the coming ten years". The G20 New Delhi declaration (para 38, item 10) and COP30 decision 2/CMA.7 (para 12(r)) support interconnection in general, with no India quantity.
- Size: 10.9 / 13.1 / 49.3 TWh. Low and central: 2047 imports = 5,856 MW x PLF (32, 36 percent) x 8.76 = 16.4 / 18.5 TWh, less 6.15 TWh, x 1.0655. High (since 22 Nov 2026; 15.3 before, the NEP at PLF 40 percent): Bhutan's 4,956 MW plus the 10,000 MW Nepal target in place of NEP's 900 MW, 14,956 MW x 0.40 x 8.76 = 52.41 TWh, less 6.15, x 1.0655 = 49.3 TWh. Low and central stay on the NEP, because the Nepal target (about 2033) has no year-wise schedule or contracted quantity; the MEA's bilateral brief records the long-term power trade agreement without a quantity.

**11. Cross-border exports** (`cross_border_exports`). Sign +. Context.

- Mechanism: power exported to Bangladesh, Nepal, Bhutan and Myanmar, and over any future undersea or regional interconnection, is generated in India and raises NPP generation.
- Treatment: context. No official projection of exports to 2047 was found, and today's exports are inside the history. The Minister of Power and the International Solar Alliance state an export ambition, with no quantity.
- Source: CEA General Review 2025, Table 6.1, p. 103 (exports 11,295.19 GWh) and Table 6.2, p. 104 (Bangladesh 8.53, Nepal 1.42, Bhutan 1.33, Myanmar 0.01 TWh); PIB (Ministry of Power) 2242673 (19.03.2026, Bharat Electricity Summit 2026: India as "a future global exporter of affordable energy"; "cross-border energy connectivity, undersea transmission networks"); PIB 2183262 (28.10.2025, the eighth ISA Assembly: One Sun One World One Grid); G20 New Delhi declaration, para 38, item 10; COP30 decision 2/CMA.7, para 12(r).
- Size: not quantified. At today's level it is 0.2 percent of the 2047 median.

**12. CEA 2047 capacity mix** (`cea_2047_capacity_mix`). Sign +. Context.

- Mechanism: CEA's own estimate of the 2047 power system, by source and storage, and its statement that about two-thirds of 2047 generation will be non-fossil. It bounds the mix, not the quantity.
- Treatment: context for Part 2. Each capacity is converted at the model's own CUFs (GW x CUF x 8.76, `build_limits.csv`): solar 1,186.8 GW x 0.22 = 2,287.2 TWh; wind 434.9 GW x 0.2404 to 0.275 = 915.9 to 1,047.7; coal 235.3 GW x 0.587 = 1,209.9 (1,278.0 and 1,339.8 at NGAP's 62 and 65 percent PLF); nuclear 100 GW x 0.68 = 595.7 (539.6 for the 90.588 GW of utility reactors, row 13); large hydro 99 GW x 0.36 = 312.2; biomass 23 GW x 0.60 = 120.9; gas 10.4 GW x 0.1536 = 14.0. They sum to 5,587.6 TWh, more than the requirement, so each is an upper bound, not a dispatch. CEA's "about 2/3rd" non-fossil is below every policy preset's floor (70 to 80 percent), and one-third fossil at the 4,688 TWh median would be 1,562.6 TWh, above the national coal ceiling (1,424.4 TWh). Against the Robust plan: coal 893 TWh, explicit wind 198 TWh plus part of the 686 TWh of firmed RE, and 51.1 TWh of gas kept for peaking (a 56.1 percent capacity factor on a 10.4 GW fleet). Storage (row 8) and large hydro (346 TWh in the model) are consistent. Through station auxiliary use this mix gives about 85 to 115 TWh, inside or just above row 7's range, so row 7 is unchanged. The comparison table is in [results.md](results.md), section 3.9.
- Source: CEA, Road Map for Achieving the Goal of 100 GW of Nuclear Capacity by 2047 (MoP committee report, 27.06.2025; released 11.10.2025, PIB 2177769), Table 5.3, PDF p. 41 (printed p. 16): nuclear 100, coal 235.3, gas 10.4, biomass 23, large hydro 99, wind 434.9 and solar 1,186.8 GW (2,089.4 GW in all); pumped storage 116.1 GW / 702.9 GWh; energy storage systems 359.6 GW / 1,983.9 GWh. Para 5.6, PDF p. 40, and the Executive Summary, PDF p. 16: "about 2/3rd of electricity generation in 2047 will be met from non-fossil fuel sources". The CEA URL returned 404 on 22 Nov 2026; the Internet Archive copy was read.
- Size: not quantified (supply side). CEA's 2047 requirement and peak (Table 5.2) are in `data/external/policy_targets.csv` as energy requirement, context only, never a benchmark.

**13. Nuclear Energy Mission** (`nuclear_energy_mission`). Sign +. Context.

- Mechanism: about 100 GW of nuclear by 2047, opened to private operators by the SHANTI Act 2025. It changes the mix. Nuclear's station auxiliary use (8.78 percent) is above solar's (1.94 percent), so more nuclear slightly raises gross generation for the same energy sent out.
- Treatment: 0 in the requirement; a policy option in Part 2. The "Nuclear mission" policy is the Robust policy with national nuclear generation of at least the mission's utility reactors in 2047 ([results.md](results.md), section 3.9; [decisions.md](decisions.md), D15). Utility reactors = 100,648 MW less the captive BSR (5,060 MW) and BSMR (5,000 MW) units of row 28 = 90,588 MW. The recommended Robust plan keeps only the committed reactors (133.1 TWh), and the Base policy's 10 percent cap (468.8 TWh at the median) is below the mission.
- Source: Union Budget 2025-26, Budget Speech, paras 61-62, PDF p. 15 (at least 100 GW by 2047; at least five small modular reactors operational by 2033); DAE, Lok Sabha USQ 2783 (17.12.2025), answer (b) (NPCIL to contribute 54 GW) and answers (c)-(d) (BSMR-200 and SMR-55 "can be deployed as captive plant"); DAE, Lok Sabha USQ 5368 (25.03.2026: about 90 GW to be added by 2047); PIB explainer on the SHANTI Act (19.12.2025), PDF p. 3 (22.38 GW by 2031-32); the CEA road map (row 12), Table 6.3, PDF p. 50 (100,648 MW in 2047, of which BSR 5,060 and BSMR 5,000 MW; sites identified for 48,108 MW) and para 11.1.3.1 (c), PDF p. 77 (a PLF of 80 percent achievable; fleet PLF about 79 percent in 2024-25); CEA NGAP, Executive Summary, PDF p. 8; the Ministry of Power's conferences with States: PIB 2072871 (12.11.2024: nuclear plants at sites of retired coal plants), PIB 2130769 and 2134584 (resource adequacy plans to include nuclear) and PIB 2139257 ("at least one nuclear power project in each State").
- Size: 0 in the requirement. As a floor on the mix (GW x CUF x 8.76): central 90.588 GW x 0.68 (the CEA NEP CUF of the nuclear build limits) = 539.6 TWh, 11.5 percent of the 4,688 TWh median; low, NPCIL's 54 GW x 0.68 = 321.7 TWh; high, 90.588 GW x 0.80 = 634.8 TWh, above the national nuclear build cap (627.1 TWh), so reported as infeasible. All 100 GW at 0.68 would be 595.7 TWh, but only if every small reactor sold to the grid. The 22.38 GW expected by 2031-32 is 133.3 TWh, about the committed floor.

**14. Coal pipeline to 2035** (`coal_pipeline_2035`). Sign +. Context.

- Mechanism: the official coal build-out to the mid-2030s (at least 97 GW of new coal and lignite, 526.8 to 552.3 TWh at NGAP's 62 to 65 percent PLF; 315 GW of coal generating 1,819 BU in 2035-36) keeps coal generation far above FY2023-24 in the 2030s; coal also carries higher auxiliary use.
- Treatment: context; a path check, because the model solves 2047 only. NGAP's 315 GW generates 1,819 BU in 2035-36 (an implied PLF of 65.9 percent), 1.405 times FY2023-24 coal (1,294.9 TWh) and above the model's 2047 national coal ceiling of 1.10 times (1,424.4 TWh). The Ministry of Power's 307 GW at NGAP's 62 to 65 percent PLF gives 1,667.4 to 1,748.1 TWh. CEA's 2047 coal (235.3 GW, row 12) falls back to 1,209.9 to 1,339.8 TWh, within the ceiling: the official path peaks above it in the 2030s and is back below it by 2047. The ceiling's basis is restated against this programme ([assumptions_and_uncertainty.md](assumptions_and_uncertainty.md), S3).
- Source: PIB (Ministry of Power) 2240743 (16.03.2026: about 3,07,000 MW of coal and lignite needed by 2034-35, "an additional minimum 97,000 MW", 38,745 MW under construction); CEA NGAP, Executive Summary, PDF p. 7 (315 GW in 2035-36), Fig. 4.3, PDF p. 42 (1,819 BU, 51 percent of 3,596 BU) and PDF p. 46 (PLF 62 to 65 percent); PIB 2072871 (12.11.2024: coal plants to run flexibly down to 40 percent, already the storage dispatch rule).
- Size: not quantified (supply side, a 2030s path).

**15. Biomass co-firing mandate** (`biomass_cofiring_mandate`). Sign +. Context.

- Mechanism: mandatory co-firing of biomass pellets (and torrefied charcoal from municipal waste) in coal plants moves part of coal-plant output from fossil to non-fossil; the quantity of generation does not change.
- Treatment: context; mix accounting only. The model counts all coal-plant output as coal. Taking 5 percent of coal-plant energy as an approximation (the policy states the blend by weight, and the energy share depends on calorific values that are not official here), the Robust plan's coal (893 TWh) would carry about 45 TWh of bioenergy and about 43 Mt less CO2 (at 0.971 t/MWh), and CEA's 2047 coal (1,209.9 TWh) 60.5 TWh.
- Source: Ministry of Power, Annual Report 2025-26, PDF p. 7 (comprehensive co-firing policy of 7 Nov 2025) and PDF p. 196 (SAMARTH "aims to increase the level of co-firing from present 5% to higher levels"); PIB 1721473 (the SAMARTH mission, 2021).
- Size: not quantified: no change in the quantity.

**16. CCUS energy penalty** (`ccus_energy_penalty`). Sign +. Context.

- Mechanism: carbon capture at coal plants and industrial sources uses power. On power plants it raises station auxiliary use, so more gross generation is needed for the same energy sent out; a coal-with-capture pathway also keeps firm coal in the mix in place of some storage.
- Treatment: context. The Part 2 LP has no coal-with-capture option, and row 7 uses FY2023-24 auxiliary rates (coal 7.38 percent). No CEA, NITI Aayog or Ministry of Power figure gives the energy penalty or the volume captured in power for 2047; literature values are not used. If an official deployment figure appears, the row would be sized as captured Mt x electricity per tonne. NITI Aayog places CCUS mainly in cement, steel and refining, whose electricity is largely captive.
- Source: Union Budget 2026-27, Budget Speech, para 38, PDF p. 12 (Rs 20,000 crore over five years for CCUS in five sectors, power among them); UNFCCC decision 1/CMA.5, para 28(e), PDF p. 6; CEM16 Outcomes Document, PDF p. 8 (India joins the CEM CCUS Initiative); India's NDC 2031-35, PDF pp. 12 and 34; NITI Aayog (Feb 2026), An Overview, PDF p. 110 ("CCUS requires additional power"), and Sectoral Insights: Power, Box 2, PDF p. 78.
- Size: not quantified: no official energy penalty or capture volume.

**17. Brahmaputra hydro master plan** (`brahmaputra_hydro_master_plan`). Sign +. Context.

- Mechanism: CEA's transmission master plan for about 65 GW of hydro in the Brahmaputra basin would raise North-Eastern hydro well above today's, changing the regional mix and inter-regional flows.
- Treatment: context; regional mix. 65 GW x 0.36 x 8.76 = 205.0 TWh (182.2 to 227.8 at a PLF of 32 to 40 percent), against about 120 TWh of North-Eastern hydro in the model (70 percent of the usable potential). National hydro agrees with CEA (99 GW of large hydro in 2047 is 312.2 TWh, against 346 TWh in the model), so this is about where hydro sits, not how much. Part of the basin (Sikkim, north Bengal) is in the Eastern grid region.
- Source: PIB (Ministry of Power) 2177769 (11.10.2025, CEA's 52nd Foundation Day: the master plan for "approximately 65 GW of hydroelectric capacity"). The CEA master plan itself returned 404 on 22 Nov 2026 and was not read; its figures in press summaries are context only.
- Size: not quantified (regional mix).

**18. Transmission plan to 2035-36** (`transmission_plan_2035_36`). Sign +. Context.

- Mechanism: inter-regional transmission sets how far surplus regions can supply deficit regions; the 2035-36 plan and the 1100 kV corridors extend the 2032 plan that the model scales.
- Treatment: context; a corridor setting. The model's corridors are 2 times the 2032 plan (3 times in the Robust and Low carbon policies). The 2035-36 plan's inter-regional capacity could not be read, so the corridor setting is not yet anchored to it. Line lengths and transformer capacity describe the network, not generation, and are not converted.
- Source: PIB (Ministry of Power) 2242673 (19.03.2026: the transmission plan for over 900 GW of non-fossil capacity by 2035-36, 1,37,500 ckm and 8,27,600 MVA, Rs 7.93 lakh crore), 2135450 (10.06.2025: nine 1100 kV lines by 2034) and 2064702 (14.10.2024).
- Size: not quantified (the network, not energy).

**19. Resource adequacy framework** (`resource_adequacy_framework`). Sign +. Context.

- Mechanism: resource adequacy planning (loss-of-load probability, normalised energy not served, planning reserve margin) requires firm and storage capacity ahead of peak demand; it shapes the mix towards dispatchable sources, not the quantity.
- Treatment: context; a reliability setting. NGAP's 2035-36 firm share of generation is (coal 1,819 + gas 56 + nuclear 147 + hydro 256 + biomass 14) / 3,596 BU = 63.7 percent, against the model's firm minimum of 45 percent nationally. The model is energy-only, with no reliability test; the Energy security preset (55 percent firm) is the closest case.
- Source: PIB (Ministry of Power) 1936026 (28.06.2023: Guidelines for Resource Adequacy Planning Framework); CEA NGAP, PDF pp. 8, 54 and 56 (loss-of-load probability about 0.18 percent; planning reserve margin about 13 to 14 percent in 2035-36); the five Regional Power Conferences of 2025 (PIB 2124537, 2128438, 2130769, 2134584, 2139257: tie up capacity as per the adequacy plans); PIB 2243591 (22.03.2026, the ministerial meeting of the Bharat Electricity Summit).
- Size: not quantified (reliability, not energy).

**20. RE-INVEST 2024 commitments** (`re_invest_2024_commitments`). Sign +. Context.

- Mechanism: investment pledges by States, developers and manufacturers signal the renewable build rate to 2030.
- Treatment: context. Pledges are not a forecast metric and are not dated 2047. For scale only, the developers' 570 GW at the model's solar CUF would be at most 570 x 0.22 x 8.76 = 1,098.5 TWh a year, if all were built and all were solar. The manufacturing pledges are factory capacity a year, with no TWh equivalent. The model's near-term build rates come from CEA (the build limits).
- Source: PIB (MNRE) 2055789 (17.09.2024: an additional 570 GW from developers; 340 GW of modules, 240 GW of cells, 22 GW of wind turbines and 10 GW of electrolysers from manufacturers; Rs 32.45 lakh crore by 2030).
- Size: not quantified (capacity pledges).

**21. Critical minerals supply chain** (`critical_minerals_supply_chain`). Sign +. Context.

- Mechanism: concentrated supply of lithium, graphite, nickel, cobalt and rare earths can raise the cost or slow the build-out of solar, wind and batteries; domestic processing and recycling add a small industrial load.
- Treatment: context, supply side. No official source gives an electricity figure for mineral processing in India or an effect on the 2047 mix. In Part 2 the channel is the build-limit cases and the cost basis: a supply squeeze would push the plan towards the low build case and dearer storage, so the low build case stays a live sensitivity. The processing load is inside industrial growth in the trend.
- Source: G20 New Delhi Leaders' Declaration (Sep 2023), para 38, item 9; G7 Critical Minerals Action Plan (17.06.2025), endorsed by India; G20 Johannesburg Leaders' Declaration (Nov 2025), para 32; IEA, 2026 Ministerial Chair's Summary, paras 11-12 (intergovernmental, secondary); Ministry of Mines, National Critical Mineral Mission (Union Cabinet, 29.01.2025; PIB 2097308).
- Size: not quantified (mix and cost channel).

### 3.4 Distributed and behind-the-meter generation

**22. Rooftop solar** (`rooftop_solar`). Sign -. Layer.

- Mechanism: rooftop output is used on site and lowers the demand that utilities must meet; the NEP excludes it from the utility requirement.
- Treatment: applied as given; only the offset beyond today's, which the trend carries, counts. It assumes NPP utility generation excludes rooftop output.
- Source: CEA, 20th EPS, Sec. 1.5.2, p. 27 (50 GW by 2031-32, CUF 17 percent, 75 percent used on site, 55 BU offset); CEA, NEP 2022-32, p. 133 (rooftop 17.6 BU in 2026-27 and 55.8 BU in 2031-32, outside utility demand); MNRE, Renewable Energy Statistics 2023-24 (p. 27) and 2024-25 (p. 28); MNRE, Guidelines for PM-Surya Ghar: Muft Bijli Yojana (16.04.2024), pp. 5-6.
- Size: 24.7 / 24.7 / 134.9 TWh. Offset = GW x 8.76 x 0.17 x 0.75 (the EPS formula). FY2023-24 rooftop gives 13.3 TWh, which the trend carries to 36.7 TWh. Low = central: the EPS 2031-32 level held to 2047 (55.8 TWh). High: the official 2023-24 to 2031-32 path continued linearly to 2047 (141.0 TWh), a stated extrapolation. Each less the trend, x 1.2936. PM-Surya Ghar sits inside the EPS path.

**23. PM-KUSUM C pump solarisation** (`pm_kusum_c_pump_solarisation`). Sign -. Layer.

- Mechanism: solarised grid-connected farm pumps, individual and feeder-level, draw less from the grid.
- Treatment: applied as given. The FY2023-24 level is not in the sources, so it is taken as zero and the increment is slightly overstated. Feeder-level plants feed 11 kV feeders: if CEA counts them as utility generation, the effect on NPP generation is zero.
- Source: MNRE, Comprehensive guidelines for PM-KUSUM (17.01.2024), para 2.1, p. 2 (Component C: 35 lakh pumps, feeder-level included); NITI Aayog, Sectoral Insights: Power, Table 2.1, p. 48 (pumps solarised and sanctioned as on 30.12.2025); CEA, 20th EPS, Sec. 1.5.3, p. 28 (7.5 HP a pump, CUF 17 percent).
- Size: 13.0 / 37.7 / 38.6 TWh. Energy a pump = 7.5 HP x 0.746 kW/HP x 8,760 h x 0.17 = 8,332 kWh a year. Low: 12.0 lakh pumps solarised by 30.12.2025 (10.0 TWh); central: the 35 lakh target (29.2 TWh); high: 35.8 lakh sanctioned (29.9 TWh); each x 1.2936.

**24. PM-KUSUM A decentralised plants** (`pm_kusum_a_decentralised_plants`). Sign -. Context.

- Mechanism: small grid-connected plants on farm land sell to the distribution companies; they change who generates, not how much is needed.
- Treatment: context. CEA's 20th EPS counts only Component C as changing grid demand, and whether Component A output is inside NPP's utility series is not stated.
- Source: MNRE, PM-KUSUM guidelines, para 4.1, p. 3; CEA, 20th EPS, p. 28; NITI Aayog, Sectoral Insights: Power, Table 2.1, p. 48.
- Size: not quantified (supply side).

**25. PM-KUSUM B standalone pumps** (`pm_kusum_b_standalone_pumps`). Sign -. Context.

- Mechanism: off-grid solar pumps replace diesel pumps.
- Treatment: context. They replace diesel, not grid electricity, so CEA's 20th EPS finds no effect on utility generation.
- Source: MNRE, PM-KUSUM guidelines, para 2.1, p. 2 (14 lakh pumps); NITI Aayog, Sectoral Insights: Power, Table 2.1, p. 48; CEA, 20th EPS, p. 28.
- Size: not quantified: no grid effect.

**26. PM-KUSUM 2.0** (`pm_kusum_2`). Sign -. Context.

- Mechanism: a successor scheme with more feeder-level solarisation and agri-PV would raise distributed solar on farms.
- Treatment: context; a watch item. No Cabinet approval of a successor scheme was found on PIB or MNRE by 22 Nov 2026; an agri-PV component appears only in ministerial remarks reported by the press. If it is approved with official quantities, it would be sized like row 23 (7.5 HP, CUF 17 percent, x 1.2936).
- Source: MNRE, PM-KUSUM scheme page (no successor approval listed as of 22 Nov 2026).
- Size: not quantified: no approved quantity.

**27. Captive generation** (`captive_generation`). Sign +. Excluded by rule.

- Mechanism: plants inside industries, and shifts between captive and grid supply, change utility generation.
- Treatment: excluded by the organisers' written ruling of 16 November 2026 (NPP generation only, no captive generation; [decisions.md](decisions.md), D12). It is listed so the catalogue is complete. NITI Aayog expects a shift from captive to grid supply, which would raise NPP generation. The co-located (captive) half of hydrogen supply, any captive data-centre supply and the captive small reactors of the nuclear mission (row 28) are outside NPP for the same reason.
- Source: the organisers' ruling; CEA General Review 2025; NITI Aayog, Sectoral Insights: Power, Sec. 3.3.4, p. 61 (29.74 percent of industrial electricity was captive in 2023-24).
- Size: not quantified, by rule.

**28. Captive nuclear (BSR, BSMR)** (`captive_nuclear_bsr_bsmr`). Sign -. Excluded by rule.

- Mechanism: Bharat Small Reactors (220 MW PHWRs) and BSMR-200 and SMR-55 units built as captive plants for energy-intensive industry. If they replace grid purchases, NPP utility generation falls; if they replace captive coal, it is unchanged.
- Treatment: excluded by the organisers' written ruling (captive generation, row 27). It is listed so the catalogue is complete, and the same ruling keeps these units out of the Nuclear mission floor (row 13; [decisions.md](decisions.md), D15, ruling R31).
- Source: DAE, Lok Sabha USQ 2783 (17.12.2025), answers (c)-(d), PDF pp. 1-2; the CEA road map (row 12), Table 6.3, PDF p. 50 (BSR 5,060 MW and BSMR 5,000 MW) and the note to Table 5.3, PDF p. 41 ("Includes BSR/BSMR/SMR for captive uses").
- Size: not quantified, by rule. For scale only: 10.06 GW x 0.68 to 0.80 x 8.76 = 59.9 to 70.5 TWh a year; no official source says which use they would replace.

### 3.5 Efficiency

**29. UJALA LED lighting** (`ujala_led_lighting`). Sign -. Core.

- Mechanism: LED lamps replaced incandescent and CFL lamps, lowering lighting demand.
- Treatment: inside the core forecast. The savings were realised within the 2006-2024 estimation window, so the elasticity already reflects them. The replacement is largely complete, so the trend may embed a gain that will not recur (row 72).
- Source: BEE, India Energy Scenario for 2023-24 (2nd ed., Dec 2024), Table 25, p. 134 (182.00 BU saved in 2023-24 against BEE's no-programme counterfactual); BEE, Impact of Energy Efficiency Measures 2022-23, p. 3.
- Size: inside the history, not a feature.

**30. Standards and labelling** (`standards_and_labelling`). Sign -. Core.

- Mechanism: minimum energy performance standards and star labels for appliances.
- Treatment: inside the core forecast; continuing the programme at its past pace is part of the trend.
- Source: BEE, India Energy Scenario 2023-24, Table 25, p. 134 (89.84 BU saved in 2023-24).
- Size: inside the history.

**31. PAT scheme** (`pat_scheme`). Sign -. Core.

- Mechanism: Perform, Achieve and Trade targets for energy-intensive industries.
- Treatment: inside the core forecast. PAT is being folded into the carbon credit trading scheme (row 68).
- Source: BEE, India Energy Scenario 2023-24, Table 25, p. 134 (40.11 BU saved in 2023-24, cycles I to VI).
- Size: inside the history.

**32. ECBC buildings and others** (`ecbc_buildings_and_others`). Sign -. Core.

- Mechanism: the Energy Conservation Building Code, star rating of buildings and efficient street lighting.
- Treatment: inside the core forecast.
- Source: BEE, India Energy Scenario 2023-24, Table 25, p. 134 (star rating 0.64 BU and street lighting 8.8 BU in 2023-24; the ECBC row has no electricity figure). All BEE schemes together saved 321.39 BU in 2023-24 and 307.33 BU in 2022-23.
- Size: inside the history.

**33. Efficiency beyond trend** (`efficiency_beyond_trend`). Sign -. Layer.

- Mechanism: appliance efficiency faster than current policy lowers residential demand.
- Treatment: applied as given. Low and central are 0, because NITI Aayog's Current Policy Scenario represents current policies and past trends. High is the gap between NITI's Net Zero and Current Policy scenarios. Cooling is left out here (row 48). BEE's UNNATEE target (285 BU in 2031) is already in the 20th EPS baseline, and its part beyond the model's trend cannot be separated, so it is not added.
- Source: NITI Aayog (Feb 2026), Sectoral Insights: Buildings, Sec. 4.5.1, pp. 79 and 81 (lighting 66 / 62, heating 96 / 76 and other appliances 277 / 192 TWh in 2050, Current Policy / Net Zero); NITI Aayog, Sectoral Insights: Power, p. 58; CEA, 20th EPS, Sec. 1.5.6, p. 30. Conference context: UNFCCC decision 1/CMA.5 (para 28(a)), the G20 New Delhi declaration (para 38, item 7), the G20 Energy Transitions Ministerial of 2024 and the IEA's efficiency conferences of 2025 and 2026 (Brussels, Montreal) set global doubling targets for primary-energy intensity, not electricity, with no India figure; India did not endorse the IEA's 2025 and 2026 statements. The high case stays NITI's Net Zero gap.
- Size: 0 / 0 / 125.3 TWh: the Net Zero gap of 109 TWh in 2050, x 24/27 and x 1.2936.

**34. National efficiency targets** (`national_efficiency_targets`). Sign -. Context.

- Mechanism: the NDC emissions-intensity targets and BEE's national efficiency targets push demand down.
- Treatment: context. No official 2047 electricity figure beyond the trend can be separated. The current NDC (2031-35) is economy-wide and "does not commit the country to any specific emission reduction obligations for individual sectors" (PDF p. 11), and its 60 percent is installed capacity, not generation. For comparison, CEA's NGAP plan for 2035-36 is about 48 percent non-fossil in generation (1,721 of 3,596 BU).
- Source: India's Nationally Determined Contribution 2031-2035 (UNFCCC, April 2026; approved by the Union Cabinet, PIB 2245209, 25.03.2026), section 3.0, items 3 and 4, PDF p. 10 (emissions intensity of GDP 47 percent below 2005 by 2035; about 60 percent non-fossil installed capacity by 2035), PDF p. 8 and PDF p. 34 (informed by the first global stocktake); India's Updated First NDC (Aug 2022), item 3 (45 percent cut in emissions intensity by 2030 from 2005); the Prime Minister's national statement at COP26 (1 Nov 2021); CEA, 20th EPS, p. 30 (UNNATEE, 285 BU in 2031).
- Size: not quantified.

**35. Smart metering (RDSS)** (`smart_metering_rdss`). Sign -. Context.

- Mechanism: smart and prepaid meters under the Revamped Distribution Sector Scheme cut commercial losses and enable time-of-day billing; lower losses reduce generation for the same use.
- Treatment: inside row 6, which is already an upper bound that may be inside the trend (ruling R22). AT&C loss is not the same measure as T&D loss, so it is not converted. Time-of-day billing is row 65.
- Source: PIB (Ministry of Power) 2215761 (18.01.2026: AT&C losses from 22.62 percent in FY2013-14 to 15.04 percent in FY2024-25), 1779726 (the RDSS target of 12 to 15 percent), 2130769 and 2139257 (prepaid smart meters in Government establishments by August 2025).
- Size: not quantified: inside T&D losses.

**36. Sustainable lifestyles (Mission LiFE)** (`sustainable_lifestyles_life`). Sign -. Context.

- Mechanism: behaviour change (switching off, efficient use of appliances and cooling, public transport) lowers electricity use at the same income.
- Treatment: context. No official source sizes LiFE savings for 2047. Behaviour is partly inside the history, and NITI Aayog's Net Zero case assumes lower demand, so any effect beyond the trend is inside row 33's high value; it is not added again.
- Source: G20 New Delhi Leaders' Declaration (Sep 2023), para 36 and item 36.1 (High-Level Principles on Lifestyles for Sustainable Development); CEM15 Outcomes Document, PDF p. 12; CEM16 Outcomes Document, PDF p. 6 (Declaration on Sustainable Lifestyles and Fairness, endorsed by India); India's NDC 2031-35, PDF p. 6.
- Size: not quantified.

### 3.6 Electrification and new loads

**37. Transport electrification** (`transport_electrification`). Sign +. Layer.

- Mechanism: battery electric road vehicles, rail and metro traction add grid load.
- Treatment: applied as given; only the part beyond the trend counts (2047 level less FY2023-24 traction carried by the trend). Rail and metro are inside this row, so rows 50 and 51 are not added on top.
- Source: NITI Aayog (Feb 2026), Sectoral Insights: Transport, Table 4.6, p. 92 and p. 93 (transport electricity 47 Mtoe in the Current Policy Scenario and 48 Mtoe in Net Zero, 2050); NITI Aayog, Sectoral Insights: Power, p. 71; CEA General Review 2025, Table 9.1, p. 137 (railways and tramways 33.92 TWh in FY2023-24); CEA, 20th EPS, p. 26 (a lower near-term path). Conference context: UNFCCC decision 1/CMA.5 (para 28(g)) gives no India quantity, and biofuels under the Belém 4X pledge on sustainable fuels (co-launched by India in October 2025) could replace part of road electrification; neither is sized.
- Size: 511.9 / 511.9 / 525.2 TWh. 1 Mtoe = 11.63 TWh, so 546.6 and 558.2 TWh in 2050, and 489.6 and 500.0 TWh in 2047 by 24/27. The trend carries 33.92 x 2.7697 = 93.9 TWh. Each less the trend, x 1.2936. Only two official figures exist, so low = central.

**38. EV sales targets** (`ev_sales_targets`). Sign +. Context.

- Mechanism: an official target for the share of electric vehicles in sales would set the pace of transport electrification.
- Treatment: inside row 37. The often-quoted "30 percent of sales by 2030" is not an official target: the Ministry of Heavy Industries states that none has been set. Transport stays sized by NITI Aayog's scenarios.
- Source: Ministry of Heavy Industries, Lok Sabha USQ 4155 (19.08.2025), PDF p. 1: "Government has not set any annual sales target of EVs and hybrid electric vehicles".
- Size: not quantified: no official target.

**39. Green hydrogen** (`green_hydrogen`). Sign +. Layer.

- Mechanism: electrolysers add a new load. Only grid-supplied electricity is in NPP's scope.
- Treatment: applied as given; all of it is beyond the trend, because there was no green hydrogen in FY2023-24. Half the electricity is taken from the grid (CEA's 20th EPS; the co-located half is captive and outside NPP). With all supply from the grid the values would double.
- Source: MNRE, National Green Hydrogen Mission (Jan 2023), p. 24 (at least 5 MMT a year by 2030); NITI Aayog (Feb 2026), An Overview, p. 101 (470 TWh in the Current Policy Scenario, 1,392 TWh in Net Zero, 2050); NITI Aayog, Sectoral Insights: Power, p. 62; CEA, 20th EPS, Sec. 1.5.4, p. 29 (50 kWh per kg; half on the grid). Conference context: UNFCCC decision 1/CMA.5 (para 28(c) and (e)), the G20 New Delhi declaration (para 38, item 3), the COP29 Hydrogen Declaration and the Belém 4X pledge, co-launched by India, support the high case but give no India number; no value changes.
- Size: 137.9 / 230.4 / 682.3 TWh. Low: the 2030 target held (5 MMT x 50 kWh/kg = 250 TWh); central: 470 x 24/27 = 417.8 TWh; high: 1,392 x 24/27 = 1,237.3 TWh. Each x 0.5 grid share and x 1.1029 (connected at extra-high voltage). NITI calls its Current Policy figure additional generation, so the gross-up may double count about 10 percent there.

**40. Green hydrogen hubs** (`green_hydrogen_hubs`). Sign +. Context.

- Mechanism: three port-based hubs concentrate electrolyser load on the Western, Southern and Eastern coasts.
- Treatment: inside row 39, which holds the quantity. Regional note: ruling R23 spreads features by each region's share of the requirement, while the hubs place electrolyser load at ports in Gujarat (Western), Tamil Nadu (Southern) and Odisha (Eastern).
- Source: PIB Backgrounder, Unlocking India's Green Hydrogen Production Potential (12.11.2025), PDF p. 3 (Deendayal, V.O. Chidambaranar and Paradip ports recognised as hubs in October 2025).
- Size: not quantified separately.

**41. Data centres** (`data_centres`). Sign +. Layer.

- Mechanism: data-centre IT and cooling load (IT load x utilisation x PUE), including AI and GPU data centres.
- Treatment: applied as given; only the part beyond the 2024 level carried by the trend counts. Operators also use captive and group-captive supply, which would be outside NPP. The Ministry of Power says the AI load is expected to be integrated into the grid, and no official captive share exists, so none is removed. AI compute (row 43) and government data centres (row 47) are inside this row.
- Source: NITI Aayog (Feb 2026), An Overview, Sec. 5.4.4, p. 143 (2047: 65 GW of IT load at 50 percent utilisation and PUE 1.4, about 45 GW on average, which is 394 TWh; growth from 2030 to 2047 "due to AI integration"); NITI Aayog, Sectoral Insights: Power, Sec. 3.3.6, p. 63 (1.4 GW in 2024, 12.26 TWh a year) and pp. 71-72 (about 400 TWh by 2050 and 700 TWh by 2070, steady across all hours); NITI Aayog, Sectoral Insights: Buildings, p. 73 (data-centre IT load and utilisation in 2030 and 2050); Ministry of Power, Rajya Sabha Unstarred Question 956, answered 27.07.2026, part (a), p. 32 ("an additional load of 26.3 GW from Artificial Intelligence (AI) Data Centres is projected by 2031-32, which is expected to be integrated into the grid"); MeitY, Rajya Sabha replies of 07.08.2026 (PIB 2298108: CEA, about 17 GW of data-centre demand up to 2031-32, 148.9 TWh as average load) and 13.03.2026 (PIB 2239616: Ministry of Power, 13.56 GW by 2031-32, 118.8 TWh as average load); CEA, 20th EPS, Sec. 1.5, pp. 26-30 (its emerging loads do not include data centres). Conference context (intergovernmental, secondary; a cross-check only): the IEA's 2026 ministerial chair's summary (paras 20-21), its Montreal Action Plan (para 10b) and its report Energy and AI (April 2025: about 415 TWh worldwide in 2024 and 945 TWh in 2030) give no India figure, and the COP29 Declaration on Green Digital Action has none either, so no value changes. The Union Budget 2026-27's tax holiday to 2047 for data-centre exports (row 42) supports the high case without a quantity.
- Size: 356.2 / 397.1 / 697.8 TWh. The high is our extrapolation of an official 2031-32 figure (the Ministry of Power's 26.3 GW), not an official 2047 figure. Average load in GW x 8.76 = TWh a year, NITI Aayog's own conversion (45 GW = 394 TWh, a flat load around the clock). The trend carries 12.26 x 2.7697 = 34.0 TWh. Low: the 2050 value scaled to 2047 (356.9 TWh). Central: 394 TWh in 2047 (the Overview). High: the Ministry of Power's 26.3 GW in 2031-32, read as average load, is 230.4 TWh; the official FY2023-24 to FY2031-32 path continued linearly to FY2047-48, as rooftop solar's high is, gives 12.26 + (230.4 - 12.26) x 24/8 = 666.6 TWh (76.1 GW), still below NITI's 700 TWh for 2070. This continuation is ours, stated. Each less the trend, x 1.1029. The 26.3 GW is the only official figure that separates AI data centres; it is read as the whole data-centre level in 2031-32, not as an addition to CEA's 17 GW. CEA's 17 GW (148.9 TWh) continued the same way would give a milder high of 428.2 TWh. Read as sanctioned capacity at NITI's 50 percent utilisation, the 26.3 GW would give 316.6 TWh, below the central, and the high would stay 397.1 ([decisions.md](decisions.md), D14, ruling R30).

**42. Data-centre tax holiday to 2047** (`data_centre_tax_holiday_2047`). Sign +. Context.

- Mechanism: a tax holiday to 2047 for foreign companies that serve global customers from Indian data centres encourages export-oriented data-centre load.
- Treatment: inside row 41. It supports the data-centre high (697.8 TWh, itself our extrapolation of an official 2031-32 figure) but gives no MW or TWh, so nothing is added.
- Source: Union Budget 2026-27, Budget Speech, paras 130-131, PDF p. 28.
- Size: not quantified.

**43. AI compute (GPUs)** (`ai_compute_gpus`). Sign +. Context.

- Mechanism: GPU compute for AI training and inference (the IndiaAI common compute pool and private AI clusters) adds data-centre load.
- Treatment: inside row 41. The GPUs are hosted in empanelled data centres, NITI Aayog attributes its 2030-2047 growth in IT load to AI integration, and the Ministry of Power's 26.3 GW of AI data-centre load (230.4 TWh a year as average load) sets the data-centre high. Counting AI compute again would double count.
- Source: MeitY, Rajya Sabha reply of 13.03.2026 (PIB 2239616: about 38,231 GPUs onboarded through 14 empanelled service providers and data centres, at Rs 65 per GPU-hour); PIB Backgrounder, India's AI Moment (17.02.2026, PIB 2229397: 20,000 more GPUs, from 38,000 to more than 58,000); Ministry of Power, Rajya Sabha Unstarred Question 956 (27.07.2026), p. 32. Conference context (IEA, intergovernmental, secondary; COP29 Declaration on Green Digital Action): no India figure, so no value changes.
- Size: not quantified: no official kW per GPU, utilisation or PUE exists for the IndiaAI pool.

**44. Telecom networks** (`telecom_networks`). Sign +. Context.

- Mechanism: mobile towers, 4G and 5G base stations, core networks and broadband equipment draw power. Traffic growth and 5G densification raise it; efficiency and on-site renewables lower grid draw.
- Treatment: inside the trend. Telecom growth over 2006-2024 is in the history, and no TRAI or DoT projection of telecom electricity exists, so no increment beyond the trend can be sized. A shift of towers from diesel to the grid would raise grid demand, and the National Broadband Mission 2.0 renewable target would lower it; neither is quantified officially.
- Source: TRAI, Recommendations on Approach towards Green Telecommunications (12.04.2011), p. 8, para (vi) (about 400,000 towers at an average 3 to 4 kW) and p. 21, para 1.17; DoT, 2025 Year End Review (PIB 2206477, 19.12.2025; pages of DoT's PDF copy), p. 8 (about 8.40 lakh towers with about 31.31 lakh base stations), p. 11 (5.08 lakh 5G base stations on 31.10.2025) and p. 14 (30 percent of towers on sustainable energy by 2030); TRAI, Press Release 140/2026 (27.10.2026), pp. 6-7 and 11 (1,300.00 million wireless subscriptions, including 144.85 million M2M).
- Size: not quantified. For scale only, not a layer value: 8.40 lakh towers x 3 kW (TRAI's 2011 average) x 8.76 = 22.1 TWh a year, about 1.3 percent of FY2023-24 generation, which the trend carries to about 61 TWh in 2047. The 3 kW is a 2011 figure and part of tower energy is diesel or on-site renewables, so this overstates grid draw. TRAI's own 11,169 million kWh does not match its 400,000 towers x 3 kW x 8,760 h (10,512 million kWh).

**45. Consumer electronics and devices** (`consumer_electronics_devices`). Sign +. Context.

- Mechanism: televisions, computers, laptops, Wi-Fi routers and phone charging in homes and offices. More devices per household raise demand; labelled efficiency lowers it.
- Treatment: inside the trend and NITI Aayog's Current Policy appliance path. A television in NITI's Current Policy case uses 80 W x 1,850 h = 148 kWh a year (urban, 2020) and 65 W x 1,850 h = 120 kWh in 2050 (-19 percent), while ownership saturates. TV electricity therefore grows more slowly than total generation, and any effect beyond the trend lowers generation; NITI's faster Net Zero appliance efficiency is inside row 33's high. For phones, computers and routers no official per-device or total figure exists: BEE has no phone label, its laptop label is voluntary with no labelled production in 2023-24, and NITI Aayog leaves computers and routers out as small.
- Source: BEE, India Energy Scenario 2023-24, Table 17, p. 101 (colour televisions 5.15 TWh in 2023-24; 302 of 500 TWh of building use "to be estimated") and Table V, pp. 152-153 (labelled production in 2023-24: 6,414,183 colour televisions, no computers); NITI Aayog, Sectoral Insights: Buildings, p. 70 (computers and Wi-Fi routers "excluded due to data limitations and their relatively small impact on overall demand") and Annexure B, pp. 123-125 (television ownership, sets, hours and wattage in 2020 and 2050); TRAI, Press Release 140/2026, pp. 1 and 7 (1,300.00 million wireless subscriptions, about 1,155 million excluding M2M).
- Size: not quantified. Televisions used 5.15 TWh in 2023-24, which the trend already carries (5.15 x 2.7697 = 14.3 TWh in 2047).

**46. Semiconductor and electronics manufacturing** (`semiconductor_electronics_manufacturing`). Sign +. Context.

- Mechanism: semiconductor fabs, assembly and test (ATMP and OSAT) units and electronics assembly add industrial load (clean rooms, process tools, ultra-pure water).
- Treatment: context. Industrial growth is in the trend, no official electricity figure exists for any approved unit, and the scale is small against a 4,688 TWh median.
- Source: Union Cabinet, Semicon 2.0 approval and progress of ISM 1.0 (PMIndia, 15.07.2026: 12 units approved with over Rs 1.64 lakh crore, namely a silicon fab, a silicon carbide fab, a GaN micro-LED display fab and nine packaging units; the first fab to be commissioned in 2028); Union Cabinet, three semiconductor units (PMIndia, 29.02.2024: the Tata-PSMC fab at Dholera, 50,000 wafer starts a month).
- Size: not quantified: the Cabinet releases give investment and wafer or chip capacity, not MW or TWh. Per-fab power figures in the press are industry estimates and are not used.

**47. Digital public infrastructure** (`digital_public_infrastructure`). Sign +. Context.

- Mechanism: government data centres and cloud for e-governance, Aadhaar, UPI and other digital public platforms.
- Treatment: inside row 41. Government platforms run in NIC and empanelled cloud data centres, which are inside the national data-centre totals.
- Source: PIB Explainer, Data Centres in India: Infrastructure for the Digital Age (14.09.2026, NoteId 159969: NIC national data centres at Delhi, Pune, Hyderabad and Bhubaneswar and 37 smaller ones in state capitals). Its data-centre capacity figures give no utilisation, so they are neither converted nor used.
- Size: not quantified: no separate official electricity figure.

**48. Space cooling** (`space_cooling`). Sign +. Layer.

- Mechanism: air conditioners, coolers and fans in homes and commercial buildings, rising with income and heat.
- Treatment: low and central are 0, because cooling that grows with income is in the trend. High is the India Cooling Action Plan's growth above the trend. It is an upper bound: cooling also grew faster than total demand in 2006-2024, and that is inside the elasticity. Hotter weather is part of the projection, so row 57 is not added on top.
- Source: MoEFCC Ozone Cell, India Cooling Action Plan (Mar 2019), Figure D and the summaries of Chapters 3 and 4, p. 17 (space cooling about 135 TWh in 2017-18 and about 585 TWh in 2037-38 in the reference case; the intervention case about 30 percent lower); NITI Aayog, Sectoral Insights: Buildings, p. 79. Conference context: India was not a launch endorser of the COP28 Global Cooling Pledge (press, context only) or of the IEA's Brussels statement (June 2025) on doubling the efficiency of new air conditioners by 2030; at CEM16 (August 2025) India called for efficient air conditioners at scale "to tackle heat stress and optimize power demand" (Outcomes Document, PDF pp. 3 and 6). The Cooling Action Plan's intervention case remains the official Indian lower path.
- Size: 0 / 0 / 252.3 TWh. FY2023-24 cooling = 135 x (585/135)^(6/20) = 209.6 TWh, which the trend carries to 389.9 TWh in FY2037-38. The plan's excess in 2037-38 (195.1 TWh) is held flat to 2047 and x 1.2936. The intervention case would cut the high value to roughly 25 TWh.

**49. Electric cooking** (`electric_cooking`). Sign +. Context.

- Mechanism: a shift from LPG and biomass to electric cooking.
- Treatment: context. NITI Aayog gives only shares and a chart without printed values, and the only TWh figures (TERI) are from a think tank.
- Source: NITI Aayog, Sectoral Insights: Power, Table 3.3, p. 61 (cooking under 1 percent electric today; 30 percent in the Current Policy Scenario and 60 percent in Net Zero by 2070); NITI Aayog, Sectoral Insights: Buildings, Fig. 4.9, p. 87. Conference context: the G20 energy ministerials of 2024 and 2025 and CEM16 (a clean cooking initiative with India) back clean cooking access, COP30 decision 2/CMA.7 (para 12(o)-(p)) names it, and the 2026 G20 statement favours LPG, which has no grid effect; none gives an India electricity quantity.
- Size: not quantified: no official TWh figure.

**50. Railway traction** (`railway_traction`). Sign +. Context.

- Mechanism: railway electrification and traffic growth raise traction demand.
- Treatment: inside row 37 (NITI Aayog's transport electricity includes rail). Railways buying through open access stay inside NPP generation.
- Source: CEA General Review 2025, Table 9.1, p. 137 (33,917.67 GWh, 2.51 percent of utility sales, +12.96 percent); NITI Aayog, Sectoral Insights: Power, p. 59; BEE, Impact of Energy Efficiency Measures 2022-23, p. 184 (non-traction use).
- Size: not quantified separately: counted in transport electrification.

**51. Metro rail** (`metro_rail`). Sign +. Context.

- Mechanism: urban metro networks add traction load.
- Treatment: inside row 37 and CEA's railways and tramways category. No official national metro electricity figure was found.
- Source: NITI Aayog, Sectoral Insights: Power, Table 3.2, p. 60 (metro modal share 1 percent in 2024, 2 percent in 2050).
- Size: not quantified separately.

**52. Desalination** (`desalination`). Sign +. Context.

- Mechanism: seawater desalination for urban and industrial water.
- Treatment: context.
- Source: no official Indian figure for desalination electricity to 2047 was found in the CEA, NITI Aayog and MoHUA material searched.
- Size: not quantified: no official figure.

**53. Industrial electrification (green steel)** (`industrial_electrification_green_steel`). Sign +. Context.

- Mechanism: electric process heat, heat pumps, scrap-based electric arc furnaces and hydrogen-based steel shift industry towards electricity.
- Treatment: context. Industry is 33.3 percent of utility sales and its growth is inside the trend. The official figures include captive supply, which is excluded by rule, and give no grid-only increment for 2047. Hydrogen for steel is inside row 39.
- Source: Ministry of Steel, Greening the Steel Sector in India (Sept 2024), Executive Summary, p. 21 (steel electricity 94 TWh in 2021-22 and 184 TWh in 2030-31, mostly captive in integrated plants); NITI Aayog, Sectoral Insights: Power, Table 3.4, p. 62 and p. 70; CEM15 Outcomes Document, PDF p. 12 (green public procurement of low-emission steel and cement); Ministry of Steel, Green Steel Taxonomy for India (released 12 Dec 2024, notified 23 Dec 2024). Row 69 gives the EU CBAM channel.
- Size: not quantified: the grid share beyond the trend cannot be separated.

**54. Agriculture pumping** (`agriculture_pumping`). Sign +. Context.

- Mechanism: irrigation pumping grows with groundwater depth and cropping; efficiency and solar pumps lower it.
- Treatment: inside the trend. Solar pumps are row 23; a dry year is row 60.
- Source: CEA General Review 2025, Table 9.1, p. 137 (agriculture 275,484.96 GWh, 20.40 percent of utility sales, +12.97 percent); NITI Aayog, Sectoral Insights: Power, pp. 59 and 70.
- Size: not quantified: inside the history.

**55. Cold chain refrigeration** (`cold_chain_refrigeration`). Sign +. Context.

- Mechanism: pack-houses, cold storage, refrigerated transport and commercial refrigeration.
- Treatment: context: small against the 2047 total, and its growth with farm income is inside the trend.
- Source: MoEFCC Ozone Cell, India Cooling Action Plan, Chapter 4 summary, p. 17 (pack-houses 2.4 TWh in the next decade and 5.2 TWh in the decade after).
- Size: not quantified.

**56. Electricity share of energy in 2047** (`electricity_share_of_energy_2047`). Sign +. Context.

- Mechanism: raising electricity's share of final energy from 22 percent to an aspired 40 percent by 2047 shifts fuels to electricity.
- Treatment: context. The same source gives no 2047 total of final energy, so the share cannot be converted to TWh; its electrification content is sized through rows 33, 37 and 39.
- Source: PIB (NITI Aayog) 2226010 (10.02.2026; the Secretary (Power): "Electricity is 22% of the current basket and we aspire to reach 40% by 2047"). The aspiration is also in `data/external/techno_economic/policy_targets.csv`.
- Size: not quantified: an aspiration with no energy total.

### 3.7 Climate and weather

**57. Warming (cooling degree days)** (`warming_cooling_degree_days`). Sign +. Context.

- Mechanism: warmer years raise cooling demand.
- Treatment: context. No official Indian estimate of electricity per cooling degree day was found; any value would come from the literature only. Warming is part of the cooling projections, so it is inside row 48's high value.
- Source: MoES (2020), Assessment of Climate Change over the Indian Region, Executive Summary, pp. 10-12 (about 0.7 C of warming over 1901-2018; about 4.4 C by 2100 under RCP8.5; summer heat waves three to four times more frequent by 2100); NITI Aayog, Sectoral Insights: Buildings.
- Size: not quantified.

**58. Monsoon hydro variability** (`monsoon_hydro_variability`). Sign +. Context.

- Mechanism: a weak monsoon lowers hydro output and raises pumping demand.
- Treatment: context. Hydro variability changes the mix (Part 2), not the total requirement; the extra pumping of dry years is inside the core forecast's persistent model error.
- Source: NITI Aayog, Sectoral Insights: Power, p. 66 (large-hydro PLF 32 to 40 percent over five years); CEA General Review 2025, Table 6.5.2, p. 109 (hydro 134,053.93 GWh in FY2023-24).
- Size: not quantified.

### 3.8 Shocks

**59. Extreme heat year** (`extreme_heat_year`). Sign +. Driver.

- Mechanism: a hot year raises demand above the trend; FY2023-24 was 0.072 log points above the fitted relation.
- Treatment: an existing uncertainty. Weather-year shocks enter through the AR(1) model error of the core forecast (persistence 0.51, residual s.d. 0.037; [methodology.md](methodology.md), section 2.5).
- Source: the model's own estimate on NPP data (`outputs/india_fit.csv`); MoES (2020), Executive Summary, p. 12 (heat waves).
- Size: not a feature.

**60. Drought** (`drought`). Sign +. Context.

- Mechanism: drought years raise irrigation pumping and cut hydro.
- Treatment: context: part of the model error, not quantified separately.
- Source: CEA General Review 2025, Table 9.1, p. 137 (agriculture +12.97 percent in FY2023-24).
- Size: not quantified.

**61. Pandemic** (`pandemic`). Sign -. Core.

- Mechanism: a pandemic year depresses demand (FY2020-21).
- Treatment: estimated as a dummy in the core regression (-0.055 log points). No pandemic is assumed for 2047.
- Source: the model's own estimate on NPP data (`outputs/india_fit.csv`, row `covid_fy2020`).
- Size: inside the estimation, not a feature.

**62. Energy security and fuel-price shocks** (`energy_security_fuel_price_shock`). Sign +. Context.

- Mechanism: an oil and gas supply shock, such as the 2026 global energy crisis, raises fuel prices. It makes gas-fired power dearer, can speed the shift of transport and cooking to electricity, and can slow GDP growth.
- Treatment: context. The GDP channel is inside the core growth presets (Low growth); gas in the mix is a setting (the Energy security policy caps gas at 2 percent, and the Robust policy keeps 51.1 TWh for peaking, which an LNG shock would make dearer); transport is row 37. The FY2022-23 price spike is inside the history. The net sign is not established; +1 is the substitution channel.
- Source: IEA, 2026 Ministerial Chair's Summary, paras 7-9, and the Montreal Action Plan of its 11th Global Conference on Energy Efficiency (29-30 Jun 2026), paras 1, 3 and 10b (intergovernmental, secondary); the White House, G20 Energy Abundance Ministerial (Houston, 16.09.2026); G20 Energy Transitions Ministerial, Chair's Statement (10.10.2025), para 9.
- Size: not quantified: the official texts give no India quantity.

### 3.9 Prices and policy

**63. Electricity tariffs** (`electricity_tariffs`). Sign -. Context.

- Mechanism: higher real tariffs lower demand.
- Treatment: context. Tariffs are set by CERC and the state commissions, and no official projection of real tariffs to 2047 was found.
- Source: no official source found in the CEA, CERC, NITI Aayog and BEE documents searched.
- Size: not quantified: literature only.

**64. Price elasticity** (`price_elasticity`). Sign -. Context.

- Mechanism: the own-price elasticity of electricity demand.
- Treatment: context. No official estimate from CEA, CERC, RBI or NITI Aayog was found.
- Source: no official source found.
- Size: not quantified: literature only.

**65. Time-of-day tariffs** (`time_of_day_tariffs`). Sign -. Context.

- Mechanism: tariffs 10 to 20 percent lower in solar hours and 10 to 20 percent higher at peak shift consumption into solar hours. Total energy is roughly unchanged, but less storage cycling is needed.
- Treatment: context. No official load-shift quantity exists. It suggests that the Part 2 storage back-calculation, which uses today's load shape, may overstate 2047 storage, and that row 8 sits towards its low end under a strong response.
- Source: PIB (Ministry of Power) 1934673 (23.06.2023: Electricity (Rights of Consumers) Amendment Rules 2023; from 1 April 2024 for commercial and industrial consumers of 10 kW and above, and from 1 April 2025 for others except agriculture).
- Size: not quantified: a change of load shape, not of energy.

**66. Fossil-fuel subsidy phase-out** (`fossil_fuel_subsidy_phaseout`). Sign +. Context.

- Mechanism: phasing out inefficient fossil-fuel subsidies raises the price of LPG, kerosene and diesel relative to electricity, which favours electric cooking, pumping and vehicles; subsidies that address energy poverty are exempt.
- Treatment: context. The diesel and kerosene reforms of the 2010s are inside the history. LPG support for poor households is outside the call, and the 2026 G20 statement promotes LPG for clean cooking, which works the other way. No official projection of fuel subsidies or relative prices to 2047 exists.
- Source: UNFCCC decision 1/CMA.5, para 28(h), PDF p. 6; G20 New Delhi Leaders' Declaration (Sep 2023), para 38, item 12.
- Size: not quantified.

**67. LNG and energy independence** (`lng_and_energy_independence`). Sign +. Context.

- Mechanism: the aim of energy independence, and of meeting 15 percent of total energy demand with LNG, bears on gas availability and price for power.
- Treatment: context. The 15 percent is a share of primary energy, not a power target. NGAP has gas at 56 BU (2 percent) of generation in 2035-36, and CEA's 2047 mix has 10.4 GW of gas, 14.0 TWh at the NEP gas CUF (row 12). The model's gas cap (5 percent) is unchanged.
- Source: PIB (PMO) 2218984 (27.01.2026, the Prime Minister at India Energy Week 2026).
- Size: not quantified (primary energy, not generation).

**68. Carbon market (CCTS)** (`carbon_market_ccts`). Sign -. Context.

- Mechanism: a carbon price on obligated industries favours efficiency and cleaner supply.
- Treatment: context. No official electricity-demand impact is published; it works through PAT's successor and through generation costs (the mix's carbon-price setting).
- Source: Ministry of Power, Carbon Credit Trading Scheme 2023, Gazette S.O. 2825(E), 28 Jun 2023, p. 6; Ministry of Commerce and Industry, India-EU Free Trade Agreement concluded (PIB, 27.01.2026), PDF p. 3: technical cooperation on recognising carbon prices and verifiers under the EU CBAM (row 69), with no electricity quantity.
- Size: not quantified.

**69. EU CBAM (steel, aluminium)** (`eu_cbam_steel_aluminium`). Sign +. Context.

- Mechanism: the EU Carbon Border Adjustment Mechanism prices the embedded emissions of imported iron and steel, aluminium, cement, fertilisers, hydrogen and electricity. For steel, aluminium and hydrogen only direct emissions count, so it favours electric routes (scrap electric arc furnaces, hydrogen-based iron) over blast furnaces and raises industrial electricity use.
- Treatment: context. It applies only to exports to the EU, and no official Indian figure gives the tonnage that would switch route or its electricity. Integrated steel plants and aluminium smelters run largely on captive power, outside NPP by the organisers' ruling. Hydrogen for steel is inside row 39; row 53 gives the scale (steel electricity 94 TWh in 2021-22 and 184 TWh in 2030-31, mostly captive).
- Source: Regulation (EU) 2023/956, Art. 3(34), Art. 7(1) and Annex II, Art. 30(2)(a), Art. 32 and Art. 36(2) (financial obligation from 1 Jan 2026); European Commission proposal 2025/0419(COD) (17.12.2025: downstream steel and aluminium goods from 2028); Ministry of Commerce and Industry, India-EU Free Trade Agreement concluded (PIB, 27.01.2026), PDF p. 3; Ministry of Steel, Green Steel Taxonomy for India (released 12 Dec 2024, notified 23 Dec 2024).
- Size: not quantified: no official tonnage or grid share.

**70. Net zero 2070** (`net_zero_2070`). Sign +. Context.

- Mechanism: the net-zero commitment pushes electrification, which raises demand, and efficiency, which lowers it.
- Treatment: context. It shows up through the Net Zero (high) values of rows 33, 37 and 39. NITI Aayog's scenario totals include captive use and are consumption-side, so they are context only.
- Source: the Prime Minister's national statement at COP26 (1 Nov 2021); NITI Aayog, Sectoral Insights: Power, Table E.1, p. 24. Conference context: UNFCCC decision 1/CMA.5 (para 28(c)-(d)) and the IEA's 2024 ministerial communique (paras 3 and 8) call for net-zero energy systems by around mid-century; COP30 (Belém, 2025) did not repeat the fossil-fuel wording and refers only to the UAE Consensus (1/CMA.7, para 15(f); 3/CMA.7). No conference text gives an India quantity.
- Size: not quantified separately.

**71. RPO and RCO trajectory** (`rpo_rco_trajectory`). Sign +. Context.

- Mechanism: renewable consumption obligations set the supply mix, not the quantity.
- Treatment: context (supply side). Its distributed renewable component overlaps rows 22 and 23.
- Source: Ministry of Power, Gazette S.O. 4617(E), 20 Oct 2023, p. 3 (total obligation 43.33 percent in 2029-30, including 4.50 percent distributed renewables). Conference context: tripling renewable capacity worldwide by 2030 (UNFCCC decision 1/CMA.5, para 28(a); G20 New Delhi, para 38, item 5; G20 Johannesburg, para 25) is a global capacity goal that sets the mix, not the quantity; India did not endorse the COP28 Global Renewables and Energy Efficiency Pledge.
- Size: not quantified.

### 3.10 Historical trend effects

**72. Trend carry-over (losses, deficit)** (`trend_carryover_losses_deficit`). Sign +. Context.

- Mechanism: the 2006-2024 trend embeds one-off gains that cannot recur at the same pace: falling losses, closing supply deficits and the LED roll-out.
- Treatment: a diagnostic, not added. Falling T&D losses lowered generation growth by about 0.144 log points over the window, so the trend may understate future generation if losses cannot keep falling. Closing supply deficits raised it, so the trend may overstate. The net direction is not established from official figures alone. This row is why T&D losses (row 6) are flagged as overlapping.
- Source: CEA, Growth of Electricity Sector in India 1947-2025, Chart 29, p. 58 (T&D losses 28.65 percent in 2006-07, 17.63 percent in 2023-24); NITI Aayog, Sectoral Insights: Power, p. 37 (energy deficit 11.1 percent in 2008-09, 0.1 percent in 2024-25); BEE, India Energy Scenario 2023-24, Table 25, p. 134.
- Size: not quantified.

**73. Household access and supply hours** (`household_access_supply_hours`). Sign +. Context.

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
| Electrification high | Transport, green hydrogen, data centres and space cooling high; the rest central | +2,140.6 | 5,331 | 6,828 | 9,161 | +45.7% |
| Efficiency and distributed | Every row that lowers generation at its high value (T&D losses included, at their upper bound); the rest central | +354.6 | 3,545 | 5,042 | 7,375 | +7.6% |
| Climate stress | Space cooling and storage losses high; the rest central | +1,398.2 | 4,589 | 6,086 | 8,419 | +29.8% |
| All upside | Every row that raises generation high, every row that lowers it low (T&D losses at 0) | +2,191.0 | 5,381 | 6,879 | 9,212 | +46.7% |
| All downside | Every row that raises generation low, every row that lowers it high (T&D losses at their upper bound) | +143.3 | 3,334 | 4,831 | 7,164 | +3.1% |

TWh in FY2047-48.
A constant adjustment shifts every quantile by its total (up to histogram binning, about 0.1 TWh).
Before D15 (22 November 2026), which raised the storage high from 150.1 to 173.6 TWh and the imports high from 15.3 to 49.3 TWh ([decisions.md](decisions.md), ruling R35), Efficiency and distributed was 5,076 TWh, Climate stress 6,063, All upside 6,855 and All downside 4,865; Features central and Electrification high did not change. Before ruling R30 raised the data-centre high from 397.1 to 697.8 TWh (D14), Electrification high was 6,528 TWh and All upside 6,555.

Regional medians, TWh:

| Scenario | Northern | Western | Southern | Eastern | North-Eastern |
| --- | --- | --- | --- | --- | --- |
| Core (headline) | 1,342 | 1,276 | 1,188 | 730 | 151 |
| Features central | 1,663 | 1,582 | 1,473 | 905 | 187 |
| Electrification high | 1,955 | 1,859 | 1,731 | 1,064 | 219 |
| Efficiency and distributed | 1,444 | 1,373 | 1,278 | 785 | 162 |
| Climate stress | 1,742 | 1,657 | 1,543 | 948 | 196 |
| All upside | 1,969 | 1,873 | 1,744 | 1,072 | 221 |
| All downside | 1,383 | 1,315 | 1,224 | 752 | 155 |
| Joint (section 5) | 1,651 | 1,569 | 1,461 | 898 | 185 |

Every feature scenario raises the requirement, even All downside: transport (at least 512 TWh), data centres (at least 356) and hydrogen (at least 138) outweigh every row that lowers generation at its high value, T&D losses included.

**One at a time.** Each layer row at its central value and at its extreme, the end of its range farthest from central (ties go to high), with every other row off (`outputs/feature_scenarios_2047.csv`, section `one_at_a_time`):

| Row | Central: adjustment, P50 | Extreme: level, adjustment, P50 |
| --- | --- | --- |
| T&D losses | 0; 4,688 | high; -476.3; 4,212 |
| Auxiliary consumption | -91.6; 4,596 | high; -110.5; 4,577 |
| Storage round-trip losses | +150.1; 4,838 | low; +72.2; 4,760 |
| Cross-border imports | -13.1; 4,675 | high; -49.3; 4,639 |
| Rooftop solar | -24.7; 4,663 | high; -134.9; 4,553 |
| PM-KUSUM C pump solarisation | -37.7; 4,650 | low; -13.0; 4,675 |
| Efficiency beyond trend | 0; 4,688 | high; -125.3; 4,563 |
| Transport electrification | +511.9; 5,200 | high; +525.2; 5,213 |
| Green hydrogen | +230.4; 4,918 | high; +682.3; 5,370 |
| Data centres | +397.1; 5,085 | high; +697.8; 5,386 |
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
| Core plus features (joint) | 4,222 | 4,515 | 5,765 | 7,503 | 8,118 | 5,916 |
| Feature total alone | 731 | | 1,069 | | 1,429 | 1,073 |

- The joint median is 1,078 TWh (+23.0 percent) above the headline.
- It sits below Features central (+1,122 TWh) because the joint draws give T&D losses a mean of -238 TWh, where Features central counts them at 0. Most of that is offset because the triangular draws of the other rows average 189 TWh more than their central values: green hydrogen, data centres and space cooling have high ends far above central, while the raised imports high (49.3 TWh) pulls the other way.
- Before D15 the joint distribution was P5 4,225, P10 4,519, P50 5,769 (+23.1 percent), P90 7,506 and P95 8,121 TWh (mean 5,919); before ruling R30 it was P5 4,130, P50 5,668 (+20.9 percent), P90 7,404 and P95 8,017 TWh.
- Joint regional medians are in the table of section 4.

**P90 plan test** (`outputs/feature_scenarios_2047.csv`, section `p90_plan`). The Robust policy's least-cost plan is solved at three requirements:

| Requirement planned for | TWh | Robust plan unmet, TWh |
| --- | --- | --- |
| Core P90 (the headline stress test) | 6,408 | 639 |
| Core plus features, joint P50 | 5,765 | 5.5 |
| Core plus features, joint P90 | 7,503 | 1,734 |

The Robust plan leaves 5.5 TWh (0.1 percent) of the joint median unmet (8.9 TWh at 5,769 before D15; before ruling R30 it met the joint median, then 5,668 TWh, in full). At the joint P90 it leaves 1,734 TWh unmet (1,738 at 7,506 TWh before D15; 1,635 at 7,404 TWh before R30; the LP status is optimal). The levers that close the core P90 gap (more usable solar land, [results.md](results.md), section 3.3) were tested only at the core P90.

## 6. How the scenarios cover every combination

- **Every combination of feature levels lies between All downside and All upside.** The layer is a sum of independent signed rows, each between its low and high value (T&D losses between 0 and their upper bound). The smallest possible total takes each row at the end that lowers generation: All downside, +143.3 TWh. The largest takes each at the end that raises it: All upside, +2,191.0 TWh. Every joint draw lies inside this envelope; the test `joint_quantiles_bracket` checks it on 2^18 draws.
- **The named scenarios mark the corners that matter.** Electrification high and Climate stress push the load rows up; Efficiency and distributed pushes the offset rows to their limit; Features central is the official central case; the joint distribution weighs all combinations by their ranges.
- **Crossed with growth.** The three growth presets (Base, High growth, Low growth; [assumptions_and_uncertainty.md](assumptions_and_uncertainty.md), section 6) change the core forecast. A named feature scenario adds the same ramped total to any of them, so it shifts that preset's quantiles by its adjustment; the joint scenario adds the same feature draws. `outputs/feature_scenarios_2047.csv` reports the scenarios on the Base growth preset; the dashboard applies any feature scenario on top of any growth preset.
- **Crossed with policy.** The five policy presets (Base, Low carbon, Robust, Energy security and, since 22 November 2026, Nuclear mission) take whatever requirement the growth and feature settings give. `outputs/presets.csv` holds the 3 x 5 growth-by-policy matrix without features, the P90 plan test above solves the Robust plan with them, and the dashboard solves any policy for any feature scenario.
- So every combination of growth preset, feature levels and policy preset is either computed directly or bounded by the envelope on the chosen growth preset.

## 7. Overlap rules

- **The trend rule.** The core forecast carries every existing load growing with total generation: x2.7697 from FY2023-24 (1,698.03 TWh) to FY2047-48, and x1.8604 to FY2037-38. Only level 2047 - level 2023 x 2.7697 is beyond the trend. For supply-side rows (T&D losses, auxiliary use, imports) the rule is the change in rate or level from FY2023-24. The multiples are those of the 20 Nov headline (4,703 TWh); on the current headline the multiple is 2.7607, which changes no row by more than 2 TWh, so the values were not rescaled.
- **Rows that overlap the history** (`overlap_history` = yes). Only T&D losses is a layer row with full overlap. Its size is an upper bound on the effect beyond the trend, so it is 0 at central, uniform from 0 in the joint draws and at its high value only in the downside scenarios (ruling R22). The other "yes" rows are core, driver or context rows and add nothing.
- **Partly overlapping rows** (`partial`) are already cut to the part beyond the trend. Where no official figure separates that part (space cooling, efficiency beyond trend), low and central are 0 and high is the whole excess over the trend.
- **Rows inside other rows.** Rail and metro, and any EV sales target, are inside transport electrification. AI compute (GPUs), digital public infrastructure and the data-centre tax holiday are inside data centres. Hotter weather is inside space cooling. Hydrogen for steel and the hydrogen hubs are inside green hydrogen. Smart metering is inside T&D losses, and Mission LiFE inside the high value of efficiency beyond trend. Storage funding and co-location are inside storage round-trip losses. The distributed renewable component of the RCO overlaps rooftop solar and PM-KUSUM. None of these is added twice.
- **Mix rows.** The CEA 2047 capacity mix, the Nuclear Energy Mission, the coal pipeline, biomass co-firing, CCUS, the Brahmaputra plan, the transmission plan, resource adequacy, RE-INVEST, critical minerals and LNG change the supply mix (Part 2) or its accounting, not the quantity, so they add nothing to the requirement. The nuclear mission enters Part 2 as an optional policy ([results.md](results.md), section 3.9).
- **Part 2 storage.** The storage back-calculation of the plan ([results.md](results.md), section 3.6) reports its own round-trip losses (136 TWh in the Robust plan). If those are added to the requirement, the storage row must not be added again.
- **Gross-up interaction.** New loads are grossed up at FY2023-24 loss rates. With T&D losses active they are overstated by up to about 10 percent.
- **Captive and off-grid supply.** Hydrogen takes CEA's 50 percent grid share; the co-located half is captive and outside NPP. No official captive share exists for data centres, and the Ministry of Power expects the AI data-centre load to be integrated into the grid, so none is removed. The captive small reactors of the nuclear mission are excluded by rule (row 28).
- **Independence.** The rows are summed, and drawn, independently, exactly as the CSV gives them; no official source gives their correlations.

## 8. Limits

The full list is in [decisions.md](decisions.md), D14 and D15. The ones that touch this catalogue:

- Only T&D losses, data centres (central) and storage round-trip losses (central and high) have official values for or near 2047; the other rows hold or scale official values for 2030 to 2050.
- The data-centre high is our extrapolation of an official 2031-32 figure (the Ministry of Power's 26.3 GW), not an official 2047 figure: it continues that figure linearly for 16 more years and reads the 26.3 GW as average load (230.4 TWh a year); the reply does not say whether it is load or capacity (ruling R30).
- The imports high assumes the full 10,000 MW Nepal target, which has no schedule or contract; the storage high reads the road map's energy storage as batteries cycled once a day (ruling R35).
- No conference text gives an India figure in TWh beyond the trend, so every factor from the review is a context row (ruling R36); the largest unquantified effects are on the mix, not the requirement.
- Telecom networks, consumer electronics, semiconductor manufacturing and AI compute have no official electricity projection; they stay context rows.
- The unit factors are official but fixed (section 3 gives each).
- Whether feeder-level solar (PM-KUSUM C) and Component A plants are inside NPP utility generation is not stated in the sources. If they are, row 23's effect is zero.
- Features are split across regions by each region's share of the requirement; no official regional split exists (ruling R23).
- The notes of the core and driver rows in `data/external/demand_features.csv` quote the 20 Nov run (elasticity 0.828, 4,703 TWh); section 3 gives the current values.

## 9. Official conferences and texts reviewed

On 22 November 2026 the catalogue was checked against the outcomes of official energy conferences and the texts behind them, 2023 to 2026 ([decisions.md](decisions.md), D15). Only official outcome texts were used for substance: the Government of India (ministries, PIB, the Union Budget, CEA, NITI Aayog, replies in Parliament), the UNFCCC, the G20 and G7, the Clean Energy Ministerial and the EU. The IEA and IRENA are intergovernmental and are labelled secondary. The press and think tanks served only to locate documents or, where an official list could not be opened, as labelled context.

**Result.** No conference outcome gives an India figure in TWh beyond the historical trend, so no layer row was added and the headline does not move. Two layer highs were raised from Indian official sources (rows 8 and 10), three rows gained revised text (rows 11, 34 and 41), 15 rows gained conference citations with no value change, and 24 rows were added: 23 context rows and the captive small reactors, excluded by rule (row 28).

**India.**

| Conference or text | Date | What it gave the catalogue | Rows |
| --- | --- | --- | --- |
| National Conference of Power Ministers of States and UTs (PIB 2072871) | 12 Nov 2024 | Nuclear plants at retired coal sites; coal plants flexible to 40 percent; storage and transmission plans | 13, 14 |
| Regional Power Conferences: North-Eastern (Gangtok), Western (Mumbai), Southern (Bengaluru), Northern (Chandigarh), Eastern (Patna) (PIB 2124537, 2128438, 2130769, 2134584, 2139257) | Apr to Jun 2025 | Resource adequacy tie-ups; "at least one nuclear power project in each State"; prepaid smart meters | 13, 19, 35 |
| Bharat Electricity Summit 2026 and its ministerial meeting (PIB 2242673, 2243591) | 19-22 Mar 2026 | National Resource Adequacy Plan; transmission plan to 2035-36; export ambition | 11, 18, 19 |
| Brainstorming Session on the Indian Power Sector Scenario 2047, CEA and MoP (PIB 2064702) | 14-15 Oct 2024 | CEA's vision of 2047 capacity and storage; the national transmission plan | 9, 18 |
| CEA's 52nd Foundation Day (PIB 2177769) | 11 Oct 2025 | Release of the 100 GW nuclear road map (requirement, capacity mix, storage) and of the Brahmaputra master plan | 8, 12, 13, 17, 28 |
| Ministry of Power press conference (PIB 2135450) | 10 Jun 2025 | Battery viability gap funding; storage charge waiver; 1100 kV lines | 9, 18 |
| India Energy Week 2025 and 2026 (PIB 2218984) | 2025; 27 Jan 2026 | 15 percent of energy from LNG; energy independence (primary energy, not power) | 67 |
| International Solar Alliance, eighth Assembly, New Delhi (PIB 2183262) | 27-30 Oct 2025 | One Sun One World One Grid (no India quantity) | 11 |
| 4th RE-INVEST, Gandhinagar (PIB 2055789) | 16-18 Sep 2024 | Capacity pledges to 2030 | 20 |
| Union Budgets 2025-26 and 2026-27 | 1 Feb 2025; 1 Feb 2026 | The Nuclear Energy Mission (100 GW by 2047); CCUS outlay; data-centre tax holiday to 2047 | 13, 16, 42 |
| Replies in Parliament: DAE (Lok Sabha USQ 2783 and 5368), Ministry of Heavy Industries (USQ 4155), Ministry of Power (PIB 2240743) | 2025-2026 | NPCIL's 54 GW; captive small reactors; no official EV sales target; the coal programme to 2034-35 | 13, 14, 28, 38 |
| Other official texts: CEA NGAP (19 Mar 2026), PMIndia statement on Nepal (1 Jun 2023), NITI Aayog scenario launch (PIB 2226010), PIB backgrounders (green hydrogen hubs, SHANTI Act), MoP Annual Report 2025-26, RDSS and time-of-day rules (PIB 2215761, 1934673), National Critical Mineral Mission (PIB 2097308), India's NDC 2031-35 | 2023-2026 | The imports high; hubs; co-firing; smart meters; time-of-day tariffs; critical minerals; the 2047 electricity share | 10, 15, 21, 34, 35, 40, 56, 65 |

**International** (none gives an India quantity in TWh).

| Conference or text | Date | What it gave the catalogue | Rows |
| --- | --- | --- | --- |
| COP28, Dubai: decision 1/CMA.5 (first global stocktake) and the presidency pledges | Dec 2023 | Tripling renewables and doubling efficiency (global), coal phase-down, CCUS, zero-emission vehicles, fossil-fuel subsidies; India did not endorse the renewables and efficiency pledge | 16, 33, 37, 39, 66, 70, 71 |
| COP29, Baku: declarations and pledges letter | Nov 2024 | Storage and grids pledge (1,500 GW worldwide); hydrogen; green digital action | 8, 39, 41, 43 |
| COP30, Belém: decisions 1/CMA.7 to 3/CMA.7; the Belém 4X pledge on sustainable fuels (co-launched by India) | Nov 2025 | Grid interconnection; clean cooking; the UAE Consensus without the fossil-fuel wording; sustainable fuels | 10, 11, 37, 39, 49, 70 |
| COP31, Antalya | Nov 2026 | No adopted decision as of 21 Nov 2026; to be re-checked | none |
| G20: New Delhi Leaders' Declaration (2023), Energy Transitions Ministerial in Brazil (2024), South Africa ministerial and Johannesburg declaration (2025), Energy Abundance Ministerial in Houston (2026) | 2023-2026 | Lifestyles (LiFE), critical minerals, interconnection, subsidy reform, clean cooking with LPG, energy security | 21, 36, 49, 62, 66 |
| G7 Critical Minerals Action Plan, endorsed by India | Jun 2025 | Critical minerals | 21 |
| Clean Energy Ministerial and Mission Innovation: CEM15/MI-9 (2024), CEM16/MI-10 (2025); CEM17/MI-11 (2026, no outcomes document found) | 2024-2026 | Sustainable lifestyles; efficient air conditioners; India joins the CCUS Initiative; clean cooking; green steel procurement | 16, 36, 48, 49, 53 |
| IEA: 2024 ministerial communique, 2026 chair's summary, Global Conferences on Energy Efficiency (Versailles 2023, Nairobi 2024, Brussels 2025, Montreal 2026), Energy and AI (2025) (intergovernmental, secondary) | 2023-2026 | Efficiency targets (India not a signatory of the 2025 and 2026 statements); data centres worldwide; the 2026 energy crisis; nuclear | 33, 41, 43, 62 |
| IRENA: 15th and 16th Assemblies; tracking reports on the UAE Consensus (intergovernmental, secondary) | 2025-2026 | Global renewables and efficiency tracking only | none |
| EU CBAM: Regulation (EU) 2023/956 and proposal 2025/0419(COD); the India-EU FTA (27 Jan 2026); the Ministry of Steel's Green Steel Taxonomy | 2023-2026 | Export steel and aluminium; recognition of carbon prices | 53, 68, 69 |

**Not read.** CEA's transmission plan to 2035-36 and the CEA Brahmaputra master plan (404 or a stub page on 22 Nov 2026; only the PIB totals are used); parts of PIB (automated access refused; ministry copies were read instead); IRENA's site (indexed publication pages only); the UNEP list of Global Cooling Pledge endorsers (press, context only); the CEM17 outcomes; COP31's decisions, not yet posted.
