# Building and deploying the dashboard

The dashboard (`apps/dashboard`) builds in two ways from the same C++ sources:

- **Native** (Windows, MSVC, optional CUDA): `build.cmd`.
- **Web** (WebAssembly with Emscripten, WebGL 2): `build_web.cmd`.
  The output in `web/dist` is a static site deployed on Vercel: a static landing page at `/` and the dashboard at `/app/`.

Both scripts keep their build trees outside OneDrive (`%USERPROFILE%\.e47build` and `%USERPROFILE%\.e47build_web`), because syncing build files slows builds.

## Native build

Needs Visual Studio 2022 Build Tools (MSVC, CMake, Ninja) and, for the GPU path, the CUDA toolkit.

```bat
build.cmd                      :: CUDA on when a CUDA compiler is found
build.cmd -DENABLE_CUDA=OFF    :: CPU only (OpenMP)
%USERPROFILE%\.e47build\apps\e47_dashboard.exe "<repo root>"
```

## Web build

### Install Emscripten (once)

The web build is tested with **Emscripten 7.0.0** (emsdk `7.0.0`).

```bat
git clone https://github.com/emscripten-core/emsdk.git %USERPROFILE%\emsdk
cd /d %USERPROFILE%\emsdk
emsdk install 7.0.0
emsdk activate 7.0.0
```

`build_web.cmd` finds emsdk in `%EMSDK%` or `%USERPROFILE%\emsdk` and loads its environment itself.
CMake and Ninja come from the Visual Studio Build Tools when they are not on `PATH`.
The first build takes about 10 minutes (Emscripten builds its system libraries once); later builds take a few minutes.

### Build

The landing page is rendered from `outputs/site_summary.json`, which only the native command-line run writes, so run that first (once, and again whenever the model or its inputs change):

```bat
build.cmd
%USERPROFILE%\.e47build\apps\energy2047.exe "<repo root>"   :: writes outputs\*.csv and outputs\site_summary.json
build_web.cmd
build_web.cmd -DE47_WEB_CROSS_COUNTRY=OFF   :: smaller data package, see "Data package"
```

`build_web.cmd` configures with `emcmake cmake` (the `if(EMSCRIPTEN)` branches in `CMakeLists.txt` and `cmake/*.cmake`, and `cmake/Web.cmake`), builds the `e47_site` target (which builds `e47_dashboard` first) and copies `<build>\site` into `web/dist`:

| File | Size | Sent by Vercel (Brotli) | Content |
| --- | --- | --- | --- |
| `index.html` | 64 KB | 14 KB | landing page: `web/landing.html` filled from `outputs/site_summary.json` |
| `summary.json` | 16 KB | 4 KB | copy of `outputs/site_summary.json`, linked from the page |
| `index.md` | | | Markdown version of the landing page: `web/landing.md` filled from the same summary; sent for `/` when the request asks for `Accept: text/markdown` |
| `robots.txt`, `vercel.json`, `llms.txt`, `sitemap.xml`, `og-image.png` | | | copied from `web/`; `llms.txt` tells agents what the site answers and where the numbers are; the preview image carries no model numbers |
| `about.html`, `contact.html`, `privacy.html`, `404.html`, `404.md`, `pages.css` | | | static pages copied from `web/`, served at `/about`, `/contact`, `/privacy`; the two 404 bodies (HTML, and Markdown for `Accept: text/markdown`) |
| `app/index.html` | 4 KB | 2 KB | dashboard page from `web/shell.html`: top bar with the link back, loading status, full-window canvas |
| `app/index.js` | 0.3 MB | 0.1 MB | Emscripten runtime and data-package index |
| `app/index.wasm` | 3.8 MB | 1.4 MB | dashboard, pipeline and HiGHS |
| `app/index.data` | 8.6 MB | 4.6 MB | CSV inputs and the dashboard font (LZ4 inside) |

A first visit to the landing page transfers about 14 KB; opening the dashboard transfers about 6.1 MB more.
`build_web.cmd` deletes `index.js`, `index.wasm` and `index.data` left at the root of `web/dist` by the earlier layout and keeps everything else there (for example the `.vercel` project link).
`web/dist` is not committed (`web/.gitignore`).
Building only the `e47_dashboard` target still produces the dashboard in `<build>/web` without the landing page; that target does not need the summary.

### Landing page

