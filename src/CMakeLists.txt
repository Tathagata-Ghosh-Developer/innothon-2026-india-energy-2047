add_library(e47_core STATIC
  core/csv.cpp
  core/price_base.cpp
  forecast/panel.cpp
  forecast/models.cpp
  forecast/backtest.cpp
  forecast/india.cpp
  forecast/mc_cpu.cpp
  forecast/state_panel.cpp
  regional/convergence.cpp
  regional/states.cpp
  allocation/inputs.cpp
  allocation/allocate.cpp
  allocation/storage.cpp
  pipeline/demand_features.cpp
  pipeline/pipeline.cpp)
target_include_directories(e47_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(e47_core PUBLIC e47_flags e47_eigen highs)

if(ENABLE_CUDA)
  target_sources(e47_core PRIVATE forecast/mc_gpu.cu)
  set_target_properties(e47_core PROPERTIES CUDA_SEPARABLE_COMPILATION OFF)
  find_package(CUDAToolkit REQUIRED)
  target_link_libraries(e47_core PUBLIC CUDA::cudart_static)
else()
  target_sources(e47_core PRIVATE forecast/mc_gpu_stub.cpp)
endif()

# Browser build: the Monte Carlo on the visitor's GPU through WebGPU when the browser
# has it (forecast/mc_webgpu.cpp, Emscripten's emdawnwebgpu port), the CPU path otherwise.
# The WGSL kernel is embedded in a generated header at build time.
if(EMSCRIPTEN AND E47_WEBGPU)
  set(_wgsl "${CMAKE_CURRENT_SOURCE_DIR}/forecast/mc_webgpu.wgsl")
  set(_wgsl_h "${CMAKE_CURRENT_BINARY_DIR}/generated/mc_webgpu_wgsl.hpp")
  add_custom_command(OUTPUT "${_wgsl_h}"
    COMMAND ${CMAKE_COMMAND} "-DIN=${_wgsl}" "-DOUT=${_wgsl_h}" -DNAME=kMcWebGpuWgsl
            -P "${PROJECT_SOURCE_DIR}/cmake/EmbedText.cmake"
    DEPENDS "${_wgsl}" "${PROJECT_SOURCE_DIR}/cmake/EmbedText.cmake"
    COMMENT "Embedding forecast/mc_webgpu.wgsl"
    VERBATIM)
  target_sources(e47_core PRIVATE forecast/mc_webgpu.cpp "${_wgsl_h}")
  target_include_directories(e47_core PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
  target_compile_definitions(e47_core PUBLIC E47_WEBGPU=1)
  target_compile_options(e47_core PRIVATE --use-port=emdawnwebgpu)
  target_link_options(e47_core INTERFACE --use-port=emdawnwebgpu)
endif()
