# Results

All numbers are energy generated (TWh).
They come from the default scenario: 4,194,304 Monte Carlo scenarios, seed 20470815.
Regenerate them with `energy2047 <repo_root>`; the tables are written to `outputs/`.

![National forecast](img/1_national_forecast.png)

## 1. National generation requirement

India generated 2,080 TWh in 2025 (Ember, including captive plants), or 1,421 kWh per person.

| Year | P5 | P25 | Median | P75 | P95 |
| --- | --- | --- | --- | --- | --- |
| 2030 | 2,388 | 2,536 | 2,616 | 2,699 | 2,838 |
| 2035 | 2,719 | 3,035 | 3,182 | 3,336 | 3,592 |
| 2040 | 3,010 | 3,527 | 3,751 | 3,986 | 4,390 |
| **2047** | **3,351** | **4,185** | **4,536** | **4,915** | **5,606** |

- The median requirement for 2047 is **4,536 TWh**, 2.18 times the 2025 level (3.6 percent a year).
- Generation per person rises to about 2,720 kWh.
- The 90 percent range is 3,351 to 5,606 TWh.

Central paths of the individual models in 2047, using medium drivers and mean growth:

| Model | 2047 (TWh) | Ensemble weight |
| --- | --- | --- |
| FE-Quad | 4,777 | 0.26 |
| FE-Gompertz | 4,533 | 0.60 |
| Damped trend | 3,782 | 0.14 |

**Cross-check.** The CEA National Electricity Plan projects gross generation of 2,666 TWh for 2031-32.
Our median for 2031 to 2032 is in the same range.
That plan was built independently, from bottom-up demand and supply planning.

## 2. Regional requirement

![Regional requirement](img/2_regional_requirement.png)

| Region | 2030 median | 2040 median | 2047 P5 | 2047 median | 2047 P95 | Share 2047 | Generation 2024 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Northern | 735 | 1,085 | 968 | 1,326 | 1,614 | 29% | 454 |
| Western | 671 | 932 | 831 | 1,121 | 1,396 | 25% | 658 |
| Southern | 645 | 849 | 731 | 984 | 1,286 | 22% | 395 |
| Eastern | 473 | 744 | 669 | 930 | 1,135 | 20% | 276 |
| North-Eastern | 95 | 141 | 126 | 173 | 214 | 4% | 21 |

The planned 2047 requirement scales these medians to the national median (4,536 TWh).
Base-year generation is from utilities only, so it excludes captive plants.

Reading the table:

- The **Northern** region has the largest requirement because of its population (Uttar Pradesh, Rajasthan).
- The **Eastern** and **North-Eastern** requirements grow fastest. Two drivers explain this:
  - Bihar and Jharkhand have the fastest-growing populations.
  - Poorer states close their income gap at the rate measured from the organiser GSDP data (2.1 percent of the gap a year).
- The **Western** region generates far more today than its requirement share. It exports coal power, and that pattern shows up as transfers in the strategy.

## 3. 2047 generation strategy

![2047 generation mix](img/3_generation_mix_2047.png)

### 3.1 Least-cost plan (default policy settings)

| Region | Coal | Nuclear | Hydro | Solar | Onshore wind | Offshore wind | Total | Net export |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Northern | 291 | 11 | 337 | 504 | 153 | 0 | 1,297 | -30 |
| Western | 317 | 20 | 66 | 608 | 246 | 58 | 1,313 | +192 |
| Southern | 149 | 24 | 111 | 458 | 233 | 88 | 1,063 | +78 |
| Eastern | 390 | 45 | 72 | 117 | 10 | 0 | 635 | -296 |
| North-Eastern | 3 | 0 | 218 | 20 | 0 | 0 | 241 | +69 |

National mix:

| Source | Share |
| --- | --- |
| Solar | 37.5% |
| Coal | 25.3% |
| Hydro | 17.7% |
| Onshore wind | 14.1% |
| Offshore wind | 3.2% |
| Nuclear | 2.2% |
| Gas | 0% |

- Non-fossil share: **74.7 percent**.
- CO2 from generation: **1,114 Mt** a year. The 2024-25 grid emitted about 0.71 t/MWh, so 2,080 TWh today means roughly 1,480 Mt.
- Generation cost: about 197 billion USD a year.

### 3.2 How each region's strengths show up

| Region | Mix and reason |
| --- | --- |
| Northern | Solar (Rajasthan, the largest solar resource) and Himalayan hydro (J&K, Himachal, Uttarakhand). Hydro supplies 26 percent of the region. |
| Western | Solar and wind (Gujarat, Maharashtra), Gulf of Khambhat offshore wind, and the existing Chhattisgarh and Madhya Pradesh coal fleet. It becomes the largest exporter (+192 TWh). |
| Southern | The best wind (Tamil Nadu, Karnataka), offshore wind off Tamil Nadu, and nuclear at Kudankulam and Kaiga. It exports 78 TWh. |
| Eastern | Coal stays at its fleet limit (1.5 times 2024), and the planned Haripur nuclear site is used. Solar and wind resources are small, so the least-cost answer imports about 296 TWh over the ER-NR, ER-WR and ER-SR corridors. |
| North-Eastern | Hydro (Arunachal Pradesh, about 50 GW identified). It turns the region from a small system into a net exporter of firm clean power (+69 TWh). |

### 3.3 Resource-score allocation (transparent alternative)

This method keeps the same national totals by source.
It shares them across regions in proportion to usable resource, existing fleets and nuclear sites.
It gives the same regional pattern with larger transfers:

