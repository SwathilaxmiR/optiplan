#pragma once

/// @file lightpath.hpp
/// @brief Representation of active provisioned lightpaths in the optical network.

#include <memory>

#include "optiplan/modulation.hpp"
#include "optiplan/routing.hpp"

namespace optiplan {

/// An established optical lightpath carrying user traffic
struct Lightpath {
    int id = -1;                          ///< Unique lightpath ID
    int request_id = -1;                  ///< Associated demand request ID
    Path path;                            ///< Optical path traversed
    int slot_idx = -1;                    ///< Assigned frequency slot index
    std::shared_ptr<ModulationFormat> format; ///< Modulation format used
    double launch_power_dBm = 0.0;        ///< Optical launch power [dBm]
    double snr_db = 0.0;                  ///< Evaluated SNR [dB]
    double snr_margin_db = 0.0;           ///< Margin above required threshold [dB]
    double established_time = 0.0;        ///< Simulation arrival timestamp [s]
    double teardown_time = 0.0;           ///< Simulation departure timestamp [s]

    /// Bitrate carried by this lightpath [Gb/s]
    double bitrateGbps() const {
        return format ? format->netBitRateGbps() : 0.0;
    }
};

}  // namespace optiplan
