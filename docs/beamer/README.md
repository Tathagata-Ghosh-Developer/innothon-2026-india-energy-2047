# Presentation deck (LaTeX Beamer)

The core forecast is the India-only model, built only on the two challenge datasets (National Power Portal generation; Renewable India state population and GSDP), as the organisers ruled at the kickoff on 13 Nov 2026.
These two are the primary sources; every other source in the deck is secondary and labelled as such.
The cross-country model (Ember and World Bank, 153 countries) appears only as an independent check on non-core data, used for justification and not for estimation.
The deck follows the organisers' written ruling of 16 Nov 2026 (`docs/decisions.md`, D12): no captive layer, the regional split from the challenge data, and the headline as the country's total in TWh.
For the final evaluation (21-22 Nov 2026) it adds the answers to the midpoint panel (`docs/decisions.md`, D13): what drives 2047 beyond income, income state by state, and the published figures that choose the mix.

| File | Contents |
| --- | --- |
| `main.pdf` | Presentation, 36 pages: 20 slides (title and 19 content slides), 9 pages of references and 7 backup slides |
| `reader.pdf` | Reader's edition, 75 pages: the same deck with a reader's note page after every slide (message, evidence, likely questions and answers); 12 continuation note pages carry the longer notes of slides 5, 6, 7, 8, 11, 12, 14, 15 and 20 |
| `midpoint.pdf`, `midpoint.png` | One 16:9 slide for the midpoint review of 18 Nov 2026: approach, findings, roadblocks (PNG rendered at 220 dpi). Kept as presented; its numbers are still current |
| `main.tex` | Deck source; `reader.tex` sets a flag and inputs it |
| `midpoint.tex` | Midpoint slide source (same theme; columns in golden proportion 1 : phi : phi) |
| `slide_alignment.tex`, `slide_novelty.tex` | ExxonMobil alignment and state-of-practice slides |
| `beamerthemeIndia2047.sty` | Theme |
| `refs_data.bib` | Data sources: the challenge datasets (NPP, Renewable India) first, then drivers, limits, the published figures of the mix (secondary) and the cross-country check data |
| `refs_web.bib` | ExxonMobil, government and agency web sources, and the official challenge page |
| `refs_academic.bib` | Papers, each with a DOI checked on Crossref |
| `research_notes.md` | Every quote used, verbatim, with page and URL; unverified items listed separately |
| `data/` | Copies of the model's `outputs/` tables and `site_summary.json` that the charts and slides read (all except `bootstrap_params.csv`, which no slide uses) |

## Slides

