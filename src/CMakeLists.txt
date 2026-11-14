add_library(e47_core STATIC
  core/csv.cpp
  forecast/panel.cpp
  forecast/models.cpp
  forecast/backtest.cpp)
target_include_directories(e47_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(e47_core PUBLIC e47_flags e47_eigen)
