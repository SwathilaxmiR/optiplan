/// @file simulator.cpp
/// @brief Implementation of discrete event-driven dynamic network simulator.

#include "optiplan/simulator.hpp"

#include <chrono>
#include <limits>
#include <stdexcept>

namespace optiplan {

Simulator::Simulator(Topology topo,
                     std::shared_ptr<IPlanner> planner,
                     Config config,
                     int k)
    : topo_(std::move(topo)),
      planner_(std::move(planner)),
      config_(std::move(config)),
      ksp_cache_(topo_, k),
      spectrum_(topo_.numLinks(), config_.channel.num_channels) {
    if (!planner_) {
        throw std::invalid_argument("Simulator: planner cannot be null");
    }
}

void Simulator::reset() {
    spectrum_.reset();
    active_lightpaths_.clear();
}

SimulationMetrics Simulator::run(const std::vector<Request>& requests) {
    reset();
    SimulationMetrics metrics;
    metrics.total_requests = static_cast<int>(requests.size());

    if (requests.empty()) {
        return metrics;
    }

    auto start_time = std::chrono::steady_clock::now();

    // Event priority queue ordered by time (smallest time first)
    std::priority_queue<SimulationEvent, std::vector<SimulationEvent>, std::greater<SimulationEvent>>
        event_queue;

    for (const auto& req : requests) {
        event_queue.push(SimulationEvent{EventType::ARRIVAL, req.arrival_time, req, -1});
        metrics.requested_bandwidth_gbps += req.bitrate_Gbps;
    }

    double total_margin = 0.0;
    double min_margin = std::numeric_limits<double>::infinity();
    int accepted_count = 0;

    while (!event_queue.empty()) {
        auto event = event_queue.top();
        event_queue.pop();

        if (event.type == EventType::ARRIVAL) {
            auto decision = planner_->planRequest(event.request, topo_, ksp_cache_, spectrum_,
                                                  active_lightpaths_, config_);
            metrics.total_qot_revalidations += decision.revalidations_performed;

            if (decision.accepted) {
                // Commit slot allocation
                spectrum_.allocateSlot(decision.lightpath.path, decision.lightpath.slot_idx);
                active_lightpaths_[decision.lightpath.id] = decision.lightpath;

                accepted_count++;
                total_margin += decision.lightpath.snr_margin_db;
                if (decision.lightpath.snr_margin_db < min_margin) {
                    min_margin = decision.lightpath.snr_margin_db;
                }

                // Schedule departure event
                event_queue.push(SimulationEvent{EventType::DEPARTURE,
                                                 decision.lightpath.teardown_time,
                                                 Request{},
                                                 decision.lightpath.id});
            } else {
                metrics.blocked_requests++;
                metrics.blocked_bandwidth_gbps += event.request.bitrate_Gbps;
            }
        } else if (event.type == EventType::DEPARTURE) {
            auto it = active_lightpaths_.find(event.lightpath_id);
            if (it != active_lightpaths_.end()) {
                spectrum_.releaseSlot(it->second.path, it->second.slot_idx);
                active_lightpaths_.erase(it);
            }
        }
    }

    auto end_time = std::chrono::steady_clock::now();
    metrics.wall_clock_time_ms =
        std::chrono::duration<double, std::milli>(end_time - start_time).count();

    metrics.accepted_requests = accepted_count;
    if (metrics.total_requests > 0) {
        metrics.request_blocking_probability =
            static_cast<double>(metrics.blocked_requests) / metrics.total_requests;
    }
    if (metrics.requested_bandwidth_gbps > 0.0) {
        metrics.bandwidth_blocking_probability =
            metrics.blocked_bandwidth_gbps / metrics.requested_bandwidth_gbps;
    }
    if (accepted_count > 0) {
        metrics.average_snr_margin_db = total_margin / accepted_count;
        metrics.min_snr_margin_db = min_margin;
    }
    metrics.average_link_utilization = spectrum_.getAverageUtilization();

    return metrics;
}

}  // namespace optiplan
