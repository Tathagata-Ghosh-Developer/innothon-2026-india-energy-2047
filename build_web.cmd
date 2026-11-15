@echo off
rem Build the WebAssembly dashboard with Emscripten and stage a static site in web\dist.
rem Usage: build_web.cmd [extra CMake options]
rem Needs: emsdk in %EMSDK% or %USERPROFILE%\emsdk (see docs/deployment.md), and
rem CMake + Ninja (taken from Visual Studio 2022 Build Tools when not on PATH).
rem Build directory: %E47_WEB_BUILD_DIR%, default %USERPROFILE%\.e47build_web
rem (kept outside OneDrive so build files are not synced).
setlocal
set "SRC=%~dp0"
set "BUILD=%E47_WEB_BUILD_DIR%"
if "%BUILD%"=="" set "BUILD=%USERPROFILE%\.e47build_web"
set "DIST=%SRC%web\dist"

if "%EMSDK%"=="" set "EMSDK=%USERPROFILE%\emsdk"
if not exist "%EMSDK%\emsdk_env.bat" (
  echo emsdk not found in "%EMSDK%". Install it as described in docs\deployment.md.
  exit /b 1
)
call "%EMSDK%\emsdk_env.bat" >nul 2>&1

where cmake >nul 2>&1 && where ninja >nul 2>&1 && goto have_tools
set "VSCMAKE=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake"
if not exist "%VSCMAKE%\CMake\bin\cmake.exe" set "VSCMAKE=%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake"
set "PATH=%VSCMAKE%\CMake\bin;%VSCMAKE%\Ninja;%PATH%"
:have_tools

call emcmake cmake -S "%SRC%." -B "%BUILD%" -G Ninja -DCMAKE_BUILD_TYPE=Release %* || exit /b 1
cmake --build "%BUILD%" --target e47_dashboard --parallel || exit /b 1

rem Stage the site. Other files in web\dist (for example the .vercel project link) are kept.
if not exist "%DIST%" mkdir "%DIST%"
for %%f in (index.html index.js index.wasm index.data) do (
  copy /y "%BUILD%\web\%%f" "%DIST%\%%f" >nul || exit /b 1
)
copy /y "%SRC%web\vercel.json" "%DIST%\vercel.json" >nul || exit /b 1
echo Site staged in %DIST%
endlocal
