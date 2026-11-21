# Landing-page template (cmake/Site.cmake): renders small templates from a test summary
# and compares the output with the expected text: number formatting, then the
# conditional blocks (a missing key in the part not kept must not stop the build), then
# the group parts of each blocks, then the malformed blocks that must stop the build.
# Usage: cmake -DSRC=<repo root> -DWORK=<scratch dir> -P tests/site_format.cmake
cmake_minimum_required(VERSION 3.26)
file(MAKE_DIRECTORY "${WORK}")
file(WRITE "${WORK}/summary.json" [=[
{"schema": 1, "a": 0.1038, "b": 0.0653, "c": 12.0049, "d": 1234.5, "e": -0.0004, "f": 0.0099, "g": 100.07, "h": 1234567.8, "i": 0.0105,
 "n": null, "o": {"k": "x"}, "arr": [1, 2, 3], "ea": [], "eo": {}, "es": "",
 "grp": [{"k": "a", "v": 1, "s": {"t": 1}}, {"k": "a", "v": 2, "s": {"t": 1}}, {"k": "b", "v": 3, "s": {"t": 2}}, {"k": "a", "v": 4, "s": {"t": 2}}]}
]=])

# _run(<template text>): renders it; sets rc, err and got in the caller.
macro(_run template)
  file(WRITE "${WORK}/template.html" "${template}")
  file(REMOVE "${WORK}/out.html")
  execute_process(
    COMMAND "${CMAKE_COMMAND}" -DSUMMARY=${WORK}/summary.json -DTEMPLATE=${WORK}/template.html
            -DOUT=${WORK}/out.html -P "${SRC}/cmake/Site.cmake"
    RESULT_VARIABLE rc OUTPUT_QUIET ERROR_VARIABLE err)
  set(got "")
  if(EXISTS "${WORK}/out.html")
    file(READ "${WORK}/out.html" got)
    string(STRIP "${got}" got)
  endif()
endmacro()

# _render(<template text> <expected output>)
function(_render template want)
  _run("${template}")
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR "Site.cmake failed: ${err}")
  endif()
  if(NOT got STREQUAL want)
    message(FATAL_ERROR "site template\n  got:  ${got}\n  want: ${want}")
  endif()
  message(STATUS "site template ok: ${got}")
endfunction()

# _render_fails(<template text> <regex the error must match>)
function(_render_fails template want)
  _run("${template}")
  if(rc EQUAL 0)
    message(FATAL_ERROR "site template should have failed: ${template}\n  got: ${got}")
  endif()
  if(NOT err MATCHES "${want}")
    message(FATAL_ERROR "site template failed with the wrong error: ${template}\n  error: ${err}\n  want:  ${want}")
  endif()
  message(STATUS "site template fails as it should: ${want}")
endfunction()

_render("{{a|r3}};{{b|r3}};{{c|2}};{{d|0}};{{e|r1}};{{f|r2}};{{g|r1}};{{a|+1}};{{h|0}};{{i|r3}}"
        "0.104;0.065;12.00;1,235;0.0;0.01;100.1;+0.1;1,234,568;0.011")

# Kept part, fallback for a missing key (whose values are never read), null counts as
# missing, nested keys, no else part, and each blocks inside a kept part.
_render("<!--if a-->A{{a|r1}}<!--else-->noA<!--/if-->;<!--if zz.y-->{{zz.y}}<!--else-->noZ<!--/if-->;<!--if n-->N<!--else-->noN<!--/if-->;<!--if o.k-->{{o.k}}<!--/if-->;<!--if o.q-->Q<!--/if-->;<!--if arr--><!--each arr-->{{@}}<!--/each--><!--else-->none<!--/if-->"
        "A0.1;noZ;noN;x;;123")
# An empty array, object or string counts as present.
_render("<!--if ea-->A<!--else-->-<!--/if-->;<!--if eo-->O<!--else-->-<!--/if-->;<!--if es-->S<!--else-->-<!--/if-->"
        "A;O;S")
# Nested blocks, in either part of the outer block.
_render("<!--if o-->[<!--if o.k-->k<!--else-->-<!--/if--><!--if o.q-->{{o.q}}<!--else-->q<!--/if-->]<!--else--><!--if a-->{{zz}}<!--/if--><!--/if-->;<!--if zz--><!--if a-->{{zz}}<!--/if--><!--else-->[<!--if a-->a<!--/if-->]<!--/if-->"
        "[kq];[a]")

# Group parts: kept for the first element and wherever the field changes from the element
# written out before (so a run of equal values gets one heading), in either order; nested
# field paths; several group parts in one block.
_render("<!--each grp--><!--group @.k-->[{{@.k}}]<!--/group-->{{@.v}}<!--/each-->"
        "[a]12[b]3[a]4")
_render("<!--each-reverse grp--><!--group @.k-->[{{@.k}}]<!--/group-->{{@.v}}<!--/each-->"
        "[a]4[b]3[a]21")
_render("<!--each grp--><!--group @.s.t-->(<!--/group-->{{@.v}}<!--group @.k-->/<!--/group--><!--/each-->"
        "(1/2(3/4/")

# Malformed blocks stop the build with a message that says what is wrong.
_render_fails("<!--if a-->x" "if block without <!--/if-->")
_render_fails("x<!--/if-->" "<!--/if--> without an opening <!--if")
_render_fails("<!--if a b-->x<!--/if-->" "malformed block tag")
_render_fails("x<!--else-->y" "<!--else--> outside an if block")
_render_fails("<!--if a-->1<!--else-->2<!--else-->3<!--/if-->" "has more than one <!--else-->")
_render_fails("<!--group @.k-->x<!--/group-->" "group part outside an each block")
_render_fails("<!--each grp--><!--group @.k-->x<!--/each-->" "has no <!--/group-->")
_render_fails("<!--each grp--><!--group k-->x<!--/group--><!--/each-->" "malformed group tag")
_render_fails("<!--each grp--><!--group @.zz-->x<!--/group--><!--/each-->" "<!--group @.zz-->")
_render_fails("<!--each grp-->x<!--/group--><!--/each-->" "<!--/group--> without an opening <!--group")
