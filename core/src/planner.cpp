/// @file planner.cpp
/// @brief Implementation of FixedMarginPlanner and QoTAwarePlanner.

#include "optiplan/planner.hpp"

#include <algorithm>
#include <unordered_set>

#include "optiplan/gn_model.hpp"

namespace optiplan {

// ─── FixedMarginPlanner ──────────────────────────────────────────────────────

FixedMarginPlanner::FixedMarginPlanner(std::shared_ptr<ISpectrumPolicy> policy)
    : policy_(policy ? policy : std::make_shared<FirstFitPolicy>()) {}

std::shared_ptr<ModulationFormat> FixedMarginPlanner::selectFormatByReach(double path_length_km) {
    if (path_length_km <= 600.0) {
        return std::make_shared<DP_16QAM>();
    } else if (path_length_km <= 1200.0) {
        return std::make_shared<DP_8QAM>();
    } else if (path_length_km <= 2500.0) {
        return std::make_shared<DP_QPSK>();
    } else if (path_length_km <= 5000.0) {
        return std::make_shared<DP_BPSK>();
    }
    return nullptr;  // Beyond physical optical reach
}

PlanningDecision FixedMarginPlanner::planRequest(
    const Request& request,
    const Topology& /*topo*/,
    const KShortestPathCache& ksp_cache,
    SpectrumState& spectrum,
    const std::unordered_map<int, Lightpath>& /*active_lightpaths*/,
    const Config& config) {
    PlanningDecision decision;

    const auto& candidate_paths = ksp_cache.getKShortestPaths(request.src, request.dst);
    if (candidate_paths.empty()) {
        decision.failure_reason = "No route found between source and destination";
        return decision;
    }

    for (const auto& path : candidate_paths) {
        decision.candidates_evaluated++;

        auto format = selectFormatByReach(path.length_km);
        if (!format || format->netBitRateGbps() < request.bitrate_Gbps) {
            continue;  // Format cannot support requested line rate over this distance
        }

        SlotMask available_slots = spectrum.findAvailableSlots(path);
        int slot = policy_->selectSlot(path, available_slots, spectrum);
        if (slot != -1) {
            decision.accepted = true;
            decision.lightpath.id = request.id;
            decision.lightpath.request_id = request.id;
            decision.lightpath.path = path;
            decision.lightpath.slot_idx = slot;
            decision.lightpath.format = format;
            decision.lightpath.launch_power_dBm = config.channel.launch_power_dBm;
            decision.lightpath.established_time = request.arrival_time;
            decision.lightpath.teardown_time = request.departure_time();
            return decision;
        }
    }

    decision.failure_reason = "Spectrum exhausted along all candidate paths";
    return decision;
}

// ─── QoTAwarePlanner ─────────────────────────────────────────────────────────

QoTAwarePlanner::QoTAwarePlanner(std::shared_ptr<IQoTModel> qot_model,
                                 std::shared_ptr<ISpectrumPolicy> policy,
                                 double design_margin_dB)
    : qot_model_(qot_model ? qot_model : std::make_shared<GNModel>(0.05)),
      policy_(policy ? policy : std::make_shared<FirstFitPolicy>()),
      design_margin_dB_(design_margin_dB),
      formats_(getStandardModulationFormats()) {}

PlanningDecision QoTAwarePlanner::planRequest(
    const Request& request,
    const Topology& topo,
    const KShortestPathCache& ksp_cache,
    SpectrumState& spectrum,
    const std::unordered_map<int, Lightpath>& active_lightpaths,
    const Config& config) {
    PlanningDecision decision;

    const auto& candidate_paths = ksp_cache.getKShortestPaths(request.src, request.dst);
    if (candidate_paths.empty()) {
        decision.failure_reason = "No candidate route available";
        return decision;
    }

    for (const auto& path : candidate_paths) {
        SlotMask available_slots = spectrum.findAvailableSlots(path);
        if (available_slots.none()) {
            continue;
        }

        // Evaluate candidate QoT with tentative occupancy (+1 channel on each traversed link)
        std::vector<LinkSegment> segments;
        segments.reserve(path.link_ids.size());
        for (int lid : path.link_ids) {
            int current_occ = spectrum.getOccupiedChannelCount(lid);
            segments.push_back({topo.getLink(lid).length_km, current_occ + 1});
        }

        auto qot = qot_model_->evaluatePath(segments, config.channel.launch_power_dBm, config);

        // Try modulation formats from highest rate to lowest rate
        for (auto fmt_it = formats_.rbegin(); fmt_it != formats_.rend(); ++fmt_it) {
            const auto& candidate_fmt = *fmt_it;
            if (candidate_fmt->netBitRateGbps() < request.bitrate_Gbps) {
                continue;
            }

            double req_snr = candidate_fmt->requiredSnrDb(0.01, design_margin_dB_);
            if (qot.snr_db < req_snr) {
                continue;  // Format threshold not met
            }

            // Test slots in order of policy
            SlotMask candidate_slots = available_slots;
            while (candidate_slots.any()) {
                decision.candidates_evaluated++;
                int slot = policy_->selectSlot(path, candidate_slots, spectrum);
                if (slot == -1) break;
                candidate_slots.reset(slot);

                // ─── Re-Validation of affected active lightpaths ───────────
                // A new lightpath on this route adds 1 channel of NLI to any existing
                // lightpath that traverses links in path.link_ids.
                std::unordered_set<int> path_links(path.link_ids.begin(), path.link_ids.end());
                bool all_revalidated_ok = true;

                for (const auto& [lp_id, live_lp] : active_lightpaths) {
                    bool shares_link = false;
                    for (int live_lid : live_lp.path.link_ids) {
                        if (path_links.count(live_lid)) {
                            shares_link = true;
                            break;
                        }
                    }

                    if (!shares_link) continue;

                    decision.revalidations_performed++;

                    // Re-evaluate live lightpath QoT with new occupancy (+1 on shared links)
                    std::vector<LinkSegment> live_segments;
                    live_segments.reserve(live_lp.path.link_ids.size());
                    for (int live_lid : live_lp.path.link_ids) {
                        int live_occ = spectrum.getOccupiedChannelCount(live_lid);
                        if (path_links.count(live_lid)) {
                            live_occ += 1;  // Candidate adds NLI
                        }
                        live_segments.push_back({topo.getLink(live_lid).length_km, live_occ});
                    }

                    auto reval_qot = qot_model_->evaluatePath(
                        live_segments, live_lp.launch_power_dBm, config);
                    double min_allowed_snr = live_lp.format->requiredSnrDb(0.01, 0.0);

                    if (reval_qot.snr_db < min_allowed_snr) {
                        all_revalidated_ok = false;
                        break;  // Cannot accept candidate: it degrades existing live traffic!
                    }
                }

                if (all_revalidated_ok) {
                    // Candidate passes all physics and network constraints!
                    decision.accepted = true;
                    decision.lightpath.id = request.id;
                    decision.lightpath.request_id = request.id;
                    decision.lightpath.path = path;
                    decision.lightpath.slot_idx = slot;
                    decision.lightpath.format = candidate_fmt;
                    decision.lightpath.launch_power_dBm = config.channel.launch_power_dBm;
                    decision.lightpath.snr_db = qot.snr_db;
                    decision.lightpath.snr_margin_db = qot.snr_db - req_snr;
                    decision.lightpath.established_time = request.arrival_time;
                    decision.lightpath.teardown_time = request.departure_time();
                    return decision;
                }
            }
        }
    }

    decision.failure_reason = "No candidate path meets QoT and spectrum requirements";
    return decision;
}

}  // namespace optiplan
