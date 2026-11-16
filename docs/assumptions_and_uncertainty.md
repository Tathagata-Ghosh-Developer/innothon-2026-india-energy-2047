# Assumptions, uncertainties and limitations

Every assumption below is either estimated from data, taken from a cited source, or exposed as a dashboard control.
The "Control" column names the dashboard slider where one exists.

## 1. Forecast assumptions (primary core, India only)

| # | Assumption | Value | Basis | Control |
| --- | --- | --- | --- | --- |
| F1 | Generation per person grows with real income per person at India's own historical elasticity | 0.828 (FY2011-12 to FY2023-24) and 0.866 (FY2006-07 to FY2023-24); bootstrap mixture median 0.850, 90 percent interval 0.731 to 0.939 | NPP generation and organiser population and GSDP only; GSDP values copied unchanged from the previous year are not counted as income growth | Elasticity decline |
| F2 | Scope is NPP utility generation; captive plants are a separate layer | Captive 13.2 percent of utility generation, held | NPP; CEA General Review 2025 Table 1.6 | none |
| F3 | Base year FY2023-24; model error is a persistent residual that starts at the FY2023-24 residual and reverts to the fitted relation (AR(1)) | 1,698 TWh observed; residual +0.070 log points (1.9 residual s.d.); persistence 0.51; innovation s.d. 0.032 | Durbin-Watson 0.99, persistence = 1 - DW / 2; residual s.d. 0.037 | none |
| F4 | Population path | UN WPP 2024 medium, anchored to the organiser FY2023-24 population; low/high variants treated as a 90 percent band | UN WPP 2024 | Population band |
| F5 | Urban share path | UN WUP 2018 change from FY2023-24; plus or minus 2 percentage points (1 s.d.) by 2047 | UN WUP 2018 | Urban shift |
| F6 | Urbanisation beyond the income-implied path raises generation per person | gamma normal(1.0, 0.5), truncated at 0; kappa = 0.092 | Liddle 2013; Shahbaz et al. 2016; Holtedahl & Joutz 2004 | Model weights (0 / 1 / 0 = no urban term) |
| F7 | Income growth per person | 4.95 percent a year to 2030 (organiser data, FY2012-13 to FY2023-24, pandemic years left out), then slowing by 1.5 points by 2047 | Organiser GSDP; the slowdown is our assumption, consistent with slower growth at higher income | Growth, slowdown |
| F8 | Growth uncertainty | Persistent shift with s.d. 0.9 points (90 percent band of the average about +-1.5 points), plus annual shocks with s.d. 1.50 percent and AR(1) persistence 0.63 | Organiser GSDP (persistence from consecutive retained years only); the persistent shift spans published low and high growth paths | Growth uncertainty |
| F9 | Model weights | 1 / 0 / 0 (elasticity with urban prior / no urban term / damped trend) | Backtest errors from seven short origins are too noisy for weights; the trend has no drivers | Model weights |
| F10 | New loads not seen in history (green hydrogen, data centres, electric mobility beyond trend) | 0 by default | Not in the base case to avoid double counting with the income response | New loads |

The cross-country secondary check keeps its own assumptions: FE-Quad and FE-Gompertz fitted on 153 countries (1990-2025), base 2,080 TWh in 2025 (Ember, including captive), World Bank GDP per capita growth 4.90 percent to 2030 with persistent s.d. 0.9 points, weights 0.26 / 0.60 / 0.14 from a 2010-origin backtest.

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
| Model structure | Primary: one driver model; the no-urban-term and damped-trend slots are ablations. Secondary: three models mixed by backtest weights |
| Model error | Primary: AR(1) residual of ln(kWh per person), starting at the FY2023-24 residual (+0.070), persistence 0.51, innovation s.d. 0.032 |
| Parameter estimates | Primary: 2,000 moving-block (3-year) bootstrap replicates of the residuals of the years without the pandemic dummy, rescaled for degrees of freedom, half from each estimation window, each with a gamma draw from its prior (truncated normal). Secondary: 2,000 country-cluster bootstrap replicates per model |
| State convergence | Normal draw of the convergence speed |
| New loads | Uniform draw between the dashboard limits |
| Policy and cost settings | One-at-a-time sweep of 14 settings (tornado), 28 least-cost solves |
| CO2 | Cost-CO2 trade-off of the recommended plan: a national CO2 cap swept from the least-cost plan down to the lowest cap that still meets the requirement (13 points, `outputs/pareto_2047.csv`) |

