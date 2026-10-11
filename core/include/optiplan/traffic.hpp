#pragma once

/// @file traffic.hpp
/// @brief Synthetic dynamic traffic generation (Poisson arrivals, exponential holding times).

#include <random>
#include <vector>

namespace optiplan {

/// Dynamic connection demand request
struct Request {
    int id = -1;
    int src = -1;
    int dst = -1;
    double bitrate_Gbps = 100.0;
    double arrival_time = 0.0;
    double holding_time = 1.0;

    double departure_time() const { return arrival_time + holding_time; }
};

/// Generates reproducible Poisson traffic traces
class TrafficGenerator {
public:
    explicit TrafficGenerator(int num_nodes, uint64_t seed = 42);

    /// Generate dynamic traffic requests according to load in Erlangs
    /// @param num_requests Total number of requests to generate
    /// @param load_erlangs Traffic load (Erlang = arrival_rate * mean_holding_time)
    /// @param mean_holding_time Mean connection duration [s] (default 1.0)
    /// @param bitrate_Gbps Requested line rate [Gb/s] (default 100.0)
    std::vector<Request> generateRequests(int num_requests,
                                         double load_erlangs,
                                         double mean_holding_time = 1.0,
                                         double bitrate_Gbps = 100.0);

    void setSeed(uint64_t seed) { rng_.seed(seed); }

private:
    int num_nodes_;
    std::mt19937_64 rng_;
};

}  // namespace optiplan
