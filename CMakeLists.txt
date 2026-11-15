cmake_minimum_required(VERSION 3.26)
project(energy2047 VERSION 0.1.0 LANGUAGES CXX)

option(ENABLE_CUDA "Build the CUDA Monte Carlo and sensitivity kernels" ON)
option(BUILD_DASHBOARD "Build the Dear ImGui dashboard" ON)
option(BUILD_TESTS "Build unit and parity tests" ON)

# WebAssembly build (emcmake, see build_web.cmd and docs/deployment.md):
# CPU only, no OpenMP, dashboard only.
if(EMSCRIPTEN)
  enable_language(C)  # HiGHS has C sources; enabled here so cmake/CompilerFlags.cmake can extend the C flags
  set(ENABLE_CUDA OFF CACHE BOOL "" FORCE)
  set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(BUILD_DASHBOARD ON CACHE BOOL "" FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

if(ENABLE_CUDA)
  include(CheckLanguage)
  check_language(CUDA)
  if(CMAKE_CUDA_COMPILER)
    # Ship machine code (SASS) for the target GPU, not only PTX: a toolkit newer
    # than the installed driver cannot JIT-compile its own PTX. RTX 30-series = 86.
    # Other GPUs: configure with -DCMAKE_CUDA_ARCHITECTURES=native
    if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
      set(CMAKE_CUDA_ARCHITECTURES 86-real)
    endif()
    enable_language(CUDA)
    set(CMAKE_CUDA_STANDARD 20)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
  else()
    message(WARNING "No CUDA compiler found; building the CPU path only")
    set(ENABLE_CUDA OFF)
  endif()
endif()

include(cmake/CompilerFlags.cmake)
include(cmake/Dependencies.cmake)

add_subdirectory(src)
add_subdirectory(apps)
if(EMSCRIPTEN)
  include(cmake/Web.cmake)
endif()
if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
