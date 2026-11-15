# Team work plan

Status on 2026-11-15:

- The end-to-end forecast, the regional strategy and the dashboard work.
- Documentation is in place.

The plan below runs to the final evaluation.

## Milestones

| Date | Milestone | What must be ready |
| --- | --- | --- |
| 16-17 Nov, 18:00-20:00 | Office hours | Ask the two rule questions below; get feedback on the method |
| 18 Nov | Midpoint review (regional model review, sensitivity checks) | Short deck: problem framing, data audit, backtest, national and regional forecast, first 2047 mix, sensitivity |
| 21-22 Nov | Final evaluation | Defensibility deck, live dashboard demo, recorded backup video |
| 30 Nov | Prize ceremony | |

## Questions for the organisers (office hours)

1. May we split the national generation forecast across regions using population, GSDP and urbanisation, without regional consumption data?
2. May we convert published resource potentials (GW) to generation potential (TWh per year) with capacity factors, used only as upper bounds?

## Workstreams

| Workstream | Owner | Tasks before 18 Nov | Tasks before 21 Nov |
| --- | --- | --- | --- |
| Forecast and validation | (assign) | Check backtest numbers; prepare "why saturation" and "why India follows other countries" answers | Add a sector view for industry-heavy regions (East, West) if time permits |
| Regional strategy | (assign) | Review default policy settings and their sources; check regional mixes against known projects | Scenario presets (policy, high growth, low carbon); corridor expansion narrative |
| Data audit and compliance | (assign) | Walk through [data_audit.md](data_audit.md); confirm no consumption or capacity data in forecasting | Final provenance appendix for the deck |
| Dashboard and demo | (assign) | Run the dashboard on two machines (one without GPU, `-DENABLE_CUDA=OFF`) | Rehearse the live demo over Zoom screen share; record a backup video |
| Deck and story | (assign) | Midpoint deck from [results.md](results.md) and the dashboard images in [img/](img/) | Defensibility deck with likely questions and answers |

## Likely jury questions (prepare answers)

1. **Why should India follow other countries' path?**
   It need not follow their path exactly.
   The model uses within-country changes only, so each country keeps its own level.
   India's starting point is its own 2025 data.
   The backtest shows the approach would have predicted 2025 within 1 percent from 2010.
2. **Why is the 2047 range so wide?**
   The range includes uncertainty in population, urbanisation, income growth, model choice and parameters.
   The dashboard attributes the spread to each source.
3. **Is any capacity or consumption data used?**
   No capacity or consumption data enters the forecast.
   Resource potentials in GW only cap generation after an explicit capacity-factor conversion.
   See [data_audit.md](data_audit.md).
4. **Why does the Eastern region import in 2047?**
   Its requirement grows fastest because of population and income catch-up.
   Its solar and wind resource is small, and coal is limited to its fleet.
   Importing western and southern renewables is cheaper, so the plan argues for corridor expansion.
5. **Why so little nuclear?**
   At IEA cost projections, nuclear costs about the same as coal and more than renewables plus hydro.
   With a carbon price or 30 percent lower nuclear cost, nuclear and hydro displace coal.
   The sensitivity tab shows this.
6. **Why use a GPU?**
   The uncertainty analysis is millions of independent scenarios.
   The GPU runs 4 million in about 30 ms, which lets the dashboard recompute live as the jury changes assumptions.
   The CPU path gives identical results, checked by the parity test.
7. **Why does faster growth add only about 14 percent?**
   Within countries, income and urbanisation rise together.
   Both fitted models show income effects flattening at the income India reaches in the 2040s, so much of the historical rise is attributed to urbanisation.
   The high-growth preset therefore also raises urbanisation and adds new loads (green hydrogen, data centres).
   The low-carbon policy is cheaper than the base plan, and the energy-security policy fails under high growth (4.7 TWh unmet).
