# ─── OptiPlan Core Library ───────────────────────────────────────────────────
add_library(optiplan_core STATIC
    src/config.cpp
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

# ─── Tests ───────────────────────────────────────────────────────────────────
if(OPTIPLAN_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