| Region | Net export |
| --- | --- |
| Western | +348 TWh |
| Southern | +155 TWh |
| North-Eastern | +107 TWh |
| Northern | -86 TWh |
| Eastern | -512 TWh |

Where the two methods disagree, the corridor limits bind in the least-cost plan.
That is a direct argument for expanding transmission into the Eastern region.

### 3.4 Scenario and policy presets

Each scenario preset is crossed with each policy preset.
The definitions are in [assumptions_and_uncertainty.md](assumptions_and_uncertainty.md), section 6.
The tool writes the full table to `outputs/presets.csv`.

| Scenario | Policy | 2047 median (TWh) | Non-fossil | Coal | Solar | Hydro | Nuclear | CO2 (Mt) | Cost (bn USD) | Unmet (TWh) | East net export (TWh) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Base | Base | 4,536 | 74.7% | 25.3% | 37.5% | 17.7% | 2.2% | 1,114 | 197 | 0 | -296 |
| Base | Low carbon | 4,536 | 89.8% | 10.2% | 44.9% | 18.3% | 6.4% | 450 | 177 | 0 | -544 |
| Base | Energy security | 4,536 | 73.1% | 26.9% | 37.5% | 16.9% | 2.2% | 1,188 | 199 | 0 | -294 |
| High growth | Base | 5,176 | 71.9% | 28.1% | 32.9% | 16.0% | 7.9% | 1,415 | 242 | 0 | -456 |
| High growth | Low carbon | 5,176 | 82.5% | 14.7% | 39.5% | 16.6% | 8.8% | 806 | 223 | 0 | -259 |
| High growth | Energy security | 5,176 | 70.0% | 28.0% | 32.9% | 15.3% | 6.4% | 1,455 | 245 | **4.7** | -338 |
| Low growth | Base | 4,108 | 80.7% | 19.3% | 41.4% | 19.1% | 1.3% | 772 | 168 | 0 | -453 |
| Low growth | Low carbon | 4,108 | 88.7% | 11.3% | 49.6% | 18.4% | 2.0% | 450 | 150 | 0 | -466 |
| Low growth | Energy security | 4,108 | 75.3% | 24.7% | 41.4% | 17.0% | 1.3% | 987 | 172 | 0 | -338 |

The 90 percent ranges for 2047 are:

| Scenario | P5 to P95 (TWh) |
| --- | --- |
| Base | 3,351 to 5,606 |
| High growth | 3,560 to 6,572 |
| Low growth | 3,213 to 5,056 |

**What the table shows:**

1. **The low-carbon policy costs less, not more.**
   It cuts 2047 CO2 to about 450 Mt, which is coal at its 35 percent fleet floor.
   Generation cost falls by about 20 billion USD a year, because more land for solar and wind and larger corridors let cheaper renewables replace coal.
   The 50 USD/t carbon price is a transfer, so it is not included in generation cost.
2. **High growth breaks the energy-security policy.**
   With regional self-sufficiency rules and corridors frozen at the 2032 plan, 4.7 TWh cannot be served.
   Fast growth needs corridor expansion or more firm capacity in deficit regions.
3. **Nuclear becomes the marginal firm source under high growth.**
   It rises to 8 to 9 percent once the cheaper firm options are used up.
4. **Faster growth raises the requirement only moderately.**
   The fitted models show generation per person saturating with income at the levels India reaches in the 2040s.
   Within countries, urbanisation and income rise together, and the models attribute much of the historical rise to urbanisation.
   The growth presets therefore also move the urban share: +3 points for high growth, -2 points for low growth.

## 4. Sensitivity

![Uncertainty and sensitivity](img/5_uncertainty_sensitivity.png)

### 4.1 Requirement uncertainty

Each driver's share of the 2047 spread is a rank R-squared, measured one input at a time.

| Driver | Share of the spread |
| --- | --- |
| Urban share path | about 13% |
| Population path | about 9% |
| Model choice | about 9% |
| Persistent GDP growth | about 2% |

The rest comes from bootstrap parameter uncertainty, annual growth shocks and the trend model's own noise.

### 4.2 Policy settings

Each setting is moved to a low and a high value with everything else fixed.
That is 26 least-cost solves, which run in parallel in about 7 ms.

| Setting | Effect on 2047 CO2 |
| --- | --- |
| Requirement (P10 vs P90) | -342 Mt to +340 Mt. This is the largest effect. |
| Usable solar land (15% vs 35%) | +208 Mt to -342 Mt |
| Usable wind land (15% vs 35%) | +208 Mt to -196 Mt |
| Coal cost (0.7x vs 1.3x) | +208 Mt to -328 Mt |
| Carbon price of 60 USD/t | -336 Mt; the coal share falls from 25.3% to 17.5% |
| Nuclear cost (0.7x vs 1.3x) | At 0.7x, coal falls to 17.8 percent and CO2 by 328 Mt; at 1.3x, +45 Mt |

Two settings cannot be met with the other defaults:

- A non-fossil minimum of 85 percent conflicts with the default coal floor (60 percent of the 2024 fleet).
- Raising that minimum therefore requires early coal retirement.

## 5. Performance

| Run | 1,048,576 scenarios | Scenarios per second |
| --- | --- | --- |
| CPU, 1 thread | 22.7 s | 46,000 |
| CPU, 16 threads (OpenMP) | 2.07 s | 507,000 |
| GPU, RTX 3070 Ti Laptop (CUDA) | 7.5 ms | 140 million |

The full dashboard update takes about 1.8 s:

- 4.2 million scenarios on the GPU, about 30 ms.
- Least-cost and score allocations.
- The 26-solve policy sweep.
- Bootstrap of 2,000 replicates for each of two models: 2.2 s on 16 threads, once at start-up.
