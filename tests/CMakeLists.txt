add_executable(test_mc_parity test_mc_parity.cpp)
target_link_libraries(test_mc_parity PRIVATE e47_core)
add_test(NAME mc_parity COMMAND test_mc_parity)

add_executable(test_allocation test_allocation.cpp)
target_link_libraries(test_allocation PRIVATE e47_core)
add_test(NAME allocation COMMAND test_allocation ${PROJECT_SOURCE_DIR})
