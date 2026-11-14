add_executable(test_mc_parity test_mc_parity.cpp)
target_link_libraries(test_mc_parity PRIVATE e47_core)
add_test(NAME mc_parity COMMAND test_mc_parity)