The page at `/` answers the brief without WebAssembly: plain HTML and CSS with inline SVG charts, no JavaScript, no fonts or scripts from other sites (the COOP/COEP headers below apply to it as well), about 14 KB over the network.
Every number on it comes from the model:

1. `energy2047` (`apps/energy2047.cpp`) collects the numbers from the same in-memory results that write the CSV files (forecast quantiles and path, holdout, sensitivity cases, regional requirement and NPP regional generation, the Robust plan from the preset matrix with its corridor flows, the base-policy plan, the P90 levers, and storage and land of the Robust median plan) and `write_site_summary()` writes them to `outputs/site_summary.json`.
   If a part is missing (for example the storage inputs failed to load) it deletes the file instead of writing stale numbers.
2. `cmake/Site.cmake` (CMake script mode, no other tools) fills `web/landing.html` from that JSON.
   Placeholders are JSON paths, for example `{{national.p50|0}}` (rounded, thousands separators), `{{holdout.0.error_pct|+1}}` (signed) or `{{plan.scale_pct|r3}}` (plain number for SVG); `<!--each plan.mix-->...<!--/each-->` repeats a block per array element.
   The syntax is listed at the top of the script.
   An unknown path or a leftover placeholder stops the build.
3. The numbers are embedded at build time, so the page needs neither `fetch` nor JavaScript and works on locked-down machines.

Render the page alone, for example after editing `web/landing.html`:

```bat
cmake -DSUMMARY=outputs\site_summary.json -DTEMPLATE=web\landing.html -DOUT=web\dist\index.html -P cmake\Site.cmake
```

Design: dark control-room look, golden-ratio spacing and type steps as in the deck, the deck's validated series colours (text never uses series colours).
Accessibility: skip link, landmarks, one `h1`, every chart an SVG with `role="img"`, title and description, plus a data table under "Show the numbers"; colour is never the only cue (direct labels, bar position, outline for "today").
The region map is a schematic of the five grid regions (node positions only suggest geography; no borders are drawn).

### What differs in the web build

| | Native | Web |
| --- | --- | --- |
| Compiler | MSVC, `/O2 /arch:AVX2`, OpenMP, CUDA | Emscripten (clang), `-O3 -msimd128`, `E47_WEB=1` |
| Monte Carlo | CUDA GPU or OpenMP on all cores | CPU, one thread (WebAssembly has no OpenMP runtime; the OpenMP loops run serially) |
| Background work | `std::async` threads | the same code on web workers (`-pthread`, pool of 4) |
| Window and GL | GLFW 3.4, OpenGL 3.3 core | Emscripten's GLFW 3 (`-sUSE_GLFW=3`), WebGL 2, `#version 300 es` shaders, device pixel ratio |
| LP solver | HiGHS v1.11.0, multithreaded | HiGHS v1.11.0, single thread (`HIGHS_NO_DEFAULT_THREADS`) |
| Data | read from `<repo>/data` | packaged into `index.data` (LZ4) and mounted at `/data` |
| Font | Segoe UI | Roboto (from the Dear ImGui sources) |
| Screenshots (`--shot`) | yes: `docs/img/1_*.png` to `6_*.png` (one per tab) and the data tab's evidence views `6b_data_requirement_evidence.png`, `6c_data_mix_evidence.png` | no |

`E47_WEB` is defined for all project code in the web build and selects smaller defaults (`kDefaultDrawsLog2`, `kDefaultBootstrapReplicates` in `src/pipeline/pipeline.hpp`).

### Data package

Only the files the pipeline reads are packaged (`cmake/Web.cmake`):

1. every `"data/....csv"` path literal in `src/` (found automatically at configure time),
2. files whose path is assembled in code (`E47_WEB_DATA_ASSEMBLED` in `cmake/Web.cmake`),
3. the cross-country panel inputs (Ember global generation, World Bank `API_*.csv`), unless `-DE47_WEB_CROSS_COUNTRY=OFF`.

The cross-country inputs are 16 MB of CSV (6.4 MB of the 7 MB package) and are read only when the "Cross-country (check)" core is selected.
With `E47_WEB_CROSS_COUNTRY=OFF` the package is 0.6 MB, but that core then fails in the browser.

When a new input file is opened with a path built from pieces, add its full path to `E47_WEB_DATA_ASSEMBLED`.
Configure stops with an error on any `.csv` path piece in `src/` that matches no packaged file, and warns about packaged paths that do not exist (they are left out).

### Test locally

