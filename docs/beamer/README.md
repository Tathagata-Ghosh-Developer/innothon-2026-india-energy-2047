# Presentation deck (LaTeX Beamer)

The core forecast is the India-only model, built only on the two challenge datasets (National Power Portal generation; Renewable India state population and GSDP), as the organisers ruled at the kickoff on 13 Nov 2026.
The cross-country model (Ember and World Bank, 153 countries) appears only as an independent check on non-core data.

| File | Contents |
| --- | --- |
| `main.pdf` | Presentation, 28 pages: 18 slides, 6 pages of references and 4 backup slides |
| `reader.pdf` | Reader's edition, 50 pages: the same deck with a reader's note page after every slide (message, evidence, likely questions) |
| `midpoint.pdf`, `midpoint.png` | One 16:9 slide for the midpoint review of 18 Nov 2026: approach, findings, roadblocks (PNG rendered at 220 dpi) |
| `main.tex` | Deck source; `reader.tex` sets a flag and inputs it |
| `midpoint.tex` | Midpoint slide source (same theme; columns in golden proportion 1 : phi : phi) |
| `slide_alignment.tex`, `slide_novelty.tex` | ExxonMobil alignment and state-of-practice slides |
| `beamerthemeIndia2047.sty` | Theme |
| `refs_data.bib` | Data sources: the challenge datasets (NPP, Renewable India) first, then drivers, limits and the cross-country check data |
| `refs_web.bib` | ExxonMobil, government and agency web sources |
| `refs_academic.bib` | Papers, each with a DOI checked on Crossref |
| `research_notes.md` | Every quote used, verbatim, with page and URL; unverified items listed separately |
| `data/` | Copies of the model's `outputs/` tables that the charts and slides read (all except `bootstrap_params.csv`, which no slide uses) |

## Slides

| # | Slide | Main numbers from |
| --- | --- | --- |
| 1 | Title: 4,703 TWh median utility generation in FY2047-48 | `forecast_national_regional.csv`, `core_comparison.csv` |
| 2 | The answer on one page | `holdout.csv`, `sensitivity_2047.csv`, `presets.csv` |
| 3 | Alignment with ExxonMobil's Global Outlook 2026 | `presets.csv` |
| 4 | The core uses only the two challenge datasets | `india_fit.csv` |
| 5 | Five advances over the state of practice | several |
| 6 | Approach in six steps | `india_fit.csv` |
| 7 | Why the elasticity is 0.83 | `india_fit.csv`, `sensitivity_2047.csv` |
| 8 | Fan chart from NPP history, with the CEA plan, the captive layer and the cross-country check | `forecast_national_regional.csv`, `holdout.csv`, `core_comparison.csv` |
| 9 | Backtest and holdout | `backtest.csv`, `backtest_horizon.csv`, `holdout.csv` |
| 10 | Sensitivity: income growth dominates | `sensitivity_2047.csv` |
| 11 | Regional transfer gap | `data/raw/npp/npp_region_generation_fy.csv`, `strategy_2047_regions.csv`, `forecast_national_regional.csv` |
| 12 | Least-cost mix by region, base policy | `strategy_2047.csv`, `corridors_2047.csv`, `presets.csv` |
| 13 | Stress test of the base plan | `tornado.csv` |
| 14 | Robust plan vs base plan | `presets.csv`, `p90_levers.csv` |
| 15 | Cost-CO2 trade-off of the robust plan: deeper cuts cost almost nothing until the coal fleet floor binds | `pareto_2047.csv`, `presets.csv`, `strategy_2047.csv` |
| 16 | GPU engine timings | `timings.csv` |
| 17 | Live dashboard | `docs/img/1_national_forecast.png` |
| 18 | Recommendations and next steps | slides 10 to 15 |
| B1-B4 | Core vs check; assumptions; preset matrix; data limits | `core_comparison.csv`, `india_fit.csv`, `presets.csv` |

## Design

| Element | Rule |
| --- | --- |
| Column split | 0.605 / 0.365 (golden ratio with a 3% gutter); the midpoint slide uses 0.229 / 0.371 / 0.371 |
| Type scale | Steps by the golden ratio from a 10 pt body: 16.2, 26.2 and 42.4 pt |
| Numbers | The largest element on every slide |
| Title slide | A golden rectangle with its nested squares and spiral |
| Chart colours | Validated for colour-vision deficiency, in fixed order: coal, nuclear, wind, hydro, solar, firmed renewables; gas and bioenergy in neutral greys |
| Evidence | Every content slide ends with an evidence line citing the source and the model output file behind its numbers |
| Years | Fiscal years labelled by start year, as in the model: 2023 = FY2023-24, 2047 = FY2047-48 |

## Build

MiKTeX or TeX Live with pdflatex, biber and latexmk:

```bat
cd docs\beamer
latexmk -pdf main.tex
latexmk -pdf reader.tex
latexmk -pdf midpoint.tex
pdftoppm -r 220 -png -singlefile midpoint.pdf midpoint
```

`pdftoppm` ships with MiKTeX.

## Refreshing the numbers

1. Run `energy2047 <repo_root>` to regenerate the outputs.
2. Copy `outputs\*.csv` into `data\` (`bootstrap_params.csv` is not needed).
3. The fan chart (slide 8), the backtest chart (slide 9) and the trade-off chart (slide 15) read the CSV files directly. Every other chart and table carries its values inline and names its source file in a comment next to the values: update those values from the new CSV files.
4. The regional generation shares on slide 11 come from `data/raw/npp/npp_region_generation_fy.csv` (FY2025-26 totals, Bhutan import excluded).
5. Rebuild the three PDFs and the PNG, then check every page for clipped or overlapping text.
