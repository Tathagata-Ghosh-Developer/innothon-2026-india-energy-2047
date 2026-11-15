#!/usr/bin/env bash
# Build the WebAssembly dashboard with Emscripten and stage a static site in web/dist:
# landing page (web/landing.html + outputs/site_summary.json) at the root, dashboard in app/.
# Linux and macOS counterpart of build_web.cmd: same CMake configuration, target and staged files.
# Usage: ./build_web.sh [extra CMake options]
# Needs: emsdk in $EMSDK or ~/emsdk (see docs/deployment.md), or emcmake already on PATH,
# CMake 3.26+ and Ninja on PATH, and outputs/site_summary.json from a native run:
# energy2047 "<repo root>".
# Build directory: $E47_WEB_BUILD_DIR, default ~/.e47build_web.
set -euo pipefail

SRC="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD="${E47_WEB_BUILD_DIR:-$HOME/.e47build_web}"
DIST="$SRC/web/dist"

if ! command -v emcmake >/dev/null 2>&1; then
  EMSDK="${EMSDK:-$HOME/emsdk}"
  if [ ! -f "$EMSDK/emsdk_env.sh" ]; then
    echo "emsdk not found in \"$EMSDK\". Install it as described in docs/deployment.md." >&2
    exit 1
  fi
  # shellcheck disable=SC1091
  source "$EMSDK/emsdk_env.sh" >/dev/null 2>&1
fi

emcmake cmake -S "$SRC" -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Release "$@"
# e47_site builds the dashboard, renders the landing page from outputs/site_summary.json
# (written by the native energy2047 run) and stages both in $BUILD/site.
cmake --build "$BUILD" --target e47_site --parallel

# Stage the site: landing page at the root, dashboard under app/.
# Other files in web/dist (for example the .vercel project link) are kept.
mkdir -p "$DIST"
# Remove the dashboard files of the earlier layout (dashboard at the root).
rm -f "$DIST/index.js" "$DIST/index.wasm" "$DIST/index.data"
cp -R "$BUILD/site/." "$DIST/"
echo "Site staged in $DIST: landing page index.html, dashboard app/"
