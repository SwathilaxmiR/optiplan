/// @file spectrum.cpp
/// @brief Implementation of SpectrumState bitset operations and assignment policies.

#include "optiplan/spectrum.hpp"

#include <algorithm>
#include <stdexcept>

namespace optiplan {

SpectrumState::SpectrumState(int num_links, int num_channels)
    : num_channels_(num_channels), link_occupancy_(num_links) {
    if (num_channels <= 0 || num_channels > static_cast<int>(MAX_CHANNELS)) {
        throw std::invalid_argument("SpectrumState: num_channels must be between 1 and " +
                                    std::to_string(MAX_CHANNELS));
    }
    for (int i = 0; i < num_channels_; ++i) {
        valid_channel_mask_.set(i);
    }
}

const SlotMask& SpectrumState::getOccupiedSlots(int link_id) const {
    if (link_id < 0 || link_id >= static_cast<int>(link_occupancy_.size())) {
        throw std::out_of_range("SpectrumState::getOccupiedSlots: invalid link_id");
    }
    return link_occupancy_[link_id];
}

SlotMask SpectrumState::getFreeSlots(int link_id) const {
    return (~getOccupiedSlots(link_id)) & valid_channel_mask_;
}

int SpectrumState::getOccupiedChannelCount(int link_id) const {
    return static_cast<int>(getOccupiedSlots(link_id).count());
}

double SpectrumState::getLinkUtilization(int link_id) const {
    if (num_channels_ == 0)
        return 0.0;
    return static_cast<double>(getOccupiedChannelCount(link_id)) / num_channels_;
}

double SpectrumState::getAverageUtilization() const {
    if (link_occupancy_.empty() || num_channels_ == 0)
        return 0.0;
    size_t total_occupied = 0;
    for (const auto& mask : link_occupancy_) {
        total_occupied += mask.count();
    }
    return static_cast<double>(total_occupied) /
           (link_occupancy_.size() * static_cast<double>(num_channels_));
}

int SpectrumState::getGlobalSlotUsage(int slot_idx) const {
    if (slot_idx < 0 || slot_idx >= num_channels_) {
        return 0;
    }
    int count = 0;
    for (const auto& mask : link_occupancy_) {
        if (mask.test(slot_idx)) {
            count++;
        }
    }
    return count;
}

SlotMask SpectrumState::findAvailableSlots(const Path& path) const {
    SlotMask free_mask = valid_channel_mask_;
    for (int link_id : path.link_ids) {
        if (link_id < 0 || link_id >= static_cast<int>(link_occupancy_.size())) {
            throw std::out_of_range("SpectrumState::findAvailableSlots: invalid link_id in path");
        }
        free_mask &= ~link_occupancy_[link_id];
    }
    return free_mask;
}

bool SpectrumState::isSlotAvailable(const Path& path, int slot_idx) const {
    if (slot_idx < 0 || slot_idx >= num_channels_) {
        return false;
    }
    for (int link_id : path.link_ids) {
        if (link_id < 0 || link_id >= static_cast<int>(link_occupancy_.size())) {
            throw std::out_of_range("SpectrumState::isSlotAvailable: invalid link_id in path");
        }
        if (link_occupancy_[link_id].test(slot_idx)) {
            return false;
        }
    }
    return true;
}

void SpectrumState::allocateSlot(const Path& path, int slot_idx) {
    if (slot_idx < 0 || slot_idx >= num_channels_) {
        throw std::out_of_range("SpectrumState::allocateSlot: slot_idx out of range");
    }

    // Verify non-overlap constraint on all links before mutating
    for (int link_id : path.link_ids) {
        if (link_id < 0 || link_id >= static_cast<int>(link_occupancy_.size())) {
            throw std::out_of_range("SpectrumState::allocateSlot: invalid link_id in path");
        }
        if (link_occupancy_[link_id].test(slot_idx)) {
            throw std::runtime_error("SpectrumState::allocateSlot: slot " +
                                     std::to_string(slot_idx) + " already occupied on link " +
                                     std::to_string(link_id));
        }
    }

    // Mutate state
    for (int link_id : path.link_ids) {
        link_occupancy_[link_id].set(slot_idx);
    }
}

void SpectrumState::releaseSlot(const Path& path, int slot_idx) {
    if (slot_idx < 0 || slot_idx >= num_channels_) {
        throw std::out_of_range("SpectrumState::releaseSlot: slot_idx out of range");
    }

    for (int link_id : path.link_ids) {
        if (link_id < 0 || link_id >= static_cast<int>(link_occupancy_.size())) {
            throw std::out_of_range("SpectrumState::releaseSlot: invalid link_id in path");
        }
        if (!link_occupancy_[link_id].test(slot_idx)) {
            throw std::runtime_error("SpectrumState::releaseSlot: slot " +
                                     std::to_string(slot_idx) + " was not occupied on link " +
                                     std::to_string(link_id));
        }
    }

    for (int link_id : path.link_ids) {
        link_occupancy_[link_id].reset(slot_idx);
    }
}

void SpectrumState::reset() {
    for (auto& mask : link_occupancy_) {
        mask.reset();
    }
}

// ─── First-Fit Policy ────────────────────────────────────────────────────────
int FirstFitPolicy::selectSlot(const Path& /*path*/, const SlotMask& available_slots,
                               const SpectrumState& state) {
    for (int i = 0; i < state.numChannels(); ++i) {
        if (available_slots.test(i)) {
            return i;
        }
    }
    return -1;
}

// ─── Most-Used Policy ────────────────────────────────────────────────────────
int MostUsedPolicy::selectSlot(const Path& /*path*/, const SlotMask& available_slots,
                               const SpectrumState& state) {
    int best_slot = -1;
    int max_usage = -1;

    for (int i = 0; i < state.numChannels(); ++i) {
        if (available_slots.test(i)) {
            int usage = state.getGlobalSlotUsage(i);
            if (usage > max_usage) {
                max_usage = usage;
                best_slot = i;
            }
        }
    }
    return best_slot;
}

// ─── Random-Fit Policy ───────────────────────────────────────────────────────
RandomFitPolicy::RandomFitPolicy(uint64_t seed) : rng_(seed) {}

int RandomFitPolicy::selectSlot(const Path& /*path*/, const SlotMask& available_slots,
                                const SpectrumState& state) {
    std::vector<int> candidates;
    candidates.reserve(state.numChannels());

    for (int i = 0; i < state.numChannels(); ++i) {
        if (available_slots.test(i)) {
            candidates.push_back(i);
        }
    }

    if (candidates.empty()) {
        return -1;
    }

    std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
    return candidates[dist(rng_)];
}

}  // namespace optiplan
