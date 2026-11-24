# Embeds a text file in a C++ header as a raw string literal (used for the WebGPU
# shader, src/forecast/mc_webgpu.wgsl).
# Usage: cmake -DIN=<file> -DOUT=<header> -DNAME=<identifier> -P EmbedText.cmake
file(READ "${IN}" _text)
if(_text MATCHES "\\)e47embed\"")
  message(FATAL_ERROR "${IN} contains the raw-string delimiter )e47embed\"")
endif()
get_filename_component(_name "${IN}" NAME)
file(WRITE "${OUT}.tmp"
  "// Generated from ${_name} by cmake/EmbedText.cmake at build time; do not edit.\n"
  "#pragma once\n"
  "inline constexpr char ${NAME}[] = R\"e47embed(${_text})e47embed\";\n")
file(COPY_FILE "${OUT}.tmp" "${OUT}" ONLY_IF_DIFFERENT)
file(REMOVE "${OUT}.tmp")
