add_executable(energy2047 energy2047.cpp)
target_link_libraries(energy2047 PRIVATE e47_core)

if(BUILD_DASHBOARD)
  add_executable(e47_dashboard dashboard/main.cpp dashboard/app.cpp)
  target_include_directories(e47_dashboard PRIVATE dashboard)
  target_link_libraries(e47_dashboard PRIVATE e47_core e47_imgui)
  if(EMSCRIPTEN AND E47_WEBGPU)
    # In-browser parity check of the WebGPU kernel against the CPU float path (/app/?selftest=1).
    target_sources(e47_dashboard PRIVATE dashboard/webgpu_selftest.cpp)
  endif()
endif()
