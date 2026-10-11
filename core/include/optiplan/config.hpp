#pragma once

/// @file config.hpp
/// @brief Configuration loading and parameter structures for OptiPlan.
///
/// All physical and simulation parameters are loaded from JSON files.
/// No parameter is hard-coded; defaults live in configs/default.json.

#include <nlohmann/json.hpp>
#include <string>

namespace optiplan {

/// Fiber parameters.
struct FiberParams {
    double attenuation_dB_per_km = 0.2;
    double dispersion_ps_nm_km = 16.7;
    double gamma_per_W_km = 1.3;
    double span_length_km = 80.0;
};

/// EDFA amplifier parameters.
struct AmplifierParams {
    double noise_figure_dB = 5.0;
};

/// Channel / WDM grid parameters.
struct ChannelParams {
    double center_frequency_THz = 193.1;
    double symbol_rate_GBaud = 32.0;
    double channel_spacing_GHz = 50.0;
    int num_channels = 80;
    double launch_power_dBm = 0.0;
};

/// Top-level simulation configuration.
struct Config {
    FiberParams fiber;
    AmplifierParams amplifier;
    ChannelParams channel;
    double reference_bandwidth_GHz = 12.5;

    /// Load configuration from a JSON file. Fields not present in the file
    /// keep their default values.
    static Config loadFromFile(const std::string& filepath);

    /// Load configuration from a JSON object.
    static Config loadFromJson(const nlohmann::json& j);
};

}  // namespace optiplan
