# India Energy 2047: forecast and regional plan

India's electricity generation requirement for {{years.target_label}}, where it is needed, and a regional generation mix that plays to each region's strengths. By Team The Losers, Indian Institute of Science, Bengaluru. Submission to ExxonMobil INNOTHON 2026; not an official ExxonMobil page.

This is the Markdown version of <https://india-energy-2047.vercel.app/>. Every model result in it is written by the model run to [summary.json](https://india-energy-2047.vercel.app/summary.json) and placed here when the site is built. Years are fiscal years: {{years.target_label}} runs from April {{years.target}} to March of the next year.

## Headline

- India needs about **{{national.p50|0}} TWh** in {{years.target_label}}: the median of {{scenarios|0}} scenarios.
- 80% of scenarios lie between {{national.p10|0}} and {{national.p90|0}} TWh (P10 to P90).
- That is {{national.multiple_of_base|1}} times the {{national.base_twh|0}} TWh generated in {{years.base_label}}: {{national.increase_twh|0}} TWh more, about {{national.growth_pct_per_year|1}}% more each year.
- Scope: the country's total utility generation, the scope of the National Power Portal (NPP) series. Captive plants are not included, as the organisers ruled.
- Analysis basis: TWh generated, from the National Power Portal. No installed capacity or consumption enters the forecast.
- An independent cross-country model of 153 countries gives {{national.cross_country_p50_twh|0}} TWh for {{national.cross_country_year}} (calendar year, total generation). It is used to justify the forecast, not to estimate it.

## Forecast

National generation requirement, TWh:

| Year | P10 | Median | P90 |
| --- | ---: | ---: | ---: |
| {{years.base_label}} (observed) | | {{national.base_twh|0}} | |
<!--each national.milestones-->| {{@.label}} | {{@.p10|0}} | {{@.p50|0}} | {{@.p90|0}} |
<!--/each-->
Generation per person follows income per person, with India's own elasticity of {{elasticity.beta|2}}, and urbanisation; {{scenarios|0}} scenarios of income, population, urbanisation and that link give the {{years.target_label}} range, split into the {{len regions.list}} grid regions from each state's population and income.

### Tested on years the model never saw

The model was fitted on data up to {{years.base_label}}; its forecasts for the next years are compared with what happened. {{holdout_range_note}}

<!--each holdout-->- {{@.label}}: error {{@.error_pct|+1}}%. Model median {{@.model_p50|0}} TWh against {{@.estimate_twh|0}} TWh estimated; the estimate lies {{@.in_range}} the model's 90% range ({{@.model_p05|0}} to {{@.model_p95|0}} TWh).
<!--/each-->
NPP publishes conventional generation for these years but no renewable generation, so renewable output is extended at its recent growth rate to build the estimate.

### What drives the uncertainty

{{years.target_label}} median when one assumption changes (base case {{sensitivity.base_p50_twh|0}} TWh). Each case reruns all {{scenarios|0}} scenarios with one assumption changed.

| Assumption changed | Median, TWh | Change |
| --- | ---: | ---: |
<!--each sensitivity.drivers-->| {{@.label}} | {{@.p50_twh|0}} | {{@.change_pct|+1}}% |
<!--/each-->
Without our urbanisation effect the forecast gives {{sensitivity.no_urban_term.p50_twh|0}} TWh ({{sensitivity.no_urban_term.change_pct|+0}}%).

## Regions

Demand follows people and incomes; today's power plants are elsewhere. The Western region makes {{regions.list.1.share_today_pct|0}}% of today's conventional generation but needs {{regions.list.1.share_2047_pct|0}}% of the 2047 requirement, so power has to move between regions.

| Region | Median, TWh | 80% range, TWh | Share of need | Share of {{regions.today_label}} conventional generation |
| --- | ---: | ---: | ---: | ---: |
<!--each regions.list-->| {{@.name}} | {{@.p50_twh|0}} | {{@.p10_twh|0}} to {{@.p90_twh|0}} | {{@.share_2047_pct|1}}% | {{@.share_today_pct|1}}% |
<!--/each-->
Today: NPP thermal, nuclear and hydro generation in {{regions.today_label}} ({{regions.today_total_twh|0}} TWh); NPP publishes no regional renewable generation.

Energy one region sends another in the recommended plan, TWh a year:

<!--each plan.flows-->- {{@.from}} to {{@.to}}: {{@.twh|0}} TWh
<!--/each-->
## The recommended {{years.target}} mix

