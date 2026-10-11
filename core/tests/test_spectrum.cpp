/// @file test_spectrum.cpp
/// @brief Unit tests for bitset spectrum management, continuity, non-overlap, and policies.

#include <gtest/gtest.h>

#include "optiplan/spectrum.hpp"

using namespace optiplan;

TEST(SpectrumTest, InvariantsContinuityAndNonOverlap) {
    // 4 links, 80 channels
    SpectrumState state(4, 80);

    Path path_a;  // Traverses links 0, 1, 2
    path_a.link_ids = {0, 1, 2};

    Path path_b;  // Traverses links 1, 3 (shares link 1 with path_a)
    path_b.link_ids = {1, 3};

    Path path_c;  // Traverses links 0, 3 (disjoint from link 1)
    path_c.link_ids = {0, 3};

    // Initially all 80 slots free
    EXPECT_EQ(state.findAvailableSlots(path_a).count(), 80u);
    EXPECT_EQ(state.findAvailableSlots(path_b).count(), 80u);

    // Allocate slot 7 on path_a
    state.allocateSlot(path_a, 7);

    // Continuity verified: slot 7 is occupied on all links of path_a (0, 1, 2)
    EXPECT_TRUE(state.getOccupiedSlots(0).test(7));
    EXPECT_TRUE(state.getOccupiedSlots(1).test(7));
    EXPECT_TRUE(state.getOccupiedSlots(2).test(7));
    EXPECT_FALSE(state.getOccupiedSlots(3).test(7));  // Link 3 is free on slot 7

    // Non-overlap constraint: path_b shares link 1, so slot 7 is NOT available for path_b
    EXPECT_FALSE(state.isSlotAvailable(path_b, 7));
    EXPECT_THROW(state.allocateSlot(path_b, 7), std::runtime_error);

    // But path_c (links 0, 3) shares link 0, so slot 7 is also unavailable for path_c
    EXPECT_FALSE(state.isSlotAvailable(path_c, 7));

    // Allocate slot 8 on path_b (links 1, 3)
    EXPECT_TRUE(state.isSlotAvailable(path_b, 8));
    state.allocateSlot(path_b, 8);
    EXPECT_TRUE(state.getOccupiedSlots(1).test(8));
    EXPECT_TRUE(state.getOccupiedSlots(3).test(8));

    // Reversibility: release slot 7 from path_a
    state.releaseSlot(path_a, 7);
    EXPECT_FALSE(state.getOccupiedSlots(0).test(7));
    EXPECT_FALSE(state.getOccupiedSlots(1).test(7));
    EXPECT_FALSE(state.getOccupiedSlots(2).test(7));

    // Now slot 7 is available again for path_b
    EXPECT_TRUE(state.isSlotAvailable(path_b, 7));

    // Double release should throw
    EXPECT_THROW(state.releaseSlot(path_a, 7), std::runtime_error);
}

TEST(SpectrumTest, UtilizationMetrics) {
    SpectrumState state(2, 80);
    Path p0;
    p0.link_ids = {0};
    Path p1;
    p1.link_ids = {1};

    EXPECT_DOUBLE_EQ(state.getLinkUtilization(0), 0.0);
    EXPECT_DOUBLE_EQ(state.getAverageUtilization(), 0.0);

    // Allocate 20 slots on link 0
    for (int s = 0; s < 20; ++s) {
        state.allocateSlot(p0, s);
    }
    EXPECT_EQ(state.getOccupiedChannelCount(0), 20);
    EXPECT_DOUBLE_EQ(state.getLinkUtilization(0), 20.0 / 80.0);
    EXPECT_DOUBLE_EQ(state.getAverageUtilization(), 10.0 / 80.0);  // (20 + 0) / 160
}

TEST(SpectrumTest, PolicyFirstFit) {
    SpectrumState state(2, 80);
    Path p;
    p.link_ids = {0, 1};

    // Pre-occupy slots 0, 1, 2
    state.allocateSlot(p, 0);
    state.allocateSlot(p, 1);
    state.allocateSlot(p, 2);

    FirstFitPolicy policy;
    SlotMask avail = state.findAvailableSlots(p);
    int chosen = policy.selectSlot(p, avail, state);

    // First-fit must pick slot 3
    EXPECT_EQ(chosen, 3);
}

TEST(SpectrumTest, PolicyMostUsed) {
    SpectrumState state(10, 80);
    Path test_path;
    test_path.link_ids = {0, 1};

    // Make slot 15 used on 5 different links (links 2, 3, 4, 5, 6)
    for (int l = 2; l <= 6; ++l) {
        Path single_link;
        single_link.link_ids = {l};
        state.allocateSlot(single_link, 15);
    }

    // Make slot 5 used on only 2 links (links 2, 3)
    for (int l = 2; l <= 3; ++l) {
        Path single_link;
        single_link.link_ids = {l};
        state.allocateSlot(single_link, 5);
    }

    MostUsedPolicy policy;
    SlotMask avail = state.findAvailableSlots(test_path);
    int chosen = policy.selectSlot(test_path, avail, state);

    // Most-used should prefer slot 15 (global usage 5) over slot 5 (usage 2) or slot 0 (usage 0)
    EXPECT_EQ(chosen, 15);
}

TEST(SpectrumTest, PolicyRandomFitReproducibility) {
    SpectrumState state(2, 80);
    Path p;
    p.link_ids = {0};

    SlotMask avail = state.findAvailableSlots(p);

    RandomFitPolicy policy_a(12345);
    RandomFitPolicy policy_b(12345);

    for (int iter = 0; iter < 10; ++iter) {
        EXPECT_EQ(policy_a.selectSlot(p, avail, state), policy_b.selectSlot(p, avail, state));
    }
}

TEST(SpectrumTest, FullOccupancyExhaustion) {
    SpectrumState state(1, 80);
    Path p;
    p.link_ids = {0};

    for (int s = 0; s < 80; ++s) {
        state.allocateSlot(p, s);
    }

    EXPECT_EQ(state.getOccupiedChannelCount(0), 80);
    EXPECT_DOUBLE_EQ(state.getLinkUtilization(0), 1.0);

    SlotMask avail = state.findAvailableSlots(p);
    EXPECT_TRUE(avail.none());

    FirstFitPolicy ff;
    MostUsedPolicy mu;
    RandomFitPolicy rf;

    EXPECT_EQ(ff.selectSlot(p, avail, state), -1);
    EXPECT_EQ(mu.selectSlot(p, avail, state), -1);
    EXPECT_EQ(rf.selectSlot(p, avail, state), -1);
}
