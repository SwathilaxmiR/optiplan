# ─── OptiPlan Tests ──────────────────────────────────────────────────────────
add_executable(optiplan_tests
    test_smoke.cpp
    test_config.cpp
)

target_link_libraries(optiplan_tests
    PRIVATE
        optiplan_core
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(optiplan_tests)
