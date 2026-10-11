/// @file test_gn_model.cpp
/// @brief Physics verification suite for GNModel against hand calculations and published
/// benchmarks.

#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include "optiplan/config.hpp"
#include "optiplan/gn_model.hpp"
#include "optiplan/physical_constants.hpp"

using namespace optiplan;

TEST(GNModelTest, PhysicalParametersDerivation) {
    // Standard SSMF values: D = 16.7 ps/(nm*km) at 193.1 THz (~1552.5 nm)
    double nu = 193.1e12;
    double beta2 = GNModel::computeBeta2(16.7, nu);

    // Expected beta2 ~ -2.13e-26 s^2/m (-21.3 ps^2/km)
    EXPECT_LT(beta2, 0.0);                   // Anomalous dispersion in C-band
    double beta2_ps2_per_km = beta2 * 1e27;  // s^2/m -> ps^2/km: * 1e24 ps^2/s^2 * 1e3 m/km
    EXPECT_NEAR(beta2_ps2_per_km, -21.34, 0.1);

    // Attenuation: 0.2 dB/km -> linear alpha [1/m]
    double alpha = GNModel::computeAlphaLinear(0.2);
    // 0.2 / 4.343 * 1e-3 ~ 4.605e-5 m^-1
    EXPECT_NEAR(alpha, 4.605e-5, 1e-7);
}

TEST(GNModelTest, SingleSpanHandCalculation) {
    Config cfg;  // Default SSMF, 80 km span, NF = 5 dB, 0 dBm launch power
    GNModel model(0.05);

    std::vector<LinkSegment> path = {{80.0, 1}};  // 1 span of 80 km, 1 active channel
    auto res = model.evaluatePath(path, 0.0, cfg);

    ASSERT_EQ(res.total_spans, 1);

    // Hand calculation:
    // G = 10^(0.2 * 80 / 10) = 10^1.6 = 39.810717
    // NF_lin = 10^0.5 = 3.162278
    // nu = 193.1e12 Hz, h = 6.62607015e-34 J*s
    // B_ch = 32e9 Hz
    // P_ASE = NF_lin * h * nu * G * B_ch ~= 5.1578e-7 W
    double expected_p_ase = 5.1578e-7;
    EXPECT_NEAR(res.p_ase_w, expected_p_ase, 2e-9);

    // OSNR in 12.5 GHz:
    // B_ref = 12.5e9 Hz
    // P_ASE_ref = NF_lin * h * nu * G * B_ref ~= 2.0148e-7 W
    // OSNR_lin = 1e-3 / 2.0148e-7 ~= 4963.3 => OSNR_dB ~= 36.96 dB
    EXPECT_NEAR(res.osnr_db, 36.96, 0.05);

    // NLI check: P_ch = 1 mW = 1e-3 W.
    // NLI noise is positive and smaller than ASE for 0 dBm on a single span
    EXPECT_GT(res.p_nli_w, 0.0);
    EXPECT_LT(res.p_nli_w, res.p_ase_w);
}

TEST(GNModelTest, OptimumLaunchPowerPeak) {
    Config cfg;
    GNModel model(0.05);

    // Multi-span path (3 spans = 240 km, 40 active channels)
    std::vector<LinkSegment> path = {{240.0, 40}};
    auto ref_res = model.evaluatePath(path, 0.0, cfg);

    double p_opt_dbm = ref_res.p_opt_dbm;
    auto opt_res = model.evaluatePath(path, p_opt_dbm, cfg);

    // Property: At optimum launch power, P_nli = 0.5 * P_ase (3 dB lower than ASE)
    EXPECT_NEAR(opt_res.p_nli_w / opt_res.p_ase_w, 0.5, 1e-3);

    // Sweep powers around P_opt and verify SNR is strictly maximized at P_opt
    double max_snr = opt_res.snr_linear;
    for (double delta_db = -5.0; delta_db <= 5.0; delta_db += 0.5) {
        if (std::abs(delta_db) < 0.01)
            continue;
        auto sweep_res = model.evaluatePath(path, p_opt_dbm + delta_db, cfg);
        EXPECT_LE(sweep_res.snr_linear, max_snr);
    }
}

TEST(GNModelTest, MonotonicityWithSpansAndChannels) {
    Config cfg;
    GNModel model(0.05);

    // 1. Monotonicity with span count: longer path -> strictly lower SNR
    auto res_1_span = model.evaluatePath({{80.0, 10}}, 0.0, cfg);
    auto res_3_spans = model.evaluatePath({{240.0, 10}}, 0.0, cfg);
    auto res_10_spans = model.evaluatePath({{800.0, 10}}, 0.0, cfg);

    EXPECT_GT(res_1_span.snr_db, res_3_spans.snr_db);
    EXPECT_GT(res_3_spans.snr_db, res_10_spans.snr_db);
    EXPECT_GT(res_10_spans.p_ase_w, res_3_spans.p_ase_w);
    EXPECT_GT(res_3_spans.p_ase_w, res_1_span.p_ase_w);

    // 2. Monotonicity with channel count: more channels -> strictly higher NLI -> lower SNR
    auto res_1_ch = model.evaluatePath({{240.0, 1}}, 0.0, cfg);
    auto res_20_ch = model.evaluatePath({{240.0, 20}}, 0.0, cfg);
    auto res_80_ch = model.evaluatePath({{240.0, 80}}, 0.0, cfg);

    // ASE is independent of channel count on other channels
    EXPECT_NEAR(res_1_ch.p_ase_w, res_80_ch.p_ase_w, 1e-15);

    // NLI grows with occupied bandwidth
    EXPECT_LT(res_1_ch.p_nli_w, res_20_ch.p_nli_w);
    EXPECT_LT(res_20_ch.p_nli_w, res_80_ch.p_nli_w);

    // Total SNR drops as more channels are added
    EXPECT_GT(res_1_ch.snr_db, res_20_ch.snr_db);
    EXPECT_GT(res_20_ch.snr_db, res_80_ch.snr_db);
}

TEST(GNModelTest, CoherenceFactorEffect) {
    Config cfg;
    // model_incoherent (epsilon = 0.0) vs model_coherent (epsilon = 0.05)
    GNModel model_eps0(0.0);
    GNModel model_eps005(0.05);

    std::vector<LinkSegment> multi_span = {{400.0, 40}};  // 5 spans
    auto res_eps0 = model_eps0.evaluatePath(multi_span, 0.0, cfg);
    auto res_eps005 = model_eps005.evaluatePath(multi_span, 0.0, cfg);

    // ASE is identical
    EXPECT_DOUBLE_EQ(res_eps0.p_ase_w, res_eps005.p_ase_w);

    // Multi-span with epsilon > 0 accumulates NLI slightly faster (N_s^1.05 vs N_s^1.0)
    EXPECT_GT(res_eps005.p_nli_w, res_eps0.p_nli_w);
    EXPECT_LT(res_eps005.snr_db, res_eps0.snr_db);
}
