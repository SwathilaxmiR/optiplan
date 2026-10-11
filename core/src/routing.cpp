/// @file routing.cpp
/// @brief Implementation of Dijkstra's algorithm and Yen's k-shortest loopless paths.

#include "optiplan/routing.hpp"

#include <algorithm>
#include <limits>
#include <queue>
#include <stdexcept>

namespace optiplan {

namespace {

inline int64_t makeDirectedEdgeKey(int u, int v) {
    return (static_cast<int64_t>(u) << 32) | static_cast<uint32_t>(v);
}

}  // namespace

Path dijkstraShortestPath(const Topology& topo, int src, int dst,
                          const std::unordered_set<int>& excluded_nodes,
                          const std::unordered_set<int64_t>& excluded_directed_edges) {
    if (src == dst || src < 0 || src >= topo.numNodes() || dst < 0 || dst >= topo.numNodes()) {
        return {};
    }
    if (excluded_nodes.count(src) || excluded_nodes.count(dst)) {
        return {};
    }

    const int n = topo.numNodes();
    const double inf = std::numeric_limits<double>::infinity();

    std::vector<double> dist(n, inf);
    std::vector<int> prev_node(n, -1);
    std::vector<int> prev_link(n, -1);

    // Min-heap storing pair: (distance, node)
    using Element = std::pair<double, int>;
    std::priority_queue<Element, std::vector<Element>, std::greater<Element>> pq;

    dist[src] = 0.0;
    pq.push({0.0, src});

    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();

        if (d > dist[u])
            continue;
        if (u == dst)
            break;

        for (const auto& edge : topo.getOutgoingEdges(u)) {
            int v = edge.to;
            if (excluded_nodes.count(v))
                continue;
            if (excluded_directed_edges.count(makeDirectedEdgeKey(u, v)))
                continue;

            double new_dist = d + edge.length_km;
            if (new_dist < dist[v]) {
                dist[v] = new_dist;
                prev_node[v] = u;
                prev_link[v] = edge.link_id;
                pq.push({new_dist, v});
            }
        }
    }

    if (dist[dst] == inf) {
        return {};  // No path exists
    }

    // Reconstruct path
    Path path;
    path.length_km = dist[dst];
    for (int curr = dst; curr != -1; curr = prev_node[curr]) {
        path.nodes.push_back(curr);
        if (prev_link[curr] != -1) {
            path.link_ids.push_back(prev_link[curr]);
        }
    }
    std::reverse(path.nodes.begin(), path.nodes.end());
    std::reverse(path.link_ids.begin(), path.link_ids.end());

    return path;
}

std::vector<Path> yenKShortestPaths(const Topology& topo, int src, int dst, int k) {
    if (k <= 0 || src == dst || src < 0 || src >= topo.numNodes() || dst < 0 ||
        dst >= topo.numNodes()) {
        return {};
    }

    std::vector<Path> A;  // Confirmed k-shortest paths
    A.reserve(k);

    // 1st shortest path
    Path p0 = dijkstraShortestPath(topo, src, dst);
    if (p0.empty()) {
        return A;
    }
    A.push_back(p0);

    // Candidate paths B (min-heap ordered by path length)
    auto path_cmp = [](const Path& a, const Path& b) {
        if (a.length_km != b.length_km)
            return a.length_km > b.length_km;
        return a.nodes > b.nodes;
    };
    std::priority_queue<Path, std::vector<Path>, decltype(path_cmp)> B(path_cmp);

    for (int i = 1; i < k; ++i) {
        const Path& prev_path = A[i - 1];

        // Iterate over spur nodes from s up to second-to-last node of prev_path
        for (size_t j = 0; j < prev_path.nodes.size() - 1; ++j) {
            int spur_node = prev_path.nodes[j];

            // Root path: prefix of prev_path from 0 to j
            std::vector<int> root_nodes(prev_path.nodes.begin(), prev_path.nodes.begin() + j + 1);
            std::vector<int> root_links(prev_path.link_ids.begin(), prev_path.link_ids.begin() + j);

            // Compute root path length
            double root_len = 0.0;
            for (size_t r = 0; r < root_links.size(); ++r) {
                root_len += topo.getLink(root_links[r]).length_km;
            }

            // Excluded edges: edges following root_path in any confirmed path in A
            std::unordered_set<int64_t> excluded_edges;
            for (const auto& a_path : A) {
                if (a_path.nodes.size() > j &&
                    std::equal(root_nodes.begin(), root_nodes.end(), a_path.nodes.begin())) {
                    excluded_edges.insert(
                        makeDirectedEdgeKey(a_path.nodes[j], a_path.nodes[j + 1]));
                }
            }

            // Excluded nodes: all nodes in root_path except spur_node to prevent loops
            std::unordered_set<int> excluded_nodes;
            for (size_t r = 0; r < j; ++r) {
                excluded_nodes.insert(root_nodes[r]);
            }

            // Find spur path from spur_node to dst
            Path spur_path =
                dijkstraShortestPath(topo, spur_node, dst, excluded_nodes, excluded_edges);

            if (!spur_path.empty()) {
                Path candidate;
                candidate.nodes = root_nodes;
                candidate.nodes.pop_back();  // Avoid duplicate spur_node
                candidate.nodes.insert(candidate.nodes.end(), spur_path.nodes.begin(),
                                       spur_path.nodes.end());

                candidate.link_ids = root_links;
                candidate.link_ids.insert(candidate.link_ids.end(), spur_path.link_ids.begin(),
                                          spur_path.link_ids.end());

                candidate.length_km = root_len + spur_path.length_km;

                // Push candidate to B (avoiding duplicates)
                B.push(candidate);
            }
        }

        if (B.empty()) {
            break;
        }

        // Pop best unique candidate from B
        bool found_new = false;
        while (!B.empty()) {
            Path best_candidate = B.top();
            B.pop();

            // Check if already in A
            bool is_duplicate = false;
            for (const auto& confirmed : A) {
                if (confirmed == best_candidate) {
                    is_duplicate = true;
                    break;
                }
            }

            if (!is_duplicate) {
                A.push_back(best_candidate);
                found_new = true;
                break;
            }
        }

        if (!found_new) {
            break;
        }
    }

    return A;
}

KShortestPathCache::KShortestPathCache(const Topology& topo, int k)
    : num_nodes_(topo.numNodes()), k_(k), cache_(num_nodes_ * num_nodes_) {
    for (int src = 0; src < num_nodes_; ++src) {
        for (int dst = 0; dst < num_nodes_; ++dst) {
            if (src != dst) {
                cache_[src * num_nodes_ + dst] = yenKShortestPaths(topo, src, dst, k);
            }
        }
    }
}

const std::vector<Path>& KShortestPathCache::getKShortestPaths(int src, int dst) const {
    if (src < 0 || src >= num_nodes_ || dst < 0 || dst >= num_nodes_ || src == dst) {
        static const std::vector<Path> empty_paths;
        return empty_paths;
    }
    return cache_[src * num_nodes_ + dst];
}

}  // namespace optiplan
