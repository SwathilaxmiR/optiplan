/// @file test_topology.cpp
/// @brief Unit tests for Topology loading, structure, and graph validation.

#include <gtest/gtest.h>

#include "optiplan/topology.hpp"

using namespace optiplan;

static std::string getTopoPath(const std::string& name) {
    return std::string(OPTIPLAN_TEST_DATA_DIR) + "/" + name + "/topology.json";
}

TEST(TopologyTest, LoadToy4Topology) {
    auto topo = Topology::loadFromFile(getTopoPath("toy4"));

    EXPECT_EQ(topo.name(), "toy4");
    EXPECT_EQ(topo.numNodes(), 4);
    EXPECT_EQ(topo.numLinks(), 4);
    EXPECT_TRUE(topo.isConnected());

    // Check nodes
    EXPECT_EQ(topo.getNode(0).name, "A");
    EXPECT_EQ(topo.getNode(1).name, "B");
    EXPECT_EQ(topo.getNode(2).name, "C");
    EXPECT_EQ(topo.getNode(3).name, "D");

    // Check link lookups
    EXPECT_EQ(topo.findLinkId(0, 1), 0);
    EXPECT_EQ(topo.findLinkId(1, 0), 0);
    EXPECT_EQ(topo.findLinkId(0, 2), 1);
    EXPECT_EQ(topo.findLinkId(1, 3), 2);
    EXPECT_EQ(topo.findLinkId(2, 3), 3);
    EXPECT_EQ(topo.findLinkId(0, 3), -1);  // No direct link A-D

    // Check lengths
    EXPECT_DOUBLE_EQ(topo.getLink(0).length_km, 100.0);
    EXPECT_DOUBLE_EQ(topo.getLink(1).length_km, 150.0);
    EXPECT_DOUBLE_EQ(topo.getLink(2).length_km, 200.0);
    EXPECT_DOUBLE_EQ(topo.getLink(3).length_km, 250.0);
}

TEST(TopologyTest, LoadNsfnetTopology) {
    auto topo = Topology::loadFromFile(getTopoPath("nsfnet"));

    EXPECT_EQ(topo.name(), "nsfnet");
    EXPECT_EQ(topo.numNodes(), 14);
    EXPECT_EQ(topo.numLinks(), 21);
    EXPECT_TRUE(topo.isConnected());

    // Verify node 0 is Seattle (WA)
    EXPECT_EQ(topo.getNode(0).name, "WA");
    // Verify link 0 connects WA (0) and CA1 (1) with 1100 km
    const auto& l0 = topo.getLink(0);
    EXPECT_EQ(l0.source, 0);
    EXPECT_EQ(l0.target, 1);
    EXPECT_DOUBLE_EQ(l0.length_km, 1100.0);
}

TEST(TopologyTest, LoadNobelEuTopology) {
    auto topo = Topology::loadFromFile(getTopoPath("nobel_eu"));

    EXPECT_EQ(topo.name(), "nobel-eu");
    EXPECT_EQ(topo.numNodes(), 28);
    EXPECT_EQ(topo.numLinks(), 41);
    EXPECT_TRUE(topo.isConnected());

    // Verify degree of each node is at least 2
    for (int i = 0; i < topo.numNodes(); ++i) {
        EXPECT_GE(topo.getOutgoingEdges(i).size(), 2u);
    }
}

TEST(TopologyTest, InvalidTopologyExceptions) {
    // Missing nodes array
    nlohmann::json invalid_no_nodes = {{"links", nlohmann::json::array()}};
    EXPECT_THROW(Topology::loadFromJson(invalid_no_nodes), std::invalid_argument);

    // Negative link length
    nlohmann::json invalid_length = {
        {"nodes", {{{"id", 0}}, {{"id", 1}}}},
        {"links", {{{"id", 0}, {"source", 0}, {"target", 1}, {"length_km", -50.0}}}},
    };
    EXPECT_THROW(Topology::loadFromJson(invalid_length), std::invalid_argument);

    // Self loop
    nlohmann::json self_loop = {
        {"nodes", {{{"id", 0}}}},
        {"links", {{{"id", 0}, {"source", 0}, {"target", 0}, {"length_km", 100.0}}}},
    };
    EXPECT_THROW(Topology::loadFromJson(self_loop), std::invalid_argument);
}
