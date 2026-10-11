/// @file test_modulation.cpp
/// @brief Unit tests for modulation formats, BER formulas, and required SNR derivations.

#include <gtest/gtest.h>

#include <cmath>

#include "optiplan/modulation.hpp"
#include "optiplan/physical_constants.hpp"

using namespace optiplan;

TEST(ModulationTest, FormatProperties) {
    DP_BPSK bpsk;
    DP_QPSK qpsk;
    DP_8QAM qam8;
    DP_16QAM qam16;

    EXPECT_EQ(bpsk.name(), "DP-BPSK");
    EXPECT_EQ(bpsk.bitsPerSymbol(), 2);
    EXPECT_DOUBLE_EQ(bpsk.netBitRateGbps(), 50.0);

    EXPECT_EQ(qpsk.name(), "DP-QPSK");
    EXPECT_EQ(qpsk.bitsPerSymbol(), 4);
    EXPECT_DOUBLE_EQ(qpsk.netBitRateGbps(), 100.0);

    EXPECT_EQ(qam8.name(), "DP-8QAM");
    EXPECT_EQ(qam8.bitsPerSymbol(), 6);
    EXPECT_DOUBLE_EQ(qam8.netBitRateGbps(), 150.0);

    EXPECT_EQ(qam16.name(), "DP-16QAM");
    EXPECT_EQ(qam16.bitsPerSymbol(), 8);
    EXPECT_DOUBLE_EQ(qam16.netBitRateGbps(), 200.0);
}

TEST(ModulationTest, StandardFormatsList) {
    auto formats = getStandardModulationFormats();
    ASSERT_EQ(formats.size(), 4u);
    EXPECT_EQ(formats[0]->name(), "DP-BPSK");
    EXPECT_EQ(formats[1]->name(), "DP-QPSK");
    EXPECT_EQ(formats[2]->name(), "DP-8QAM");
    EXPECT_EQ(formats[3]->name(), "DP-16QAM");
}

TEST(ModulationTest, AnalyticalBerKnownValues) {
    DP_BPSK bpsk;
    DP_QPSK qpsk;

    // At SNR = 10.0 linear (~10 dB)
    // QPSK: BER = Q(sqrt(10)) = Q(3.16227766) ~= 7.827e-4
    double qpsk_ber = qpsk.berFromSnr(10.0);
    EXPECT_NEAR(qpsk_ber, 7.827e-4, 1e-6);

    // BPSK: BER = Q(sqrt(20)) = Q(4.472136) ~= 3.872e-6
    double bpsk_ber = bpsk.berFromSnr(10.0);
    EXPECT_NEAR(bpsk_ber, 3.872e-6, 1e-8);
}

TEST(ModulationTest, OrderAndThresholdsAtStandardBer) {
    // Pre-FEC BER threshold of 1e-2 (standard ~7% or ~20% HD/SD-FEC limit)
    constexpr double pre_fec_ber = 0.01;
    DP_BPSK bpsk;
    DP_QPSK qpsk;
    DP_8QAM qam8;
    DP_16QAM qam16;

    double snr_bpsk = bpsk.requiredSnrDb(pre_fec_ber);
    double snr_qpsk = qpsk.requiredSnrDb(pre_fec_ber);
    double snr_8qam = qam8.requiredSnrDb(pre_fec_ber);
    double snr_16qam = qam16.requiredSnrDb(pre_fec_ber);

    // Monotonic ordering: higher rate requires higher SNR
    EXPECT_LT(snr_bpsk, snr_qpsk);
    EXPECT_LT(snr_qpsk, snr_8qam);
    EXPECT_LT(snr_8qam, snr_16qam);

    // QPSK must be exactly 3 dB higher than BPSK at symbol level
    EXPECT_NEAR(snr_qpsk - snr_bpsk, 3.0103, 1e-3);

    // Verify expected published literature ranges at BER = 1e-2:
    // BPSK: ~4.3 dB, QPSK: ~7.3 dB, 8-QAM: ~10.4 dB, 16-QAM: ~12.7 dB
    EXPECT_NEAR(snr_bpsk, 4.32, 0.1);
    EXPECT_NEAR(snr_qpsk, 7.33, 0.1);
    EXPECT_NEAR(snr_8qam, 10.41, 0.2);
    EXPECT_NEAR(snr_16qam, 12.71, 0.2);
}

TEST(ModulationTest, MarginShiftsThresholdByExactDb) {
    DP_QPSK qpsk;
    constexpr double pre_fec_ber = 0.01;

    double snr_0 = qpsk.requiredSnrDb(pre_fec_ber, 0.0);
    double snr_1 = qpsk.requiredSnrDb(pre_fec_ber, 1.0);
    double snr_2 = qpsk.requiredSnrDb(pre_fec_ber, 2.0);

    EXPECT_NEAR(snr_1 - snr_0, 1.0, 1e-9);
    EXPECT_NEAR(snr_2 - snr_0, 2.0, 1e-9);
}

TEST(ModulationTest, BisectionMatchesAnalyticalInversion) {
    constexpr double pre_fec_ber = 0.01;
    DP_16QAM qam16;

    double analytical_lin = qam16.requiredSnrLinear(pre_fec_ber, 0.0);
    double bisection_lin = qam16.solveRequiredSnrBisection(pre_fec_ber, 20.0);

    EXPECT_NEAR(analytical_lin, bisection_lin, 1e-4);
    // Double check that plugging into berFromSnr yields exactly 1e-2
    EXPECT_NEAR(qam16.berFromSnr(analytical_lin), pre_fec_ber, 1e-8);
}
