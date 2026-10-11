#pragma once

/// @file simulator.hpp
/// @brief Discrete event-driven dynamic network simulator for optical traffic.

#include <chrono>
#include <memory>
#include <queue>
#include <unordered_map>
#include <vector>

#include "optiplan/config.hpp"
#include "optiplan/lightpath.hpp"
#include "optiplan/planner.hpp"
#include "optiplan/routing.hpp"
#include "optiplan/spectrum.hpp"
#include "optiplan/topology.hpp"
#include "optiplan/traffic.hpp"

namespace optiplan {

/// Aggregated simulation metrics
struct SimulationMetrics {
    int total_requests = 0;
    int accepted_requests = 0;
    int blocked_requests = 0;
    double request_blocking_probability = 0.0;

    double requested_bandwidth_gbps = 0.0;
    double blocked_bandwidth_gbps = 0.0;
    double bandwidth_blocking_probability = 0.0;

    double average_snr_margin_db = 0.0;
    double min_snr_margin_db = 0.0;
    int total_qot_revalidations = 0;
    double average_link_utilization = 0.0;
    double wall_clock_time_ms = 0.0;
};

/// Event in dynamic discrete-event simulation
enum class EventType { ARRIVAL, DEPARTURE };

struct SimulationEvent {
    EventType type;
    double time = 0.0;
    Request request;
    int lightpath_id = -1;

    bool operator>(const SimulationEvent& other) const {
        return time > other.time;
    }
};

/// Discrete event-driven dynamic network simulator
class Simulator {
public:
    Simulator(Topology topo,
              std::shared_ptr<IPlanner> planner,
              Config config = Config{},
              int k = 3);

    /// Execute dynamic simulation on a sequence of traffic requests
    SimulationMetrics run(const std::vector<Request>& requests);

    /// Access current spectrum state
    const SpectrumState& spectrum() const { return spectrum_; }

    /// Access active lightpaths
    const std::unordered_map<int, Lightpath>& activeLightpaths() const {
        return active_lightpaths_;
    }

    /// Reset simulator state
    void reset();

private:
    Topology topo_;
    std::shared_ptr<IPlanner> planner_;
    Config config_;
    KShortestPathCache ksp_cache_;
    SpectrumState spectrum_;
    std::unordered_map<int, Lightpath> active_lightpaths_;
};

}  // namespace optiplan