| # | Slide | Main numbers from |
| --- | --- | --- |
| 1 | Title: 4,703 TWh median utility generation in FY2047-48 | `forecast_national_regional.csv`, `core_comparison.csv` |
| 2 | The answer on one page | `holdout.csv`, `sensitivity_2047.csv`, `growth_decomposition_2047.csv`, `presets.csv` |
| 3 | Alignment with ExxonMobil's Global Outlook 2026 | `presets.csv` |
| 4 | The core uses only the two challenge datasets | `india_fit.csv` |
| 5 | Five advances over the state of practice | several |
| 6 | Approach in six steps | `india_fit.csv` |
| 7 | Why the elasticity is 0.83 | `india_fit.csv`, `sensitivity_2047.csv` |
| 8 | Fan chart from NPP history, with the CEA plan and the cross-country check | `forecast_national_regional.csv`, `holdout.csv`, `core_comparison.csv` |
| 9 | Backtest and holdout | `backtest.csv`, `backtest_horizon.csv`, `holdout.csv` |
| 10 | Sensitivity: income growth dominates | `sensitivity_2047.csv` |
| 11 | **New.** What drives 2047 beyond income: growth decomposition, elasticity sensitivities, new loads (outside the headline) | `growth_decomposition_2047.csv`, `sensitivity_2047.csv`, `new_loads_2047.csv` |
| 12 | **New.** Income is used state by state; the elasticity is national: state table, regional sums, state panel check | `state_income_2047.csv`, `state_panel_check.csv`, `forecast_national_regional.csv` |
| 13 | Regional transfer gap | `data/raw/npp/npp_region_generation_fy.csv`, `strategy_2047_regions.csv`, `forecast_national_regional.csv` |
| 14 | **New.** Published figures choose the mix: build limits, NPP fleet floors, potentials, CEA v22 factors, IEA vs CEA cost basis, mix sensitivities, the organisers' reply | `sensitivity_mix.csv`, `committed_builds_2047.csv`, `presets.csv`, `data/external/techno_economic/build_limits.csv`, `data/external/resource_potential.csv` |
| 15 | **New.** Six sources, each where it is strongest: the robust plan by source and region, committed builds, gas for peaking, storage and land, the low-demand limit | `site_summary.json` (`plan.vectors`, `plan.committed`, `storage`, `land`), `committed_builds_2047.csv`, `storage_land_2047.csv` |
| 16 | Robust plan vs base plan | `presets.csv`, `p90_levers.csv` |
| 17 | Cost-CO2 trade-off of the robust plan: deeper cuts cost almost nothing until the coal fleet floor binds | `pareto_2047.csv`, `presets.csv`, `strategy_2047.csv` |
| 18 | GPU engine timings | `timings.csv` |
| 19 | Live dashboard | `docs/img/1_national_forecast.png` |
| 20 | Recommendations and next steps | slides 10 to 17 |
| B1-B4 | Core vs check; assumptions; preset matrix with the P05 test; data limits and hurdles | `core_comparison.csv`, `india_fit.csv`, `presets.csv`, `state_panel_check.csv` |
| B5 | State population paths against the challenge dataset (the ruling's consistency condition) | `population_consistency.csv` |
| B6 | Least-cost mix by region, base policy (slide 12 at the midpoint) | `strategy_2047.csv`, `corridors_2047.csv`, `presets.csv` |
| B7 | Stress test of the base plan (slide 13 at the midpoint) | `tornado.csv` |

The reader's edition carries likely jury questions with answers on every note page; the four new slides (11, 12, 14, 15) each have four or five.

## Design

| Element | Rule |
| --- | --- |
| Column split | 0.605 / 0.365 (golden ratio with a 3% gutter); the midpoint slide uses 0.229 / 0.371 / 0.371 |
| Type scale | Steps by the golden ratio from a 10 pt body: 16.2, 26.2 and 42.4 pt |
| Numbers | The largest element on every slide |
| Title slide | A golden rectangle with its nested squares and spiral |
| Chart colours | Validated for colour-vision deficiency, in fixed order: coal, nuclear, wind, hydro, solar, firmed renewables; gas and bioenergy in neutral greys. Every chart has one value axis and direct labels |
| Heatmap (slide 15) | One hue (navy), light to dark by the region's share of each source's TWh; the TWh value is printed in every cell |
| Evidence | Every content slide ends with an evidence line citing the source and the model output file behind its numbers; secondary sources are labelled |
| Years | Fiscal years labelled by start year, as in the model: 2023 = FY2023-24, 2047 = FY2047-48 |
| Capacity | GW appears only as published limits converted to TWh with a stated CUF, and as the storage the plan needs; never as a forecast |

## Build

MiKTeX or TeX Live with pdflatex, biber and latexmk:

```bat
cd docs\beamer
latexmk -pdf main.tex
latexmk -pdf reader.tex
latexmk -pdf midpoint.tex
pdftoppm -r 220 -png -singlefile midpoint.pdf midpoint
```

`pdftoppm` ships with MiKTeX. To check pages visually: `pdftoppm -r 120 -png main.pdf page`.

A full build takes several minutes. To check the layout of a few slides only, build a throwaway wrapper (not committed) that names their frame labels, for example `\def\onlyframes{s11,s12,s14,s15}\input{main.tex}`, and run pdflatex on it twice; the labels are `s2` to `s20` for the main slides and `b2` to `b7` for backups.

Reader's notes: a note page holds about 20 lines. A longer note continues on its own page through a `\note{...}` placed right after `\end{frame}`; the presentation ignores it. Overfull warnings do not catch every clipped note page, so look at the bottom of each note page after a change.

## Refreshing the numbers

1. Run `energy2047 <repo_root>` to regenerate the outputs.
2. Copy `outputs\*.csv` and `outputs\site_summary.json` into `data\` (`bootstrap_params.csv` is not needed).
3. The fan chart (slide 8), the backtest chart (slide 9), the trade-off chart (slide 17) and the population chart (backup B5) read the CSV files directly. Every other chart and table carries its values inline and names its source file in a comment next to the values: update those values from the new files. The heatmap on slide 15 takes `plan.vectors` from `site_summary.json`; the dot plot on slide 12 takes the `state` rows of `state_panel_check.csv`.
4. Slide 14's published additions and potentials are computed from `data/external/techno_economic/build_limits.csv` and `data/external/resource_potential.csv` as the model does (GW x CUF x 8.76; regional CUFs from `capacity_factors.csv`); its binding values match `presets.csv` and `site_summary.json`.
5. The regional generation shares on slide 13 come from `data/raw/npp/npp_region_generation_fy.csv` (FY2025-26 totals, Bhutan import excluded).
6. Rebuild the three PDFs and the PNG, then check every page for clipped or overlapping text.
