# WebAssembly dashboard (Emscripten): data package, HTML shell and link settings.
# Output: <build>/web/index.html, index.js, index.wasm, index.data.
# Target e47_site (end of file) stages the full site, landing page plus the dashboard
# under app/, in <build>/site; build_web.cmd copies it into web/dist.

# Only the dashboard is built for the browser.
set_target_properties(energy2047 PROPERTIES EXCLUDE_FROM_ALL TRUE)

# --- Data package -------------------------------------------------------------
# The pipeline opens <root>/data/...; the package is mounted at "/" and the
# dashboard uses root "/". Packaged files:
#  1. every "data/....csv" path literal in src/ (found automatically),
#  2. files whose path the code assembles from pieces (listed below),
#  minus the cross-country panel inputs when E47_WEB_CROSS_COUNTRY is OFF.
# The cross-country inputs (16 MB raw, about 6.4 MB of the 7 MB package) are read only
# when the dashboard's "Cross-country (check)" core is selected. OFF gives a 0.6 MB
# package, but then that button fails in the browser unless app.cpp hides it.
option(E47_WEB_CROSS_COUNTRY
  "Package the cross-country panel inputs (only CoreModel::kCrossCountry reads them)" ON)
file(GLOB _e47_wdi RELATIVE "${PROJECT_SOURCE_DIR}"
  "${PROJECT_SOURCE_DIR}/data/raw/worldbank/downloads/*/API_*.csv")
set(E47_WEB_DATA_CROSS_COUNTRY
  data/raw/generation/downloads/ember/release_generation_yearly_global.csv
  ${_e47_wdi})  # found by directory listing in pipeline.cpp

set(E47_WEB_DATA_ASSEMBLED
  # src/allocation/inputs.cpp
  data/raw/generation/state/state_region_map.csv
  data/raw/generation/national/ember_national_generation_cy.csv
  data/external/resource_potential.csv
  data/external/techno_economic/capacity_factors.csv
  data/external/techno_economic/offshore_wind.csv
  data/external/techno_economic/lcoe.csv
  data/external/techno_economic/emission_factors.csv
  data/external/techno_economic/nuclear_pipeline.csv
  data/external/techno_economic/transmission.csv)

file(GLOB_RECURSE _e47_sources "${PROJECT_SOURCE_DIR}/src/*.cpp" "${PROJECT_SOURCE_DIR}/src/*.hpp")
set(_e47_full "")
set(_e47_partial "")
foreach(_f IN LISTS _e47_sources)
  file(READ "${_f}" _text)
  string(REGEX MATCHALL "\"[^\"\n]*\\.csv\"" _lits "${_text}")
  foreach(_l IN LISTS _lits)
    string(REPLACE "\"" "" _l "${_l}")
    if(_l MATCHES "^data/")
      list(APPEND _e47_full "${_l}")
    else()
      list(APPEND _e47_partial "${_l}")
    endif()
  endforeach()
endforeach()
set(E47_WEB_DATA ${_e47_full} ${E47_WEB_DATA_ASSEMBLED})
if(E47_WEB_CROSS_COUNTRY)
  list(APPEND E47_WEB_DATA ${E47_WEB_DATA_CROSS_COUNTRY})
else()
  list(REMOVE_ITEM E47_WEB_DATA ${E47_WEB_DATA_CROSS_COUNTRY})
endif()
list(REMOVE_DUPLICATES E47_WEB_DATA)
list(SORT E47_WEB_DATA)

# A path piece in the code that matches no packaged file is probably a new input:
# add the full path to E47_WEB_DATA_ASSEMBLED above.
list(REMOVE_DUPLICATES _e47_partial)
foreach(_p IN LISTS _e47_partial)
  set(_hit FALSE)
  foreach(_d IN LISTS E47_WEB_DATA)
    string(LENGTH "${_d}" _ld)
    string(LENGTH "${_p}" _lp)
    if(_ld GREATER_EQUAL _lp)
      math(EXPR _off "${_ld} - ${_lp}")
      string(SUBSTRING "${_d}" ${_off} -1 _tail)
      if(_tail STREQUAL _p)
        set(_hit TRUE)
        break()
      endif()
    endif()
  endforeach()
  if(NOT _hit)
    message(WARNING "web: \"${_p}\" in src/ matches no packaged data file; add it to E47_WEB_DATA_ASSEMBLED in cmake/Web.cmake")
  endif()
endforeach()

# Stage the package in the build tree (no spaces in the path, unlike the repo).
set(E47_WEB_FS "${CMAKE_BINARY_DIR}/web_fs")
set(_e47_staged "")
foreach(_d IN LISTS E47_WEB_DATA)
  if(NOT EXISTS "${PROJECT_SOURCE_DIR}/${_d}")
    message(WARNING "web: ${_d} is read by the pipeline but does not exist; not packaged")
    continue()
  endif()
  add_custom_command(OUTPUT "${E47_WEB_FS}/${_d}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PROJECT_SOURCE_DIR}/${_d}" "${E47_WEB_FS}/${_d}"
    DEPENDS "${PROJECT_SOURCE_DIR}/${_d}"
    VERBATIM)
  list(APPEND _e47_staged "${E47_WEB_FS}/${_d}")
