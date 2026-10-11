#pragma once

/// @file topology.hpp
/// @brief Optical network topology model, nodes, links, and JSON parser.

#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace optiplan {

/// Optical network node (ROADM / OXC site)
struct Node {
    int id = -1;
    std::string name;
    std::string label;
    double longitude = 0.0;
    double latitude = 0.0;
};

/// Bidirectional physical optical fiber link between two nodes
struct Link {
    int id = -1;
    int source = -1;
    int target = -1;
    double length_km = 0.0;
    std::string label;
};

/// Directed edge representing optical propagation in one direction
struct DirectedEdge {
    int link_id = -1;
    int from = -1;
    int to = -1;
    double length_km = 0.0;
};

/// Optical network topology graph
class Topology {
public:
    Topology() = default;

    /// Load topology from a JSON file
    static Topology loadFromFile(const std::string& filepath);

    /// Load topology from a JSON object
    static Topology loadFromJson(const nlohmann::json& j);

    /// Name of topology
    const std::string& name() const { return name_; }

    /// Description of topology
    const std::string& description() const { return description_; }

    /// Total number of nodes
    int numNodes() const { return static_cast<int>(nodes_.size()); }

    /// Total number of bidirectional physical links
    int numLinks() const { return static_cast<int>(links_.size()); }

    /// Access node by ID
    const Node& getNode(int node_id) const;

    /// Access link by ID
    const Link& getLink(int link_id) const;

    /// Get all nodes
    const std::vector<Node>& nodes() const { return nodes_; }

    /// Get all links
    const std::vector<Link>& links() const { return links_; }

    /// Get outgoing directed edges from a given node
    const std::vector<DirectedEdge>& getOutgoingEdges(int node_id) const;

    /// Find physical link ID between two nodes, or -1 if no direct link
    int findLinkId(int u, int v) const;

    /// Verify if graph is fully connected (all nodes mutually reachable)
    bool isConnected() const;

private:
    std::string name_;
    std::string description_;
    std::string source_;
    std::vector<Node> nodes_;
    std::vector<Link> links_;

    // Adjacency lists for fast routing: node_id -> list of outgoing edges
    std::vector<std::vector<DirectedEdge>> adj_;

    // Lookup: (min(u,v), max(u,v)) -> link_id
    std::unordered_map<int64_t, int> link_lookup_;

    static int64_t makeLinkKey(int u, int v) {
        int lo = std::min(u, v);
        int hi = std::max(u, v);
        return (static_cast<int64_t>(lo) << 32) | static_cast<uint32_t>(hi);
    }
};

}  // namespace optiplan
