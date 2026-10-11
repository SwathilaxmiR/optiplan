# ─── OptiPlan Tests ──────────────────────────────────────────────────────────
add_executable(optiplan_tests
    test_smoke.cpp
    test_config.cpp
    test_modulation.cpp
    test_gn_model.cpp
    test_topology.cpp
    test_routing.cpp
    test_spectrum.cpp
    test_planner.cpp
    test_traffic.cpp
    test_simulator.cpp
    test_thread_pool.cpp
)

target_link_libraries(optiplan_tests
    PRIVATE
        optiplan_core
        GTest::gtest_main
)

target_compile_definitions(optiplan_tests
    PRIVATE
        OPTIPLAN_TEST_DATA_DIR="${CMAKE_SOURCE_DIR}/data/topologies"
)

include(GoogleTest)
gtest_discover_tests(optiplan_tests)
