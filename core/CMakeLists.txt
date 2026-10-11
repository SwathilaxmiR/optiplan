# ─── OptiPlan Core Library ───────────────────────────────────────────────────
add_library(optiplan_core STATIC
    src/config.cpp
    src/physical_constants.cpp
    src/modulation.cpp
    src/gn_model.cpp
    src/topology.cpp
    src/routing.cpp
    src/spectrum.cpp
    src/traffic.cpp
    src/planner.cpp
    src/simulator.cpp
)

target_include_directories(optiplan_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(optiplan_core
    PUBLIC
        nlohmann_json::nlohmann_json
)

# ─── OptiPlan CLI / Demo Executable ──────────────────────────────────────────
add_executable(optiplan_cli
    src/main.cpp
)

target_link_libraries(optiplan_cli
    PRIVATE
        optiplan_core
)

# ─── Tests ───────────────────────────────────────────────────────────────────
if(OPTIPLAN_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
