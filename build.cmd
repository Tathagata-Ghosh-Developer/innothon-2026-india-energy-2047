@echo off
rem Configure and build with MSVC + Ninja from any shell.
rem Usage: build.cmd [extra CMake options, e.g. -DENABLE_CUDA=OFF]
rem Build directory: %E47_BUILD_DIR%, default %USERPROFILE%\.e47build
rem (kept outside OneDrive so build files are not synced).
setlocal
set "SRC=%~dp0"
set "BUILD=%E47_BUILD_DIR%"
if "%BUILD%"=="" set "BUILD=%USERPROFILE%\.e47build"
set "VSROOT=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools"
if not exist "%VSROOT%\VC\Auxiliary\Build\vcvars64.bat" set "VSROOT=%ProgramFiles%\Microsoft Visual Studio\2022\Community"
set "PATH=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer;%PATH%"
call "%VSROOT%\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
cmake -S "%SRC%." -B "%BUILD%" -G Ninja -DCMAKE_BUILD_TYPE=Release %* || exit /b 1
cmake --build "%BUILD%" --parallel || exit /b 1
endlocal
