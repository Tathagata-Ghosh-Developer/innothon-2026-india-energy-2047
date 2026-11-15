# Compiler flags shared by every target through the e47_flags interface library.

add_library(e47_flags INTERFACE)

if(MSVC)
  target_compile_options(e47_flags INTERFACE
    $<$<COMPILE_LANGUAGE:CXX>:/W4 /permissive- /utf-8 /Zc:__cplusplus /EHsc /bigobj>
    # Missing values are NaN, so keep IEEE semantics (no /fp:fast) on the host.
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CONFIG:Release>>:/O2 /Ob3 /Oi /GL /arch:AVX2>
    # LLVM OpenMP runtime: OpenMP 3.1+ features (simd, collapse, unsigned loops)
    $<$<COMPILE_LANGUAGE:CXX>:/openmp:llvm>
  )
  target_link_options(e47_flags INTERFACE $<$<CONFIG:Release>:/LTCG>)
  target_compile_definitions(e47_flags INTERFACE NOMINMAX _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)
elseif(EMSCRIPTEN)
  # WebAssembly. Every object (including HiGHS) must share the thread, SIMD and
  # exception settings, so they go into the global flags, not only e47_flags.
  # -pthread: the dashboard computes on a worker thread (needs COOP/COEP headers).
  # No OpenMP runtime exists for wasm: OpenMP pragmas are ignored and loops run serially.
  string(APPEND CMAKE_C_FLAGS " -pthread -msimd128")
  string(APPEND CMAKE_CXX_FLAGS " -pthread -msimd128 -fwasm-exceptions")
  string(APPEND CMAKE_EXE_LINKER_FLAGS " -pthread -fwasm-exceptions")
  target_compile_options(e47_flags INTERFACE
    $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -Wpedantic -Wno-unknown-pragmas>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CONFIG:Release>>:-O3 -fno-math-errno>
  )
  # E47_WEB selects smaller defaults (Monte Carlo draws, bootstrap replicates).
  target_compile_definitions(e47_flags INTERFACE E47_WEB=1)
else()
  find_package(OpenMP REQUIRED)
  target_link_libraries(e47_flags INTERFACE OpenMP::OpenMP_CXX)
  target_compile_options(e47_flags INTERFACE
    $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -Wpedantic>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CONFIG:Release>>:-O3 -march=native -fno-math-errno>
  )
endif()

if(ENABLE_CUDA)
  target_compile_definitions(e47_flags INTERFACE E47_HAVE_CUDA=1)
  target_compile_options(e47_flags INTERFACE
    $<$<COMPILE_LANGUAGE:CUDA>:--use_fast_math --extended-lambda --expt-relaxed-constexpr -lineinfo>
  )
  if(MSVC)
    target_compile_options(e47_flags INTERFACE
      $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=/utf-8,/Zc:__cplusplus,/EHsc,/openmp:llvm>)
  endif()
endif()
