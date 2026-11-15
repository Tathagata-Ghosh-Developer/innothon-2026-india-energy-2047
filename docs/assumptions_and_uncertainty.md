# Assumptions, uncertainties and limitations

Every assumption below is either estimated from data, taken from a cited source, or exposed as a dashboard control.
The "Control" column names the dashboard slider where one exists.

## 1. Forecast assumptions

| # | Assumption | Value | Basis | Control |
| --- | --- | --- | --- | --- |
| F1 | Generation per person responds to income and urban share as in other countries' history (within-country changes) | FE-Quad and FE-Gompertz fitted on 153 countries, 1990-2025 | Ember and World Bank data; country-cluster bootstrap | Model weights |
| F2 | India starts from its own observed 2025 level | 1,421 kWh per person, 2,080 TWh | Ember | none |
| F3 | Population path | UN WPP 2024 medium, anchored to the observed 2025 population; low/high variants treated as a 90 percent band | UN WPP 2024 | Population band |
| F4 | Urban share path | UN WUP 2018 change from 2025, added to the observed 2025 share; plus or minus 2 percentage points (1 s.d.) by 2047 | UN WUP 2018 | Urban shift |
| F5 | GDP per capita growth | 4.90 percent a year to 2030 (India's 2005-2025 mean), then slowing by 1.5 points by 2047 | World Bank series; slowdown is our assumption, consistent with slower growth at higher income | Growth, slowdown |
| F6 | Growth uncertainty | Persistent shift with s.d. 0.75 points, plus annual shocks with s.d. 3.18 percent (2005-2025) and AR(1) persistence -0.03 | World Bank series | Growth uncertainty |
| F7 | Model weights | 0.26 / 0.60 / 0.14 (FE-Quad / FE-Gompertz / damped trend), proportional to 1 / backtest MAPE from 2010 | Backtest | Model weights |
| F8 | New loads not seen in history (green hydrogen, data centres, electric mobility beyond trend) | 0 by default | Not in the base case to avoid double counting with the trend term | New loads |

## 2. Regional assumptions

| # | Assumption | Basis |
| --- | --- | --- |
| R1 | A state's requirement per person follows the same income response as the national model | Model consistency; no state-level generation-need data exist under the generation-only rule |
| R2 | Base-year regional split from population and income only; urbanisation enters through each state's projected change in urban share | The urban coefficient is a within-country effect over time; applying it to cross-state levels exaggerated differences (South 33 percent at base). Recorded in [decisions.md](decisions.md), D3. |
| R3 | State income gaps close at 2.1 percent a year (s.e. 1.1), sampled in the Monte Carlo | Beta-convergence of relative GSDP per capita, organiser data 2006-2024 |
| R4 | State population shares follow the Technical Group projections to 2036, then each state's 2031-2036 trend | Technical Group on Population Projections (2020) |
| R5 | The regional requirements are planned at the national median and split by regional medians | Regional quantiles do not add up to the national quantile |

## 3. Strategy assumptions (defaults)

| # | Setting | Default | Basis | Control |
| --- | --- | --- | --- | --- |
| S1 | Minimum non-fossil share of 2047 generation | 70 percent | Between CEA NEP 2031-32 (49 percent of gross generation) and net-zero-2070 pathways | yes |
| S2 | Coal floor | 60 percent of each region's 2024 coal generation | Large share of the coal fleet is young and plants are being life-extended (NEP; parliamentary replies) | yes |
| S3 | Coal ceiling | 110 percent of 2024 coal nationally, 150 percent per region | Existing plus under-construction fleet (about 40 GW under construction) | yes |
| S4 | Firm generation | at least 45 percent nationally and 25 percent in each region | Firm = coal, gas, nuclear, hydro, bioenergy, firmed RE (round-the-clock contracts) | yes |
| S5 | Usable solar and onshore wind | 25 percent of generation potential | Land competition; NISE and NIWE potentials are technical potentials | yes |
| S6 | Usable hydro | 70 percent of identified potential | CEA reassessment; environmental and social constraints | yes |
| S7 | Build rates | 2047 generation of each source at most 2025 level + 22 years x 2 x its record annual increase (2010-2025) | Ember national data: solar +52.9, wind +22.5, hydro +23.0, nuclear +9.1 TWh in their best year | yes |
| S8 | Transfers | 2 x the 2032 planned corridor capability, 60 percent average utilisation, losses = mean regional loss (3.1-4.2 percent) | CEA / NEP Vol II | yes |
| S9 | 2047 costs | IEA WEO 2025 India projections interpolated to 2047; hydro and bioenergy relative to new-coal tariffs; firmed RE = solar + current round-the-clock premium | IEA, SECI, CERC | cost multipliers in the sensitivity sweep |
| S10 | Carbon price | 0 | Not current policy | yes |

## 4. Uncertainty treatment

| Source | How it is represented |
| --- | --- |
| Population | Draw-level normal shift scaled to the UN low/high band |
| Urbanisation | Draw-level normal shift growing to 2 points by 2047 |
| Income growth | Persistent shift plus AR(1) annual shocks |
| Model structure | Three models mixed by backtest weights |
| Parameter estimates | 2,000 country-cluster bootstrap replicates per model |
| State convergence | Normal draw of the convergence speed |
| New loads | Uniform draw between the dashboard limits |
| Policy and cost settings | One-at-a-time sweep of 13 settings (tornado), 26 least-cost solves |

## 5. Limitations

1. **Regional requirement is modelled, not observed.** Under the generation-only rule there is no regional need series to validate against. The base-year split is checked for plausibility against regional generation shares (which include transfers).
2. **Industrial structure.** Heavy industry (steel, aluminium, cement) is concentrated in the Eastern and Western regions. Population and income alone understate it. A sector-based extension is future work.
3. **Annual energy only.** The plan balances annual energy. It does not model hourly or seasonal balancing, ramping, or reserve margins. Firm-generation floors are a proxy for reliability.
4. **The trend model's tail.** Its own error variance produces a thin upper tail (a few scenarios above 8,000 TWh). The 90 percent interval is unaffected, and the dashboard histogram shows P0.5 to P99.5.
5. **Costs to 2047 are uncertain.** IEA projections are a single published path. The sensitivity sweep varies solar, coal and nuclear costs by plus or minus 30 percent.
6. **Captive generation.** The national series includes captive plants; regional base mixes do not (about 230 TWh in 2024). The regional requirement covers both, so the plan supplies captive demand from the grid mix.

## 6. Scenario and policy presets

The dashboard buttons and the command-line preset matrix both read these definitions from `src/pipeline/presets.hpp`.

| Preset | Type | Settings (changes from the defaults) |
| --- | --- | --- |
| Base | Scenario | None |
| High growth | Scenario | GDP per capita growth 6.5 percent to 2030, slowing by 1 point by 2047; urban share +3 points by 2047; new loads 100 to 300 TWh |
| Low growth | Scenario | GDP per capita growth 3.5 percent to 2030, slowing by 2 points; urban share -2 points by 2047 |
| Base | Policy | None |
| Low carbon | Policy | Non-fossil at least 80 percent; coal floor 35 percent of 2024; carbon price 50 USD/t; 30 percent usable solar and wind; corridors 3x the 2032 plan; nuclear up to 15 percent |
| Energy security | Policy | Regional firm share 40 percent; national firm share 55 percent; corridors at the 2032 plan; gas at most 2 percent; coal floor 70 percent |

The growth presets also move urbanisation.
Within countries, faster income growth comes with faster urbanisation.
The fitted models attribute much of the historical rise in generation per person to urbanisation.
