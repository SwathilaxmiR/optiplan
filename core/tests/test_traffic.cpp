/// @file test_traffic.cpp
/// @brief Unit tests for TrafficGenerator.

#include <gtest/gtest.h>

#include "optiplan/traffic.hpp"

using namespace optiplan;

TEST(TrafficTest, ReproducibilityWithSameSeed) {
    TrafficGenerator gen1(14, 12345);
    TrafficGenerator gen2(14, 12345);

    auto reqs1 = gen1.generateRequests(100, 20.0, 1.0, 100.0);
    auto reqs2 = gen2.generateRequests(100, 20.0, 1.0, 100.0);

    ASSERT_EQ(reqs1.size(), reqs2.size());
    for (size_t i = 0; i < reqs1.size(); ++i) {
        EXPECT_EQ(reqs1[i].src, reqs2[i].src);
        EXPECT_EQ(reqs1[i].dst, reqs2[i].dst);
        EXPECT_DOUBLE_EQ(reqs1[i].arrival_time, reqs2[i].arrival_time);
        EXPECT_DOUBLE_EQ(reqs1[i].holding_time, reqs2[i].holding_time);
    }
}

TEST(TrafficTest, ChronologicalArrivalTimesAndValidNodes) {
    TrafficGenerator gen(14, 42);
    auto reqs = gen.generateRequests(50, 15.0);

    ASSERT_EQ(reqs.size(), 50u);
    for (size_t i = 0; i < reqs.size(); ++i) {
        EXPECT_NE(reqs[i].src, reqs[i].dst);
        EXPECT_GE(reqs[i].src, 0);
        EXPECT_LT(reqs[i].src, 14);
        EXPECT_GE(reqs[i].dst, 0);
        EXPECT_LT(reqs[i].dst, 14);
        EXPECT_GT(reqs[i].holding_time, 0.0);

        if (i > 0) {
            EXPECT_GT(reqs[i].arrival_time, reqs[i - 1].arrival_time);
        }
    }
}
