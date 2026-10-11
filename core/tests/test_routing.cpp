/// @file test_routing.cpp
/// @brief Unit tests for Dijkstra's algorithm, Yen's k-shortest paths, and KShortestPathCache.

#include <gtest/gtest.h>

#include <unordered_set>

#include "optiplan/routing.hpp"
#include "optiplan/topology.hpp"

using namespace optiplan;

static std::string getTopoPath(const std::string& name) {
    return std::string(OPTIPLAN_TEST_DATA_DIR) + "/" + name + "/topology.json";
}

TEST(RoutingTest, DijkstraOnToy4) {
    auto topo = Topology::loadFromFile(getTopoPath("toy4"));

    // Shortest path A(0) -> D(3)
    // Option 1: A-B-D = 100 + 200 = 300 km
    // Option 2: A-C-D = 150 + 250 = 400 km
    Path p = dijkstraShortestPath(topo, 0, 3);

    ASSERT_FALSE(p.empty());
    EXPECT_DOUBLE_EQ(p.length_km, 300.0);
    EXPECT_EQ(p.hops(), 2);

    std::vector<int> expected_nodes = {0, 1, 3};
    std::vector<int> expected_links = {0, 2};
    EXPECT_EQ(p.nodes, expected_nodes);
    EXPECT_EQ(p.link_ids, expected_links);

    // Shortest path C(2) -> B(1)
    // Option 1: C-A-B = 150 + 100 = 250 km
    // Option 2: C-D-B = 250 + 200 = 450 km
    Path p_cb = dijkstraShortestPath(topo, 2, 1);
    ASSERT_FALSE(p_cb.empty());
    EXPECT_DOUBLE_EQ(p_cb.length_km, 250.0);
    std::vector<int> expected_cb_nodes = {2, 0, 1};
    EXPECT_EQ(p_cb.nodes, expected_cb_nodes);
}

TEST(RoutingTest, YenKspOnToy4) {
    auto topo = Topology::loadFromFile(getTopoPath("toy4"));

    // Request k=5 paths between 0 and 3
    // Exactly 2 loopless paths exist in this diamond topology
    auto paths = yenKShortestPaths(topo, 0, 3, 5);

    ASSERT_EQ(paths.size(), 2u);

    // 1st shortest: A-B-D (300 km)
    EXPECT_DOUBLE_EQ(paths[0].length_km, 300.0);
    EXPECT_EQ(paths[0].nodes, (std::vector<int>{0, 1, 3}));

    // 2nd shortest: A-C-D (400 km)
    EXPECT_DOUBLE_EQ(paths[1].length_km, 400.0);
    EXPECT_EQ(paths[1].nodes, (std::vector<int>{0, 2, 3}));
}

TEST(RoutingTest, YenKspPropertiesOnNsfnet) {
    auto topo = Topology::loadFromFile(getTopoPath("nsfnet"));

    // Compute k=5 shortest paths from Seattle (0) to DC (13)
    const int k = 5;
    auto paths = yenKShortestPaths(topo, 0, 13, k);

    ASSERT_EQ(paths.size(), static_cast<size_t>(k));

    // Property 1: Monotonically non-decreasing lengths
    for (size_t i = 1; i < paths.size(); ++i) {
        EXPECT_LE(paths[i - 1].length_km, paths[i].length_km);
    }

    // Property 2: Looplessness (no node appears more than once in any path)
    for (const auto& path : paths) {
        std::unordered_set<int> visited;
        for (int node : path.nodes) {
            EXPECT_TRUE(visited.insert(node).second) << "Loop detected in path at node " << node;
        }
    }

    // Property 3: Uniqueness (no two paths have the same node sequence)
    for (size_t i = 0; i < paths.size(); ++i) {
        for (size_t j = i + 1; j < paths.size(); ++j) {
            EXPECT_NE(paths[i].nodes, paths[j].nodes);
        }
    }

    // Property 4: All links along path exist and connect adjacent nodes
    for (const auto& path : paths) {
        ASSERT_EQ(path.nodes.size(), path.link_ids.size() + 1);
        double reconstructed_len = 0.0;
        for (size_t i = 0; i < path.link_ids.size(); ++i) {
            const auto& link = topo.getLink(path.link_ids[i]);
            int u = path.nodes[i];
            int v = path.nodes[i + 1];
            bool connects =
                (link.source == u && link.target == v) || (link.source == v && link.target == u);
            EXPECT_TRUE(connects);
            reconstructed_len += link.length_km;
        }
        EXPECT_NEAR(reconstructed_len, path.length_km, 1e-9);
    }
}

TEST(RoutingTest, KShortestPathCacheAllPairs) {
    auto topo = Topology::loadFromFile(getTopoPath("nsfnet"));
    KShortestPathCache cache(topo, 3);

    // Verify cache query matches direct Yen call
    for (int u = 0; u < 5; ++u) {
        for (int v = 0; v < 5; ++v) {
            if (u == v)
                continue;
            auto cached = cache.getKShortestPaths(u, v);
            auto direct = yenKShortestPaths(topo, u, v, 3);
            ASSERT_EQ(cached.size(), direct.size());
            for (size_t i = 0; i < cached.size(); ++i) {
                EXPECT_EQ(cached[i], direct[i]);
            }
        }
    }

    // Undirected symmetry of shortest path length
    for (int u = 0; u < topo.numNodes(); ++u) {
        for (int v = u + 1; v < topo.numNodes(); ++v) {
            const auto& p_uv = cache.getKShortestPaths(u, v);
            const auto& p_vu = cache.getKShortestPaths(v, u);
            ASSERT_FALSE(p_uv.empty());
            ASSERT_FALSE(p_vu.empty());
            EXPECT_NEAR(p_uv[0].length_km, p_vu[0].length_km, 1e-9);
        }
    }
}