The dashboard needs cross-origin isolation (`SharedArrayBuffer` for the worker threads), so the server must send

```text
Cross-Origin-Opener-Policy: same-origin
Cross-Origin-Embedder-Policy: require-corp
```

`emrun` from emsdk sends them:

```bat
call %USERPROFILE%\emsdk\emsdk_env.bat
emrun --no_browser --port 8080 web\dist\index.html
```

Then open <http://localhost:8080/> (landing page) and <http://localhost:8080/app/> (dashboard).
A plain static server (`python -m http.server`, `npx http-server`) does not send the headers; the landing page still works, but the dashboard then shows which headers are missing instead of starting.

## Deploy to Vercel

Live site: <https://india-energy-2047.vercel.app> (Vercel project `india-energy-2047`, Hobby plan).

| Path | Content |
| --- | --- |
| `/` | landing page (static HTML, the answer in text, charts and tables) |
| `/app/` | interactive dashboard (WebAssembly); `/app` redirects to `/app/` so its relative file names resolve |
| `/summary.json` | the numbers on the landing page |

`web/dist` is deployed as a static site; no build runs on Vercel.
`web/vercel.json` (copied into `web/dist`) uses Vercel `routes`: the `/app` redirect, the COOP/COEP/CORP headers for every path, `application/wasm` for `.wasm`, `Cache-Control: public, max-age=0, must-revalidate`, Markdown for `/` when the request sends `Accept: text/markdown` (with `Vary: Accept`), clean URLs for the about, contact and privacy pages, and a 404 status with an HTML or Markdown body for missing paths.
The landing page has no images of model results: its dashboard preview is drawn from the summary when the page is rendered, so it always matches the numbers around it. "Run the live model here" loads the dashboard in a frame only when clicked.
The landing page loads nothing from other origins, so the headers do not affect it.
File names are not content-hashed, so browsers revalidate with the ETag rather than keep a stale `.wasm` after a redeploy.

First time (sign-in opens a browser page):

```bat
cd /d web\dist
vercel login
vercel link --yes --project india-energy-2047
vercel deploy --prod --yes
```

Redeploy, whenever the model or the page changes:

```bat
build.cmd
%USERPROFILE%\.e47build\apps\energy2047.exe "<repo root>"
build_web.cmd
cd /d web\dist
vercel deploy --prod --yes
```

`build_web.cmd` replaces only the site files in `web/dist`, so the `.vercel` project link stays; if it is missing, run `vercel link --yes --project india-energy-2047` again.
`vercel deploy` prints the deployment URL; the production domain follows within seconds.
Check it:

```bat
curl -sI https://india-energy-2047.vercel.app/
curl -sI https://india-energy-2047.vercel.app/app/
```

Both should return `200`, `Cross-Origin-Opener-Policy: same-origin` and `Cross-Origin-Embedder-Policy: require-corp`.

## Limitations of the web version

Measured in Chrome on the development laptop (16 logical cores, served from localhost):

| Step | Web |
| --- | --- |
| Page shown | 0.4 s |
| Data loaded and India-only models fitted | about 1 s |
| First full dashboard (65,536 draws, LP plan, 28-LP tornado) | 5.5 to 6 s |
| Monte Carlo, 65,536 draws | 3.4 s (about 19,000 draws/s; native GPU: 4,194,304 draws in 31 ms) |
| Scenario or policy change (full update) | 4.0 to 4.6 s |
| Switch to the cross-country core | about 19 s (16 MB CSV, model fits) |

- **No GPU and no CUDA.** The Monte Carlo runs on the CPU in WebAssembly.
- **One compute thread.** WebAssembly has no OpenMP runtime, so the Monte Carlo, the bootstrap and the tornado LP runs are serial. The page stays responsive because the work runs on a web worker; the header shows "Recomputing..." meanwhile.
- **Fewer draws and replicates.** The web defaults are 2^16 Monte Carlo draws (native 2^22) and 300 bootstrap replicates (native 2,000). The "Scenarios (2^n)" slider still goes higher, at about 3.4 s per 2^16 draws. Medians and 90% ranges agree with the native build to within about 0.5%.
- **Download.** About 6 MB on first load with Vercel's Brotli compression (12.7 MB uncompressed, most of it the cross-country CSV), then cached.
- **Browser requirements.** WebGL 2, WebAssembly SIMD and exceptions, and `SharedArrayBuffer`: current Chrome, Edge and Firefox, Safari 16.4 or later.
