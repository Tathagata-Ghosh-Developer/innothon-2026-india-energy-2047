# Presentation deck (LaTeX Beamer)

| File | Contents |
| --- | --- |
| `main.pdf` | Presentation: 16 slides, references and backup slides |
| `reader.pdf` | Reader's edition: the same deck with a reader's note page after every slide, giving the message, the evidence and likely questions |
| `main.tex` | Deck source |
| `slide_alignment.tex`, `slide_novelty.tex` | ExxonMobil alignment and state-of-practice slides |
| `beamerthemeIndia2047.sty` | Theme |
| `refs_data.bib` | Data sources used by the model |
| `refs_web.bib` | ExxonMobil, government and agency web sources |
| `refs_academic.bib` | Papers, each with a DOI |
| `research_notes.md` | Every quote used, verbatim, with page and URL; unverified items listed separately |
| `data/` | Copies of the model's `outputs/` tables that the charts read |

## Design

| Element | Rule |
| --- | --- |
| Column split | 0.605 / 0.365 (golden ratio with a 3% gutter) |
| Type scale | Steps by the golden ratio from a 10 pt body: 16.2, 26.2 and 42.4 pt |
| Numbers | The largest element on every slide |
| Title slide | A golden rectangle with its nested squares and spiral |
| Chart colours | Validated for colour-vision deficiency, in fixed order: coal, nuclear, wind, hydro, solar, firmed renewables |
| Evidence | Every content slide ends with an evidence line citing the source and the model output file behind its numbers |

## Build

MiKTeX or TeX Live with pdflatex, biber and latexmk:

```bat
cd docs\beamer
latexmk -pdf main.tex
latexmk -pdf reader.tex
```

## Refreshing the numbers

1. Run `energy2047 <repo_root>` to regenerate the outputs.
2. Copy `outputs\*.csv` into `data\`.
3. Rebuild the deck.

The fan chart and back-test charts read the CSV files directly.
The bar charts carry their values inline, and each one names its source file in a comment.
