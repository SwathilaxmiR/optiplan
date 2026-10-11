/// @file traffic.cpp
/// @brief Implementation of Poisson traffic generation.

#include "optiplan/traffic.hpp"

#include <stdexcept>

namespace optiplan {

TrafficGenerator::TrafficGenerator(int num_nodes, uint64_t seed)
    : num_nodes_(num_nodes), rng_(seed) {
    if (num_nodes < 2) {
        throw std::invalid_argument("TrafficGenerator: num_nodes must be >= 2");
    }
}

std::vector<Request> TrafficGenerator::generateRequests(int num_requests,
                                                       double load_erlangs,
                                                       double mean_holding_time,
                                                       double bitrate_Gbps) {
    if (num_requests <= 0) return {};
    if (load_erlangs <= 0.0) {
        throw std::invalid_argument("TrafficGenerator: load_erlangs must be > 0");
    }
    if (mean_holding_time <= 0.0) {
        throw std::invalid_argument("TrafficGenerator: mean_holding_time must be > 0");
    }

    // Arrival rate: lambda = load / mean_holding_time
    const double lambda = load_erlangs / mean_holding_time;
    const double mu = 1.0 / mean_holding_time;

    std::exponential_distribution<double> arrival_dist(lambda);
    std::exponential_distribution<double> holding_dist(mu);
    std::uniform_int_distribution<int> node_dist(0, num_nodes_ - 1);

    std::vector<Request> requests;
    requests.reserve(num_requests);

    double current_time = 0.0;
    for (int i = 0; i < num_requests; ++i) {
        current_time += arrival_dist(rng_);
        double duration = holding_dist(rng_);

        int src = node_dist(rng_);
        int dst = node_dist(rng_);
        while (dst == src) {
            dst = node_dist(rng_);
        }

        Request req;
        req.id = i;
        req.src = src;
        req.dst = dst;
        req.bitrate_Gbps = bitrate_Gbps;
        req.arrival_time = current_time;
        req.holding_time = duration;

        requests.push_back(req);
    }

    return requests;
}

}  // namespace optiplan