The least-cost plan under our Robust policy: {{plan.settings.solar_land_pct|0}}% of the solar potential usable (including rooftop and agri-PV), corridors between regions {{plan.settings.transfer_scale|0}} times the 2032 plan, building at up to {{plan.settings.build_accel|2}} times the published {{plan.settings.build_case}}-case build limits, and renewables backed by storage. It also keeps at least {{plan.settings.nonfossil_min_pct|0}}% non-fossil generation, {{plan.settings.wind_land_pct|0}}% of the wind potential usable and nuclear up to {{plan.settings.nuclear_max_pct|0}}%. Every cost is in real 2024 USD.

| | Recommended plan | Base policy |
| --- | ---: | ---: |
| Non-fossil share | {{plan.nonfossil_pct|1}}% | {{plan.base_policy.nonfossil_pct|1}}% |
| CO₂ emissions, Mt a year | {{plan.co2_mt|0}} | {{plan.base_policy.co2_mt|0}} |
| System cost, bn USD a year | {{plan.cost_busd|0}} | {{plan.base_policy.cost_busd|0}} |
| Unmet at the median, TWh | {{plan.unmet_twh|0}} | {{plan.base_policy.unmet_twh|0}} |

Share of {{years.target_label}} generation by source ({{plan.generation_twh|0}} TWh):

| Source | Includes | Share | TWh |
| --- | --- | ---: | ---: |
<!--each plan.mix-->| {{@.label}} | {{@.note}} | {{@.share_pct|1}}% | {{@.twh|0}} |
<!--/each-->
Firmed renewables: solar and wind backed by storage and sold round the clock.
<!--if plan.vectors-->
Each source goes where its region is strongest:

- Coal: {{plan.vectors.coal.twh|0}} TWh, {{plan.vectors.coal.share_pct|1}}% of the plan.<!--if plan.vectors.coal.lead--> Largest share in the {{plan.vectors.coal.lead}} region ({{plan.vectors.coal.lead_twh|0}} TWh, {{plan.vectors.coal.lead_share_pct|0}}%).<!--else--> Not chosen by the least-cost plan.<!--/if-->
- Solar: {{plan.vectors.solar.twh|0}} TWh, {{plan.vectors.solar.share_pct|1}}% of the plan.<!--if plan.vectors.solar.lead--> Largest share in the {{plan.vectors.solar.lead}} region ({{plan.vectors.solar.lead_twh|0}} TWh, {{plan.vectors.solar.lead_share_pct|0}}%).<!--else--> Not chosen by the least-cost plan.<!--/if-->
- Wind: {{plan.vectors.wind.twh|0}} TWh, {{plan.vectors.wind.share_pct|1}}% of the plan.<!--if plan.vectors.wind.lead--> Largest share in the {{plan.vectors.wind.lead}} region ({{plan.vectors.wind.lead_twh|0}} TWh, {{plan.vectors.wind.lead_share_pct|0}}%).<!--else--> Not chosen by the least-cost plan.<!--/if-->
- Natural gas: {{plan.vectors.gas.twh|0}} TWh, {{plan.vectors.gas.share_pct|1}}% of the plan.<!--if plan.vectors.gas.lead--> Largest share in the {{plan.vectors.gas.lead}} region ({{plan.vectors.gas.lead_twh|0}} TWh, {{plan.vectors.gas.lead_share_pct|0}}%).<!--else--> Not chosen by the least-cost plan.<!--/if-->
- Hydro: {{plan.vectors.hydro.twh|0}} TWh, {{plan.vectors.hydro.share_pct|1}}% of the plan.<!--if plan.vectors.hydro.lead--> Largest share in the {{plan.vectors.hydro.lead}} region ({{plan.vectors.hydro.lead_twh|0}} TWh, {{plan.vectors.hydro.lead_share_pct|0}}%).<!--else--> Not chosen by the least-cost plan.<!--/if-->
- Nuclear: {{plan.vectors.nuclear.twh|0}} TWh, {{plan.vectors.nuclear.share_pct|1}}% of the plan.<!--if plan.vectors.nuclear.lead--> Largest share in the {{plan.vectors.nuclear.lead}} region ({{plan.vectors.nuclear.lead_twh|0}} TWh, {{plan.vectors.nuclear.lead_share_pct|0}}%).<!--else--> Not chosen by the least-cost plan.<!--/if-->
<!--/if-->
If demand comes in high: at the P90 requirement ({{plan.p90.requirement_twh|0}} TWh) the same policy leaves {{plan.p90.robust_unmet_twh|0}} TWh unmet. Making {{plan.p90.levers_solar_land_pct|0}}% of the solar potential usable, instead of {{plan.settings.solar_land_pct|0}}%, leaves {{plan.p90.levers_unmet_twh|0}} TWh unmet. Land, not money, is the binding limit.

