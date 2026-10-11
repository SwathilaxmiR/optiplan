cmake_minimum_required(VERSION 3.22)
project(optiplan
    VERSION 0.1.0
    DESCRIPTION "QoT-Aware Optical Network Planner"
    LANGUAGES CXX
)

# ─── C++ Standard ────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ─── Compiler warnings (applied globally) ────────────────────────────────────
add_compile_options(-Wall -Wextra -Wpedantic -Werror)

# ─── Export compile_commands.json for IDE / clang-tidy ────────────────────────
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ─── Options ──────────────────────────────────────────────────────────────────
option(OPTIPLAN_BUILD_TESTS "Build unit tests" ON)
option(OPTIPLAN_BUILD_BENCH "Build benchmarks" OFF)

# ─── Dependencies via FetchContent ───────────────────────────────────────────
include(FetchContent)

# GoogleTest
if(OPTIPLAN_BUILD_TESTS)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
    )
    # Prevent GoogleTest from overriding our compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

# nlohmann/json
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(nlohmann_json)

# ─── Testing ─────────────────────────────────────────────────────────────────
if(OPTIPLAN_BUILD_TESTS)
    enable_testing()
endif()

# ─── Subdirectories ──────────────────────────────────────────────────────────
add_subdirectory(core)
