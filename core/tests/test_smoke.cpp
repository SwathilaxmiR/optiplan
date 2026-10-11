/// @file test_smoke.cpp
/// @brief Trivial smoke test to verify the build pipeline works end-to-end.

#include <gtest/gtest.h>

TEST(SmokeTest, OnePlusOneIsTwo) {
    EXPECT_EQ(1 + 1, 2);
}

TEST(SmokeTest, ProjectCompiles) {
    // If this test runs, the C++17 toolchain and GoogleTest are working.
    constexpr int x = 42;
    EXPECT_EQ(x, 42);
}
