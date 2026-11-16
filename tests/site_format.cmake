# Landing-page number formatting (cmake/Site.cmake): renders a small template
# from a test summary and compares every formatted value with the expected text.
# Usage: cmake -DSRC=<repo root> -DWORK=<scratch dir> -P tests/site_format.cmake
cmake_minimum_required(VERSION 3.26)
file(MAKE_DIRECTORY "${WORK}")
file(WRITE "${WORK}/summary.json" [=[
{"schema": 1, "a": 0.1038, "b": 0.0653, "c": 12.0049, "d": 1234.5, "e": -0.0004, "f": 0.0099, "g": 100.07, "h": 1234567.8, "i": 0.0105}
]=])
file(WRITE "${WORK}/template.html" "{{a|r3}};{{b|r3}};{{c|2}};{{d|0}};{{e|r1}};{{f|r2}};{{g|r1}};{{a|+1}};{{h|0}};{{i|r3}}")
execute_process(
  COMMAND "${CMAKE_COMMAND}" -DSUMMARY=${WORK}/summary.json -DTEMPLATE=${WORK}/template.html
          -DOUT=${WORK}/out.html -P "${SRC}/cmake/Site.cmake"
  RESULT_VARIABLE rc OUTPUT_QUIET ERROR_VARIABLE err)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "Site.cmake failed: ${err}")
endif()
file(READ "${WORK}/out.html" got)
string(STRIP "${got}" got)
set(want "0.104;0.065;12.00;1,235;0.0;0.01;100.1;+0.1;1,234,568;0.011")
if(NOT got STREQUAL want)
  message(FATAL_ERROR "site formatting\n  got:  ${got}\n  want: ${want}")
endif()
message(STATUS "site formatting ok: ${got}")
