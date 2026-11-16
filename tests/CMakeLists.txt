add_executable(test_mc_parity test_mc_parity.cpp)
target_link_libraries(test_mc_parity PRIVATE e47_core)
add_test(NAME mc_parity COMMAND test_mc_parity)

add_executable(test_allocation test_allocation.cpp)
target_link_libraries(test_allocation PRIVATE e47_core)
add_test(NAME allocation COMMAND test_allocation ${PROJECT_SOURCE_DIR})

add_executable(test_storage test_storage.cpp)
target_link_libraries(test_storage PRIVATE e47_core)
add_test(NAME storage COMMAND test_storage ${PROJECT_SOURCE_DIR})

add_executable(test_pareto test_pareto.cpp)
target_link_libraries(test_pareto PRIVATE e47_core)
add_test(NAME pareto COMMAND test_pareto ${PROJECT_SOURCE_DIR})

add_executable(test_india test_india.cpp)
target_link_libraries(test_india PRIVATE e47_core)
add_test(NAME india COMMAND test_india ${PROJECT_SOURCE_DIR})

# Landing-page number formatting (CMake script, no compiler needed).
add_test(NAME site_format
         COMMAND ${CMAKE_COMMAND} -DSRC=${PROJECT_SOURCE_DIR} -DWORK=${CMAKE_CURRENT_BINARY_DIR}/site_format
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/site_format.cmake)