## 5. Limitations

1. **Regional requirement is modelled, not observed.** Under the generation-only rule there is no regional need series to validate against. The base-year split is checked for plausibility against regional generation shares (which include transfers).
2. **Industrial structure.** Heavy industry (steel, aluminium, cement) is concentrated in the Eastern and Western regions. Population and income alone understate it. A sector-based extension is future work.
3. **Annual energy in the plan.** The plan balances annual energy. Storage and the extra generation that hourly balancing needs are worked out afterwards from one representative year ([results.md](results.md), section 3.6) and do not feed back into the plan. Ramping and reserve margins are not modelled; firm-generation floors are a proxy for reliability.
4. **Eighteen annual points.** India's annual NPP series starts in FY2006-07, so only the income elasticity is estimated from it. A quadratic, a saturation curve or a free urban coefficient cannot be identified. The uncertainty of the forecast is driven mostly by the income path, not by the model form.
5. **Supply-constrained history.** Energy shortages closed between the late 2000s and FY2017-18, so part of past generation growth was catch-up, which can raise the estimated elasticity. The FY2011-12 window and the first-difference estimates (0.78 and 0.79) are lower than the full-window estimate (0.87); the elasticity-decline setting shows the size of the effect.
6. **Costs to 2047 are uncertain.** IEA projections are a single published path. The sensitivity sweep varies solar, coal and nuclear costs by plus or minus 30 percent.
7. **Captive generation.** The forecast covers NPP utility generation. Captive plants (224 TWh in FY2023-24) are reported as a layer, not planned in the regional mix.
8. **Model-error persistence from a short series.** The persistence of the residual (0.51) comes from the Durbin-Watson statistic of 13 annual residuals, not from a direct lag regression, which the slow drift of the early, supply-constrained years would dominate.
9. **Placeholder income data.** Five states and union territories repeat a GSDP figure across years in the organiser data (four of them from 2006 to 2010, Nagaland in 2007). Those year pairs are left out of national income growth, so national income growth for 2007 to 2010 rests on the remaining states.

## 6. Scenario and policy presets

The dashboard buttons and the command-line preset matrix both read these definitions from `src/pipeline/presets.hpp`.

| Preset | Type | Settings (changes from the defaults) |
| --- | --- | --- |
| Base | Scenario | None |
| High growth | Scenario | Income growth per person 1.5 points above history to 2030, slowing by 1 point by 2047; urban share +3 points by 2047; new loads 100 to 300 TWh |
| Low growth | Scenario | Income growth per person 1.5 points below history to 2030, slowing by 2 points; urban share -2 points by 2047 |
| Base | Policy | None |
| Low carbon | Policy | Non-fossil at least 80 percent; coal floor 35 percent of 2024; carbon price 50 USD/t; 30 percent usable solar and wind; corridors 3x the 2032 plan; nuclear up to 15 percent |
| Robust | Policy | Non-fossil at least 75 percent; 35 percent usable solar (including rooftop and agri-PV) and 30 percent wind; corridors 3x the 2032 plan; nuclear up to 15 percent; build rates 2.5x the record year; coal floor 50 percent. Meets the median with no unmet energy at lower cost and CO2 than the base policy; the P90 also needs 45 percent usable solar land, which is enough at the 2.5x build rate (`outputs/p90_levers.csv`) |
| Energy security | Policy | Regional firm share 40 percent; national firm share 55 percent; corridors at the 2032 plan; gas at most 2 percent; coal floor 70 percent |

The growth presets are shifts relative to each core's own historical growth, so they apply to both cores.
They also move urbanisation, because faster income growth comes with faster urbanisation.
