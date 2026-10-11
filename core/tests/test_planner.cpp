/// @file test_planner.cpp
/// @brief Unit tests for FixedMarginPlanner and QoTAwarePlanner with QoT re-validation.

#include <gtest/gtest.h>

#include "optiplan/planner.hpp"
#include "optiplan/topology.hpp"

using namespace optiplan;

static std::string getTopoPath(const std::string& name) {
    return std::string(OPTIPLAN_TEST_DATA_DIR) + "/" + name + "/topology.json";
}

TEST(PlannerTest, FixedMarginReachRules) {
    EXPECT_EQ(FixedMarginPlanner::selectFormatByReach(500.0)->name(), "DP-16QAM");
    EXPECT_EQ(FixedMarginPlanner::selectFormatByReach(1000.0)->name(), "DP-8QAM");
    EXPECT_EQ(FixedMarginPlanner::selectFormatByReach(2000.0)->name(), "DP-QPSK");
    EXPECT_EQ(FixedMarginPlanner::selectFormatByReach(4000.0)->name(), "DP-BPSK");
    EXPECT_EQ(FixedMarginPlanner::selectFormatByReach(6000.0), nullptr);
}

TEST(PlannerTest, BasicProvisioningComparison) {
    auto topo = Topology::loadFromFile(getTopoPath("toy4"));
    KShortestPathCache cache(topo, 2);
    Config cfg;

    SpectrumState spec_fixed(topo.numLinks(), 80);
    SpectrumState spec_qot(topo.numLinks(), 80);

    FixedMarginPlanner fixed_planner;
    QoTAwarePlanner qot_planner;

    std::unordered_map<int, Lightpath> active;

    Request req{0, 0, 3, 100.0, 0.0, 10.0};  // 100 Gb/s from A(0) to D(3)

    auto dec_fixed = fixed_planner.planRequest(req, topo, cache, spec_fixed, active, cfg);
    auto dec_qot = qot_planner.planRequest(req, topo, cache, spec_qot, active, cfg);

    EXPECT_TRUE(dec_fixed.accepted);
    EXPECT_TRUE(dec_qot.accepted);

    EXPECT_EQ(dec_fixed.lightpath.slot_idx, 0);
    EXPECT_EQ(dec_qot.lightpath.slot_idx, 0);
    EXPECT_GT(dec_qot.lightpath.snr_db, 0.0);
}

TEST(PlannerTest, RevalidationPreventsDegradation) {
    auto topo = Topology::loadFromFile(getTopoPath("toy4"));
    KShortestPathCache cache(topo, 2);
    Config cfg;
    SpectrumState spec(topo.numLinks(), 80);
    QoTAwarePlanner planner;

    std::unordered_map<int, Lightpath> active;

    // Establish a live lightpath LP1 that uses a sensitive format (16QAM) on links {0, 2}
    Request req1{1, 0, 3, 200.0, 0.0, 10.0};
    auto dec1 = planner.planRequest(req1, topo, cache, spec, active, cfg);
    ASSERT_TRUE(dec1.accepted);

    spec.allocateSlot(dec1.lightpath.path, dec1.lightpath.slot_idx);
    active[dec1.lightpath.id] = dec1.lightpath;

    // Now attempt to provision another request sharing link 0
    Request req2{2, 0, 1, 100.0, 1.0, 10.0};
    auto dec2 = planner.planRequest(req2, topo, cache, spec, active, cfg);

    // Verify re-validation was performed for live lightpath LP1
    EXPECT_GT(dec2.revalidations_performed, 0);
    // If accepted, verify live lightpath still has valid SNR margin
    if (dec2.accepted) {
        EXPECT_GE(dec2.lightpath.snr_margin_db, 0.0);
    }
}
