# Landing-page template (cmake/Site.cmake): renders small templates from a test summary
# and compares the output with the expected text: number formatting, then the
# conditional blocks (a missing key in the part not kept must not stop the build).
# Usage: cmake -DSRC=<repo root> -DWORK=<scratch dir> -P tests/site_format.cmake
cmake_minimum_required(VERSION 3.26)
file(MAKE_DIRECTORY "${WORK}")
file(WRITE "${WORK}/summary.json" [=[
{"schema": 1, "a": 0.1038, "b": 0.0653, "c": 12.0049, "d": 1234.5, "e": -0.0004, "f": 0.0099, "g": 100.07, "h": 1234567.8, "i": 0.0105,
 "n": null, "o": {"k": "x"}, "arr": [1, 2, 3]}
]=])

# _render(<template text> <expected output>)
function(_render template want)
  file(WRITE "${WORK}/template.html" "${template}")
  execute_process(
    COMMAND "${CMAKE_COMMAND}" -DSUMMARY=${WORK}/summary.json -DTEMPLATE=${WORK}/template.html
            -DOUT=${WORK}/out.html -P "${SRC}/cmake/Site.cmake"
    RESULT_VARIABLE rc OUTPUT_QUIET ERROR_VARIABLE err)
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR "Site.cmake failed: ${err}")
  endif()
  file(READ "${WORK}/out.html" got)
  string(STRIP "${got}" got)
  if(NOT got STREQUAL want)
    message(FATAL_ERROR "site template\n  got:  ${got}\n  want: ${want}")
  endif()
  message(STATUS "site template ok: ${got}")
endfunction()

_render("{{a|r3}};{{b|r3}};{{c|2}};{{d|0}};{{e|r1}};{{f|r2}};{{g|r1}};{{a|+1}};{{h|0}};{{i|r3}}"
        "0.104;0.065;12.00;1,235;0.0;0.01;100.1;+0.1;1,234,568;0.011")

# Kept part, fallback for a missing key (whose values are never read), null counts as
# missing, nested keys, no else part, and each blocks inside a kept part.
_render("<!--if a-->A{{a|r1}}<!--else-->noA<!--/if-->;<!--if zz.y-->{{zz.y}}<!--else-->noZ<!--/if-->;<!--if n-->N<!--else-->noN<!--/if-->;<!--if o.k-->{{o.k}}<!--/if-->;<!--if o.q-->Q<!--/if-->;<!--if arr--><!--each arr-->{{@}}<!--/each--><!--else-->none<!--/if-->"
        "A0.1;noZ;noN;x;;123")
# Nested blocks, in either part of the outer block.
_render("<!--if o-->[<!--if o.k-->k<!--else-->-<!--/if--><!--if o.q-->{{o.q}}<!--else-->q<!--/if-->]<!--else--><!--if a-->{{zz}}<!--/if--><!--/if-->;<!--if zz--><!--if a-->{{zz}}<!--/if--><!--else-->[<!--if a-->a<!--/if-->]<!--/if-->"
        "[kq];[a]")
