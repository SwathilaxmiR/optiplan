/// @file modulation.cpp
/// @brief Implementation of BER calculations and required SNR derivations.

#include "optiplan/modulation.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "optiplan/physical_constants.hpp"

namespace optiplan {

double ModulationFormat::requiredSnrDb(double pre_fec_ber, double margin_dB) const {
    double req_lin = requiredSnrLinear(pre_fec_ber, margin_dB);
    return units::linearTodB(req_lin);
}

double ModulationFormat::solveRequiredSnrBisection(double target_ber, double initial_guess) const {
    if (target_ber <= 0.0 || target_ber >= 0.5) {
        throw std::invalid_argument("target_ber must be in (0, 0.5)");
    }

    double low = initial_guess > 0.0 ? initial_guess * 0.1 : 0.1;
    double high = initial_guess > 0.0 ? initial_guess * 10.0 : 100.0;

    // Expand search bounds if needed
    while (berFromSnr(low) < target_ber && low > 1e-6) {
        low *= 0.5;
    }
    while (berFromSnr(high) > target_ber && high < 1e6) {
        high *= 2.0;
    }

    for (int iter = 0; iter < 100; ++iter) {
        double mid = 0.5 * (low + high);
        double ber = berFromSnr(mid);
        double err = ber - target_ber;

        if (std::abs(err) < 1e-12 || (high - low) / mid < 1e-9) {
            return mid;
        }

        if (ber > target_ber) {
            // Need higher SNR to reduce BER
            low = mid;
        } else {
            high = mid;
        }
    }

    return 0.5 * (low + high);
}

// ─── DP-BPSK ─────────────────────────────────────────────────────────────────
// BER = Q(sqrt(2 * SNR))
double DP_BPSK::berFromSnr(double snr_linear) const {
    if (snr_linear <= 0.0)
        return 0.5;
    return units::qFunction(std::sqrt(2.0 * snr_linear));
}

double DP_BPSK::requiredSnrLinear(double pre_fec_ber, double margin_dB) const {
    if (pre_fec_ber <= 0.0 || pre_fec_ber >= 0.5) {
        throw std::invalid_argument("pre_fec_ber must be in (0, 0.5)");
    }
    // Q(sqrt(2 * SNR)) = BER => sqrt(2 * SNR) = Q^-1(BER) => SNR = 0.5 * (Q^-1(BER))^2
    double q_inv = units::inverseQFunction(pre_fec_ber);
    double base_snr = 0.5 * q_inv * q_inv;
    return base_snr * units::dBToLinear(margin_dB);
}

// ─── DP-QPSK ─────────────────────────────────────────────────────────────────
// BER = Q(sqrt(SNR))
double DP_QPSK::berFromSnr(double snr_linear) const {
    if (snr_linear <= 0.0)
        return 0.5;
    return units::qFunction(std::sqrt(snr_linear));
}

double DP_QPSK::requiredSnrLinear(double pre_fec_ber, double margin_dB) const {
    if (pre_fec_ber <= 0.0 || pre_fec_ber >= 0.5) {
        throw std::invalid_argument("pre_fec_ber must be in (0, 0.5)");
    }
    // Q(sqrt(SNR)) = BER => sqrt(SNR) = Q^-1(BER) => SNR = (Q^-1(BER))^2
    double q_inv = units::inverseQFunction(pre_fec_ber);
    double base_snr = q_inv * q_inv;
    return base_snr * units::dBToLinear(margin_dB);
}

// ─── DP-8QAM ─────────────────────────────────────────────────────────────────
// Gray-coded rectangular 8-QAM approximation: BER ~= (2/3) * Q(sqrt(3/7 * SNR))
double DP_8QAM::berFromSnr(double snr_linear) const {
    if (snr_linear <= 0.0)
        return 0.5;
    return (2.0 / 3.0) * units::qFunction(std::sqrt((3.0 / 7.0) * snr_linear));
}

double DP_8QAM::requiredSnrLinear(double pre_fec_ber, double margin_dB) const {
    if (pre_fec_ber <= 0.0 || pre_fec_ber >= 0.5) {
        throw std::invalid_argument("pre_fec_ber must be in (0, 0.5)");
    }
    // Analytical inversion:
    // (2/3) * Q(sqrt(3/7 * SNR)) = BER => Q(...) = 1.5 * BER
    double target_q = 1.5 * pre_fec_ber;
    double base_snr;
    if (target_q < 0.5) {
        double q_inv = units::inverseQFunction(target_q);
        base_snr = (7.0 / 3.0) * q_inv * q_inv;
    } else {
        base_snr = solveRequiredSnrBisection(pre_fec_ber, 10.0);
    }
    return base_snr * units::dBToLinear(margin_dB);
}

// ─── DP-16QAM ────────────────────────────────────────────────────────────────
// Square Gray-coded 16-QAM: BER ~= (3/8) * Q(sqrt(SNR / 5))
double DP_16QAM::berFromSnr(double snr_linear) const {
    if (snr_linear <= 0.0)
        return 0.5;
    return (3.0 / 8.0) * units::qFunction(std::sqrt(snr_linear / 5.0));
}

double DP_16QAM::requiredSnrLinear(double pre_fec_ber, double margin_dB) const {
    if (pre_fec_ber <= 0.0 || pre_fec_ber >= 0.5) {
        throw std::invalid_argument("pre_fec_ber must be in (0, 0.5)");
    }
    // Analytical inversion:
    // (3/8) * Q(sqrt(SNR / 5)) = BER => Q(...) = (8/3) * BER
    double target_q = (8.0 / 3.0) * pre_fec_ber;
    double base_snr;
    if (target_q < 0.5) {
        double q_inv = units::inverseQFunction(target_q);
        base_snr = 5.0 * q_inv * q_inv;
    } else {
        base_snr = solveRequiredSnrBisection(pre_fec_ber, 20.0);
    }
    return base_snr * units::dBToLinear(margin_dB);
}

std::vector<std::shared_ptr<ModulationFormat>> getStandardModulationFormats() {
    return {std::make_shared<DP_BPSK>(), std::make_shared<DP_QPSK>(), std::make_shared<DP_8QAM>(),
            std::make_shared<DP_16QAM>()};
}

}  // namespace optiplan
