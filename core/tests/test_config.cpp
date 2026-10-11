/// @file test_config.cpp
/// @brief Tests for Config loading from JSON.

#include <gtest/gtest.h>

#include "optiplan/config.hpp"

TEST(ConfigTest, DefaultValues) {
    // Loading an empty JSON should yield all default values.
    nlohmann::json j = nlohmann::json::object();
    auto cfg = optiplan::Config::loadFromJson(j);

    EXPECT_DOUBLE_EQ(cfg.fiber.attenuation_dB_per_km, 0.2);
    EXPECT_DOUBLE_EQ(cfg.fiber.dispersion_ps_nm_km, 16.7);
    EXPECT_DOUBLE_EQ(cfg.fiber.gamma_per_W_km, 1.3);
    EXPECT_DOUBLE_EQ(cfg.fiber.span_length_km, 80.0);
    EXPECT_DOUBLE_EQ(cfg.amplifier.noise_figure_dB, 5.0);
    EXPECT_DOUBLE_EQ(cfg.channel.center_frequency_THz, 193.1);
    EXPECT_DOUBLE_EQ(cfg.channel.symbol_rate_GBaud, 32.0);
    EXPECT_DOUBLE_EQ(cfg.channel.channel_spacing_GHz, 50.0);
    EXPECT_EQ(cfg.channel.num_channels, 80);
    EXPECT_DOUBLE_EQ(cfg.channel.launch_power_dBm, 0.0);
    EXPECT_DOUBLE_EQ(cfg.reference_bandwidth_GHz, 12.5);
}

TEST(ConfigTest, OverridePartialFiber) {
    nlohmann::json j = {{"fiber", {{"attenuation_dB_per_km", 0.25}, {"span_length_km", 100.0}}}};
    auto cfg = optiplan::Config::loadFromJson(j);

    // Overridden values
    EXPECT_DOUBLE_EQ(cfg.fiber.attenuation_dB_per_km, 0.25);
    EXPECT_DOUBLE_EQ(cfg.fiber.span_length_km, 100.0);

    // Non-overridden values keep defaults
    EXPECT_DOUBLE_EQ(cfg.fiber.dispersion_ps_nm_km, 16.7);
    EXPECT_DOUBLE_EQ(cfg.fiber.gamma_per_W_km, 1.3);
}

TEST(ConfigTest, OverrideChannel) {
    nlohmann::json j = {{"channel", {{"num_channels", 40}, {"launch_power_dBm", -2.0}}}};
    auto cfg = optiplan::Config::loadFromJson(j);

    EXPECT_EQ(cfg.channel.num_channels, 40);
    EXPECT_DOUBLE_EQ(cfg.channel.launch_power_dBm, -2.0);
    // Defaults preserved
    EXPECT_DOUBLE_EQ(cfg.channel.symbol_rate_GBaud, 32.0);
}

TEST(ConfigTest, FullOverride) {
    nlohmann::json j = {{"fiber",
                         {{"attenuation_dB_per_km", 0.18},
                          {"dispersion_ps_nm_km", 17.0},
                          {"gamma_per_W_km", 1.2},
                          {"span_length_km", 100.0}}},
                        {"amplifier", {{"noise_figure_dB", 4.5}}},
                        {"channel",
                         {{"center_frequency_THz", 193.5},
                          {"symbol_rate_GBaud", 64.0},
                          {"channel_spacing_GHz", 75.0},
                          {"num_channels", 96},
                          {"launch_power_dBm", 1.0}}},
                        {"reference_bandwidth_GHz", 25.0}};
    auto cfg = optiplan::Config::loadFromJson(j);

    EXPECT_DOUBLE_EQ(cfg.fiber.attenuation_dB_per_km, 0.18);
    EXPECT_DOUBLE_EQ(cfg.fiber.dispersion_ps_nm_km, 17.0);
    EXPECT_DOUBLE_EQ(cfg.amplifier.noise_figure_dB, 4.5);
    EXPECT_DOUBLE_EQ(cfg.channel.symbol_rate_GBaud, 64.0);
    EXPECT_EQ(cfg.channel.num_channels, 96);
    EXPECT_DOUBLE_EQ(cfg.reference_bandwidth_GHz, 25.0);
}
