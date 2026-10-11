#pragma once

/// @file qot_model.hpp
/// @brief Abstract Quality of Transmission (QoT) model interface.

#include <vector>

#include "optiplan/config.hpp"

namespace optiplan {

/// Information about a single link segment traversed by a lightpath.
struct LinkSegment {
    double length_km;
    int occupied_channels = 1;  // Number of active channels on this link
};

/// Detailed QoT evaluation output for a lightpath.
struct QoTResult {
    double snr_linear = 0.0;   // Effective SNR (linear) in signal bandwidth
    double snr_db = 0.0;       // Effective SNR [dB]
    double osnr_linear = 0.0;  // OSNR (linear) in reference bandwidth (12.5 GHz)
    double osnr_db = 0.0;      // OSNR [dB]
    double p_ase_w = 0.0;      // Total ASE noise power [W] in signal bandwidth
    double p_nli_w = 0.0;      // Total NLI noise power [W] in signal bandwidth
    double p_opt_w = 0.0;      // Analytical optimum launch power [W]
    double p_opt_dbm = 0.0;    // Analytical optimum launch power [dBm]
    double eta_nli = 0.0;      // Total NLI coefficient [W^-2] where P_NLI = eta * P_ch^3
    int total_spans = 0;       // Total number of spans traversed
};

/// Strategy interface for Quality of Transmission evaluation.
class IQoTModel {
public:
    virtual ~IQoTModel() = default;

    /// Evaluate QoT for a lightpath traversing a sequence of link segments.
    /// @param path_segments Sequence of links traversed and their channel occupancies
    /// @param launch_power_dBm Channel launch power [dBm]
    /// @param config Simulation configuration (fiber, amplifier, channel grid)
    virtual QoTResult evaluatePath(const std::vector<LinkSegment>& path_segments,
                                   double launch_power_dBm, const Config& config) const = 0;
};

}  // namespace optiplan