Cutting CO₂ further: a national CO₂ cap, tightened step by step, lowers the plan's emissions from {{plan.tradeoff.co2_first_mt|0}} to {{plan.tradeoff.co2_last_mt|0}} Mt a year ({{plan.tradeoff.cut_pct|0}}% less) for {{plan.tradeoff.extra_busd|+1}} bn USD a year ({{plan.tradeoff.extra_pct|+1}}%).

Storage and land: the plan is run hour by hour through a year of NPP hourly generation; it needs {{storage.energy_gwh|0}} GWh of storage, about {{storage.duration_h|1}} hours at full power. Its solar and wind need {{land.solar_wind_km2|0}} km², {{land.share_india_pct|2}}% of India's area.

## What else drives 2047: the demand-feature layer

Income, population and urbanisation drive the forecast, but much else can change what India must generate. We listed every driver we found, {{features.counts.total|0}} in all, each with its source, and sized in TWh those that official figures allow.

- The headline stays the two-dataset estimate: the {{national.p50|0}} TWh for {{years.target_label}} is estimated on NPP and the Renewable India dataset alone. This layer does not change it.
- Each quantified driver is a signed TWh adjustment from official sources: ministries, statutory bodies such as CEA and BEE, and NITI Aayog (secondary, labelled). Only its part beyond the historical trend is counted.
- By how the model treats them: {{features.counts.core|0}} estimated, {{features.counts.driver|0}} model drivers, {{features.counts.layer|0}} in the quantified layer, {{features.counts.excluded_rule|0}} excluded by rule and {{features.counts.context|0}} context.

{{years.target_label}} requirement by scenario, TWh. Core is the headline; every other row adds the quantified features.

| Scenario | P5 | Median | P95 | Median vs Core |
| --- | ---: | ---: | ---: | ---: |
<!--each features.scenarios.list-->| {{@.name}} | {{@.p05_twh|0}} | {{@.p50_twh|0}} | {{@.p95_twh|0}} | {{@.change_pct|+1}}% |
<!--/each-->| Joint: all features at random | {{features.joint.p05_twh|0}} | {{features.joint.p50_twh|0}} | {{features.joint.p95_twh|0}} | {{features.joint.change_p50_pct|+1}}% |

## Data and sources

- Primary: the two challenge datasets, the National Power Portal (NPP) and the Renewable India dataset (state population and GSDP at constant prices). The 2047 prediction is estimated on them.
- Secondary, labelled: the UN population and urban-share paths (World Population Prospects 2024, World Urbanization Prospects 2018), used as driver assumptions as the organisers ruled and checked against the dataset's state growth; the size of the urbanisation effect, our judgment from published studies; and the MoSPI GDP deflator, which puts Jammu and Kashmir's current-price GSDP on constant prices.
- All other published sources set only limits and prices in the mix (costs, resource potential, build rates and emissions) or size the demand features, which stay outside the headline. Asked about published figures as limits and prices in the mix, the organisers replied "Use insights from this data source".

## Links

- Web page: <https://india-energy-2047.vercel.app/>
- Interactive dashboard, the whole model running in your browser: <https://india-energy-2047.vercel.app/app/>
- Every model result on the page, as JSON: <https://india-energy-2047.vercel.app/summary.json>
- Source code and documentation: <https://github.com/Tathagata-Ghosh-Developer/innothon-2026-india-energy-2047>
- Methodology: <https://github.com/Tathagata-Ghosh-Developer/innothon-2026-india-energy-2047/blob/main/docs/methodology.md>
- Results and decisions: <https://github.com/Tathagata-Ghosh-Developer/innothon-2026-india-energy-2047/tree/main/docs>
- Final evaluation deck (PDF): <https://github.com/Tathagata-Ghosh-Developer/innothon-2026-india-energy-2047/blob/main/docs/deck/India_Energy_2047_The_Losers.pdf>
- Site guide: <https://india-energy-2047.vercel.app/llms.txt>
- Sitemap: <https://india-energy-2047.vercel.app/sitemap.xml>
- About: <https://india-energy-2047.vercel.app/about>; contact: <https://india-energy-2047.vercel.app/contact>; privacy: <https://india-energy-2047.vercel.app/privacy>
