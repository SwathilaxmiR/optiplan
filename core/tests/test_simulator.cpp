/// @file test_simulator.cpp
/// @brief Integration tests for discrete event-driven dynamic network simulation.

#include <gtest/gtest.h>

#include "optiplan/planner.hpp"
#include "optiplan/simulator.hpp"
#include "optiplan/topology.hpp"
#include "optiplan/traffic.hpp"

using namespace optiplan;

static std::string getTopoPath(const std::string& name) {
    return std::string(OPTIPLAN_TEST_DATA_DIR) + "/" + name + "/topology.json";
}

TEST(SimulatorTest, DynamicSimulationRun) {
    auto topo = Topology::loadFromFile(getTopoPath("nsfnet"));
    auto planner = std::make_shared<QoTAwarePlanner>();
    Config cfg;

    Simulator sim(topo, planner, cfg, 3);

    TrafficGenerator gen(topo.numNodes(), 100);
    auto requests = gen.generateRequests(200, 25.0, 1.0, 100.0);

    auto metrics = sim.run(requests);

    EXPECT_EQ(metrics.total_requests, 200);
    EXPECT_EQ(metrics.accepted_requests + metrics.blocked_requests, 200);
    EXPECT_GE(metrics.request_blocking_probability, 0.0);
    EXPECT_LE(metrics.request_blocking_probability, 1.0);
    EXPECT_GT(metrics.wall_clock_time_ms, 0.0);

    // After all requests depart, spectrum should be completely free
    EXPECT_DOUBLE_EQ(sim.spectrum().getAverageUtilization(), 0.0);
    EXPECT_TRUE(sim.activeLightpaths().empty());
}

TEST(SimulatorTest, RQ1BlockingComparison) {
    auto topo = Topology::loadFromFile(getTopoPath("nsfnet"));
    Config cfg;

    auto qot_planner = std::make_shared<QoTAwarePlanner>();
    auto fixed_planner = std::make_shared<FixedMarginPlanner>();

    Simulator sim_qot(topo, qot_planner, cfg, 3);
    Simulator sim_fixed(topo, fixed_planner, cfg, 3);

    // Identical traffic sequence for both planners (fair comparison)
    TrafficGenerator gen(topo.numNodes(), 42);
    auto requests = gen.generateRequests(300, 30.0, 1.0, 100.0);

    auto metrics_qot = sim_qot.run(requests);
    auto metrics_fixed = sim_fixed.run(requests);

    // RQ1: QoT-aware adaptive planner should accept more traffic (or equal) than fixed-margin
    EXPECT_GE(metrics_qot.accepted_requests, metrics_fixed.accepted_requests);
    EXPECT_LE(metrics_qot.request_blocking_probability, metrics_fixed.request_blocking_probability);
}
