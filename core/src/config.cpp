/// @file config.cpp
/// @brief Implementation of Config loading from JSON files.

#include "optiplan/config.hpp"

#include <fstream>
#include <stdexcept>

namespace optiplan {

Config Config::loadFromFile(const std::string& filepath) {
    std::ifstream ifs(filepath);
    if (!ifs.is_open()) {
        throw std::runtime_error("Config::loadFromFile: cannot open '" + filepath + "'");
    }
    nlohmann::json j;
    ifs >> j;
    return loadFromJson(j);
}

Config Config::loadFromJson(const nlohmann::json& j) {
    Config cfg;

    if (j.contains("fiber")) {
        const auto& f = j["fiber"];
        if (f.contains("attenuation_dB_per_km"))
            cfg.fiber.attenuation_dB_per_km = f["attenuation_dB_per_km"].get<double>();
        if (f.contains("dispersion_ps_nm_km"))
            cfg.fiber.dispersion_ps_nm_km = f["dispersion_ps_nm_km"].get<double>();
        if (f.contains("gamma_per_W_km"))
            cfg.fiber.gamma_per_W_km = f["gamma_per_W_km"].get<double>();
        if (f.contains("span_length_km"))
            cfg.fiber.span_length_km = f["span_length_km"].get<double>();
    }

    if (j.contains("amplifier")) {
        const auto& a = j["amplifier"];
        if (a.contains("noise_figure_dB"))
            cfg.amplifier.noise_figure_dB = a["noise_figure_dB"].get<double>();
    }

    if (j.contains("channel")) {
        const auto& c = j["channel"];
        if (c.contains("center_frequency_THz"))
            cfg.channel.center_frequency_THz = c["center_frequency_THz"].get<double>();
        if (c.contains("symbol_rate_GBaud"))
            cfg.channel.symbol_rate_GBaud = c["symbol_rate_GBaud"].get<double>();
        if (c.contains("channel_spacing_GHz"))
            cfg.channel.channel_spacing_GHz = c["channel_spacing_GHz"].get<double>();
        if (c.contains("num_channels"))
            cfg.channel.num_channels = c["num_channels"].get<int>();
        if (c.contains("launch_power_dBm"))
            cfg.channel.launch_power_dBm = c["launch_power_dBm"].get<double>();
    }

    if (j.contains("reference_bandwidth_GHz"))
        cfg.reference_bandwidth_GHz = j["reference_bandwidth_GHz"].get<double>();

    return cfg;
}

}  // namespace optiplan