endforeach()
# Dashboard font: Roboto (Apache 2.0) from the Dear ImGui sources.
set(_e47_font "${E47_WEB_FS}/fonts/Roboto-Medium.ttf")
add_custom_command(OUTPUT "${_e47_font}"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "${imgui_SOURCE_DIR}/misc/fonts/Roboto-Medium.ttf" "${_e47_font}"
  DEPENDS "${imgui_SOURCE_DIR}/misc/fonts/Roboto-Medium.ttf"
  VERBATIM)
list(APPEND _e47_staged "${_e47_font}")

# Remove staged files that are no longer part of the package.
file(GLOB_RECURSE _e47_old "${E47_WEB_FS}/*")
foreach(_o IN LISTS _e47_old)
  if(NOT _o IN_LIST _e47_staged)
    file(REMOVE "${_o}")
  endif()
endforeach()

list(LENGTH _e47_staged _n)
message(STATUS "web: packaging ${_n} files under /data and /fonts")
add_custom_target(e47_web_fs DEPENDS ${_e47_staged})

# --- Page ---------------------------------------------------------------------
configure_file("${PROJECT_SOURCE_DIR}/web/shell.html" "${CMAKE_BINARY_DIR}/web_shell.html" COPYONLY)

add_dependencies(e47_dashboard e47_web_fs)
set_target_properties(e47_dashboard PROPERTIES
  OUTPUT_NAME index
  SUFFIX ".html"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/web")
set_property(TARGET e47_dashboard APPEND PROPERTY LINK_DEPENDS "${CMAKE_BINARY_DIR}/web_shell.html" ${_e47_staged})
target_link_options(e47_dashboard PRIVATE
  -O3
  -sENVIRONMENT=web,worker
  -sALLOW_MEMORY_GROWTH=1
  # Growth with -pthread costs a check on JS heap access (GL calls). GROWABLE_ARRAYBUFFERS
  # would remove it, but WebGL rejects views on resizable buffers, so it stays off.
  -Wno-pthreads-mem-growth
  -sINITIAL_MEMORY=64MB
  # LZ4-compressed data package (CDNs do not compress application/octet-stream).
  -sLZ4=1
  # Pipeline structs (Monte Carlo inputs, results) are large; the default 64 KB stack is not enough.
  -sSTACK_SIZE=4MB
  # Workers for the dashboard's background jobs (data loading, scenario updates) and
  # the Monte Carlo fork-join (up to 8 workers, src/forecast/mc_cpu.cpp).
  -sPTHREAD_POOL_SIZE=10
  "SHELL:--shell-file ${CMAKE_BINARY_DIR}/web_shell.html"
  "SHELL:--preload-file ${E47_WEB_FS}@/")

# --- Static site ---------------------------------------------------------------
# Target e47_site stages the deployable site in <build>/site:
#   index.html    landing page: web/landing.html filled from outputs/site_summary.json
#                 (written by the native energy2047 run) by cmake/Site.cmake
#   summary.json  that summary, linked from the page
#   app/          the dashboard (index.html, .js, .wasm, .data)
#   robots.txt, vercel.json
# Not part of ALL: it needs outputs/site_summary.json, which only the native run writes.
set(E47_SITE "${CMAKE_BINARY_DIR}/site")
set(E47_SITE_SUMMARY "${PROJECT_SOURCE_DIR}/outputs/site_summary.json" CACHE FILEPATH
  "Model summary the landing page is rendered from (energy2047 writes it)")
add_custom_target(e47_site
  COMMAND ${CMAKE_COMMAND} -E rm -rf "${E47_SITE}"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${E47_SITE}/app"
  COMMAND ${CMAKE_COMMAND} "-DSUMMARY=${E47_SITE_SUMMARY}" "-DTEMPLATE=${PROJECT_SOURCE_DIR}/web/landing.html"
          "-DOUT=${E47_SITE}/index.html" -P "${PROJECT_SOURCE_DIR}/cmake/Site.cmake"
  COMMAND ${CMAKE_COMMAND} -E copy "${E47_SITE_SUMMARY}" "${E47_SITE}/summary.json"
  COMMAND ${CMAKE_COMMAND} -E copy
          "${CMAKE_BINARY_DIR}/web/index.html" "${CMAKE_BINARY_DIR}/web/index.js"
          "${CMAKE_BINARY_DIR}/web/index.wasm" "${CMAKE_BINARY_DIR}/web/index.data" "${E47_SITE}/app/"
  COMMAND ${CMAKE_COMMAND} -E copy
          "${PROJECT_SOURCE_DIR}/web/vercel.json" "${PROJECT_SOURCE_DIR}/web/robots.txt" "${E47_SITE}/"
  COMMENT "Staging the site (landing page + dashboard under app/) in ${E47_SITE}"
  VERBATIM)
add_dependencies(e47_site e47_dashboard)
