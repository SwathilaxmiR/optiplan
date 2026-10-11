#pragma once

/// @file routing.hpp
/// @brief Dijkstra's algorithm and Yen's k-shortest loopless paths routing engine.

#include <cstdint>
#include <unordered_set>
#include <vector>

#include "optiplan/topology.hpp"

namespace optiplan {

/// Path traversed through the optical network
struct Path {
    std::vector<int> nodes;     ///< Sequence of nodes from source to destination
    std::vector<int> link_ids;  ///< Sequence of physical link IDs
    double length_km = 0.0;     ///< Total physical path length [km]

    int hops() const { return static_cast<int>(link_ids.size()); }
    bool empty() const { return nodes.empty(); }

    bool operator==(const Path& other) const {
        return nodes == other.nodes && link_ids == other.link_ids;
    }
    bool operator!=(const Path& other) const { return !(*this == other); }
};

/// Dijkstra's algorithm with node and edge exclusions (used as spur-path solver in Yen's KSP).
Path dijkstraShortestPath(const Topology& topo, int src, int dst,
                          const std::unordered_set<int>& excluded_nodes = {},
                          const std::unordered_set<int64_t>& excluded_directed_edges = {});

/// Yen's algorithm for finding the loopless k-shortest paths between src and dst.
std::vector<Path> yenKShortestPaths(const Topology& topo, int src, int dst, int k);

/// Cache holding precomputed k-shortest paths for all (source, destination) pairs.
class KShortestPathCache {
public:
    KShortestPathCache() = default;

    /// Precompute and cache k-shortest paths for all pairs in topology
    explicit KShortestPathCache(const Topology& topo, int k = 5);

    /// Query precomputed k-shortest paths between src and dst
    const std::vector<Path>& getKShortestPaths(int src, int dst) const;

    /// Get configured k
    int k() const { return k_; }

private:
    int num_nodes_ = 0;
    int k_ = 5;
    // Indexed as src * num_nodes + dst
    std::vector<std::vector<Path>> cache_;
};

}  // namespace optiplan
