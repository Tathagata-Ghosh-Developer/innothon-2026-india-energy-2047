add_library(e47_core STATIC
  core/csv.cpp
  forecast/panel.cpp
  forecast/models.cpp
  forecast/backtest.cpp
  forecast/india.cpp
  forecast/mc_cpu.cpp
  regional/convergence.cpp
  regional/states.cpp
  allocation/inputs.cpp
  allocation/allocate.cpp
  allocation/storage.cpp
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
