#pragma once

/// @file spectrum.hpp
/// @brief Bitset spectrum representation, wavelength continuity, and allocation policies.

#include <bitset>
#include <memory>
#include <random>
#include <vector>

#include "optiplan/routing.hpp"

namespace optiplan {

/// Maximum number of WDM channels supported by fixed bitset (C-band standard is 80)
constexpr size_t MAX_CHANNELS = 128;
using SlotMask = std::bitset<MAX_CHANNELS>;

/// State of spectrum utilization across all links in the optical network.
class SpectrumState {
public:
    SpectrumState() = default;

    /// Initialize spectrum state for num_links with num_channels (default 80)
    explicit SpectrumState(int num_links, int num_channels = 80);

    /// Number of links tracked
    int numLinks() const { return static_cast<int>(link_occupancy_.size()); }

    /// Number of optical frequency slots in grid
    int numChannels() const { return num_channels_; }

    /// Bitmask of occupied slots on a given link (1 = occupied, 0 = free)
    const SlotMask& getOccupiedSlots(int link_id) const;

    /// Bitmask of free slots on a given link (1 = free, 0 = occupied)
    SlotMask getFreeSlots(int link_id) const;

    /// Number of occupied channels on a given link
    int getOccupiedChannelCount(int link_id) const;

    /// Link utilization ratio [0.0, 1.0]
    double getLinkUtilization(int link_id) const;

    /// Average network-wide spectrum utilization [0.0, 1.0]
    double getAverageUtilization() const;

    /// Global usage count of a specific slot across all links
    int getGlobalSlotUsage(int slot_idx) const;

    /// Find all slots free along an entire path (satisfying wavelength continuity)
    SlotMask findAvailableSlots(const Path& path) const;

    /// Check if a specific slot is available along an entire path
    bool isSlotAvailable(const Path& path, int slot_idx) const;

    /// Allocate a slot along a path (sets bit on all links in path)
    /// Throws std::runtime_error if slot is already occupied on any link.
    void allocateSlot(const Path& path, int slot_idx);

    /// Release a slot along a path (clears bit on all links in path)
    /// Throws std::runtime_error if slot was not occupied on any link.
    void releaseSlot(const Path& path, int slot_idx);

    /// Reset all spectrum (all channels free)
    void reset();

private:
    int num_channels_ = 80;
    std::vector<SlotMask> link_occupancy_;  // Indexed by link_id
    SlotMask valid_channel_mask_;           // Mask with bits [0, num_channels_-1] set to 1
};

/// Abstract strategy interface for spectrum allocation policy
class ISpectrumPolicy {
public:
    virtual ~ISpectrumPolicy() = default;

    /// Name of spectrum allocation policy
    virtual std::string name() const = 0;

    /// Select a slot from available_slots according to policy heuristic
    /// @return Chosen slot index in [0, numChannels-1], or -1 if no slots available
    virtual int selectSlot(const Path& path, const SlotMask& available_slots,
                           const SpectrumState& state) = 0;
};

/// First-Fit Spectrum Assignment: chooses lowest available slot index
class FirstFitPolicy : public ISpectrumPolicy {
public:
    std::string name() const override { return "First-Fit"; }
    int selectSlot(const Path& path, const SlotMask& available_slots,
                   const SpectrumState& state) override;
};

/// Most-Used Spectrum Assignment: chooses slot with highest network-wide utilization
class MostUsedPolicy : public ISpectrumPolicy {
public:
    std::string name() const override { return "Most-Used"; }
    int selectSlot(const Path& path, const SlotMask& available_slots,
                   const SpectrumState& state) override;
};

/// Random-Fit Spectrum Assignment: chooses uniformly at random among available slots
class RandomFitPolicy : public ISpectrumPolicy {
public:
    explicit RandomFitPolicy(uint64_t seed = 42);

    std::string name() const override { return "Random-Fit"; }
    int selectSlot(const Path& path, const SlotMask& available_slots,
                   const SpectrumState& state) override;

    void setSeed(uint64_t seed) { rng_.seed(seed); }

private:
    std::mt19937_64 rng_;
};

}  // namespace optiplan
