/// @file topology.cpp
/// @brief Implementation of Topology loading, adjacency graph, and validation.

#include "optiplan/topology.hpp"

#include <fstream>
#include <queue>
#include <stdexcept>

namespace optiplan {

Topology Topology::loadFromFile(const std::string& filepath) {
    std::ifstream ifs(filepath);
    if (!ifs.is_open()) {
        throw std::runtime_error("Topology::loadFromFile: cannot open '" + filepath + "'");
    }
    nlohmann::json j;
    ifs >> j;
    return loadFromJson(j);
}

Topology Topology::loadFromJson(const nlohmann::json& j) {
    Topology topo;

    if (j.contains("name"))
        topo.name_ = j["name"].get<std::string>();
    if (j.contains("description"))
        topo.description_ = j["description"].get<std::string>();
    if (j.contains("source"))
        topo.source_ = j["source"].get<std::string>();

    if (!j.contains("nodes") || !j["nodes"].is_array()) {
        throw std::invalid_argument("Topology::loadFromJson: missing 'nodes' array");
    }
    if (!j.contains("links") || !j["links"].is_array()) {
        throw std::invalid_argument("Topology::loadFromJson: missing 'links' array");
    }

    // Parse nodes
    std::unordered_map<std::string, int> node_name_to_idx;
    int num_nodes = static_cast<int>(j["nodes"].size());
    topo.nodes_.resize(num_nodes);

    for (int i = 0; i < num_nodes; ++i) {
        const auto& node_j = j["nodes"][i];
        Node n;

        // Support string or int node IDs
        if (node_j["id"].is_string()) {
            n.id = std::stoi(node_j["id"].get<std::string>());
        } else {
            n.id = node_j["id"].get<int>();
        }

        if (n.id < 0 || n.id >= num_nodes) {
            throw std::invalid_argument("Topology: node id " + std::to_string(n.id) +
                                        " out of range [0, " + std::to_string(num_nodes - 1) + "]");
        }

        if (node_j.contains("name"))
            n.name = node_j["name"].get<std::string>();
        if (node_j.contains("label"))
            n.label = node_j["label"].get<std::string>();
        if (node_j.contains("longitude"))
            n.longitude = node_j["longitude"].get<double>();
        if (node_j.contains("latitude"))
            n.latitude = node_j["latitude"].get<double>();

        topo.nodes_[n.id] = n;
        node_name_to_idx[n.name] = n.id;
    }

    topo.adj_.resize(num_nodes);

    // Parse links
    int num_links = static_cast<int>(j["links"].size());
    topo.links_.resize(num_links);

    for (int i = 0; i < num_links; ++i) {
        const auto& link_j = j["links"][i];
        Link l;

        if (link_j["id"].is_string()) {
            l.id = std::stoi(link_j["id"].get<std::string>());
        } else {
            l.id = link_j["id"].get<int>();
        }

        // Support source/target as node id (int/string) or name
        if (link_j["source"].is_string()) {
            std::string s_str = link_j["source"].get<std::string>();
            try {
                l.source = std::stoi(s_str);
            } catch (...) {
                if (node_name_to_idx.count(s_str)) {
                    l.source = node_name_to_idx[s_str];
                } else {
                    throw std::invalid_argument("Topology: unknown link source node '" + s_str +
                                                "'");
                }
            }
        } else {
            l.source = link_j["source"].get<int>();
        }

        if (link_j["target"].is_string()) {
            std::string t_str = link_j["target"].get<std::string>();
            try {
                l.target = std::stoi(t_str);
            } catch (...) {
                if (node_name_to_idx.count(t_str)) {
                    l.target = node_name_to_idx[t_str];
                } else {
                    throw std::invalid_argument("Topology: unknown link target node '" + t_str +
                                                "'");
                }
            }
        } else {
            l.target = link_j["target"].get<int>();
        }

        l.length_km = link_j["length_km"].get<double>();
        if (l.length_km <= 0.0) {
            throw std::invalid_argument("Topology: link length must be positive");
        }

        if (link_j.contains("label"))
            l.label = link_j["label"].get<std::string>();

        if (l.source < 0 || l.source >= num_nodes || l.target < 0 || l.target >= num_nodes) {
            throw std::invalid_argument("Topology: link endpoints out of range");
        }
        if (l.source == l.target) {
            throw std::invalid_argument("Topology: self-loop links not supported");
        }

        topo.links_[l.id] = l;

        // Populate directed adjacency
        DirectedEdge fwd{l.id, l.source, l.target, l.length_km};
        DirectedEdge rev{l.id, l.target, l.source, l.length_km};
        topo.adj_[l.source].push_back(fwd);
        topo.adj_[l.target].push_back(rev);

        // Lookup
        topo.link_lookup_[makeLinkKey(l.source, l.target)] = l.id;
    }

    return topo;
}

const Node& Topology::getNode(int node_id) const {
    if (node_id < 0 || node_id >= static_cast<int>(nodes_.size())) {
        throw std::out_of_range("Topology::getNode: node_id out of range");
    }
    return nodes_[node_id];
}

const Link& Topology::getLink(int link_id) const {
    if (link_id < 0 || link_id >= static_cast<int>(links_.size())) {
        throw std::out_of_range("Topology::getLink: link_id out of range");
    }
    return links_[link_id];
}

const std::vector<DirectedEdge>& Topology::getOutgoingEdges(int node_id) const {
    if (node_id < 0 || node_id >= static_cast<int>(adj_.size())) {
        throw std::out_of_range("Topology::getOutgoingEdges: node_id out of range");
    }
    return adj_[node_id];
}

int Topology::findLinkId(int u, int v) const {
    auto it = link_lookup_.find(makeLinkKey(u, v));
    if (it != link_lookup_.end()) {
        return it->second;
    }
    return -1;
}

bool Topology::isConnected() const {
    if (nodes_.empty())
        return true;

    std::vector<bool> visited(nodes_.size(), false);
    std::queue<int> q;
    q.push(0);
    visited[0] = true;
    int count = 1;

    while (!q.empty()) {
        int u = q.front();
        q.pop();

        for (const auto& edge : adj_[u]) {
            if (!visited[edge.to]) {
                visited[edge.to] = true;
                count++;
                q.push(edge.to);
            }
        }
    }

    return count == static_cast<int>(nodes_.size());
}

}  // namespace optiplan
