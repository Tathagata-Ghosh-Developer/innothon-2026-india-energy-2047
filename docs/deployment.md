# Building and deploying the dashboard

The dashboard (`apps/dashboard`) builds in two ways from the same C++ sources:

- **Native** (Windows, MSVC, optional CUDA): `build.cmd`.
- **Web** (WebAssembly with Emscripten, WebGL 2): `build_web.cmd`.
  The output in `web/dist` is a static site deployed on Vercel.

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

```bat
build_web.cmd
build_web.cmd -DE47_WEB_CROSS_COUNTRY=OFF   :: smaller data package, see "Data package"
```

This configures with `emcmake cmake` (the `if(EMSCRIPTEN)` branches in `CMakeLists.txt` and `cmake/*.cmake`, and `cmake/Web.cmake`), builds only the `e47_dashboard` target and stages the site:

| File | Size | Sent by Vercel (Brotli) | Content |
| --- | --- | --- | --- |
| `index.html` | 3 KB | 1 KB | page from `web/shell.html`: loading indicator, full-window canvas |
| `index.js` | 0.3 MB | 0.05 MB | Emscripten runtime and data-package index |
| `index.wasm` | 3.6 MB | 1.1 MB | dashboard, pipeline and HiGHS |
| `index.data` | 6.7 MB | 6.7 MB (LZ4 inside) | CSV inputs and the dashboard font |
| `vercel.json` | | | headers, copied from `web/vercel.json` |

`web/dist` is not committed (`web/.gitignore`).

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
| Screenshots (`--shot`) | yes | no |

`E47_WEB` is defined for all project code in the web build and selects smaller defaults (`kDefaultDrawsLog2`, `kDefaultBootstrapReplicates` in `src/pipeline/pipeline.hpp`).

### Data package

Only the files the pipeline reads are packaged (`cmake/Web.cmake`):

1. every `"data/....csv"` path literal in `src/` (found automatically at configure time),
2. files whose path is assembled in code (`E47_WEB_DATA_ASSEMBLED` in `cmake/Web.cmake`),
3. the cross-country panel inputs (Ember global generation, World Bank `API_*.csv`), unless `-DE47_WEB_CROSS_COUNTRY=OFF`.

The cross-country inputs are 16 MB of CSV (6.4 MB of the 7 MB package) and are read only when the "Cross-country (check)" core is selected.
With `E47_WEB_CROSS_COUNTRY=OFF` the package is 0.6 MB, but that core then fails in the browser.

When a new input file is opened with a path built from pieces, add its full path to `E47_WEB_DATA_ASSEMBLED`.
Configure warns about any `.csv` path piece in `src/` that matches no packaged file, and about packaged paths that do not exist.

### Test locally

The page needs cross-origin isolation (`SharedArrayBuffer` for the worker threads), so the server must send

```text
Cross-Origin-Opener-Policy: same-origin
Cross-Origin-Embedder-Policy: require-corp
```

`emrun` from emsdk sends them:

```bat
call %USERPROFILE%\emsdk\emsdk_env.bat
emrun --no_browser --port 8080 web\dist\index.html
```

Then open <http://localhost:8080/index.html>.
A plain static server (`python -m http.server`, `npx http-server`) does not send the headers; the page then shows which headers are missing instead of starting.

## Deploy to Vercel

Live site: <https://india-energy-2047.vercel.app> (Vercel project `india-energy-2047`, Hobby plan).

`web/dist` is deployed as a static site; no build runs on Vercel.
`web/vercel.json` (copied into `web/dist`) sets the COOP/COEP headers, `application/wasm` for `.wasm`, and `Cache-Control: public, max-age=0, must-revalidate`.
File names are not content-hashed, so browsers revalidate with the ETag rather than keep a stale `.wasm` after a redeploy.

First time (sign-in opens a browser page; the project name is an example):

```bat
cd /d web\dist
npx vercel login
npx vercel link --yes --project india-energy-2047
npx vercel deploy --prod
```

Later deployments, after `build_web.cmd`:

```bat
cd /d web\dist
npx vercel deploy --prod
```

`build_web.cmd` replaces only the site files in `web/dist`, so the `.vercel` project link stays.
`npx vercel deploy` prints the deployment URL.

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
- **Download.** About 8 MB on first load (most of it the cross-country CSV), then cached.
- **Browser requirements.** WebGL 2, WebAssembly SIMD and exceptions, and `SharedArrayBuffer`: current Chrome, Edge and Firefox, Safari 16.4 or later.
