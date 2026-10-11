#pragma once

/// @file modulation.hpp
/// @brief Modulation format hierarchy and analytical BER / required SNR derivations.

#include <memory>
#include <string>
#include <vector>

namespace optiplan {

/// Abstract base class for optical modulation formats.
class ModulationFormat {
public:
    virtual ~ModulationFormat() = default;

    /// Name of the modulation format (e.g. "DP-QPSK")
    virtual std::string name() const = 0;

    /// Total bits per symbol across both polarizations
    virtual int bitsPerSymbol() const = 0;

    /// Net bit rate at 32 GBaud [Gb/s]
    virtual double netBitRateGbps() const = 0;

    /// Calculate pre-FEC Bit Error Rate (BER) from linear SNR in signal bandwidth
    virtual double berFromSnr(double snr_linear) const = 0;

    /// Analytical/numerical derivation of required linear SNR for a pre-FEC BER limit
    /// with an optional design margin [dB].
    virtual double requiredSnrLinear(double pre_fec_ber, double margin_dB = 0.0) const = 0;

    /// Required SNR in decibels [dB]
    double requiredSnrDb(double pre_fec_ber, double margin_dB = 0.0) const;

    /// Convenience helper to solve for required SNR using bisection on berFromSnr
    double solveRequiredSnrBisection(double target_ber, double initial_guess) const;
};

/// Dual-Polarization Binary Phase Shift Keying (DP-BPSK)
class DP_BPSK : public ModulationFormat {
public:
    std::string name() const override { return "DP-BPSK"; }
    int bitsPerSymbol() const override { return 2; }
    double netBitRateGbps() const override { return 50.0; }
    double berFromSnr(double snr_linear) const override;
    double requiredSnrLinear(double pre_fec_ber, double margin_dB = 0.0) const override;
};

/// Dual-Polarization Quadrature Phase Shift Keying (DP-QPSK)
class DP_QPSK : public ModulationFormat {
public:
    std::string name() const override { return "DP-QPSK"; }
    int bitsPerSymbol() const override { return 4; }
    double netBitRateGbps() const override { return 100.0; }
    double berFromSnr(double snr_linear) const override;
    double requiredSnrLinear(double pre_fec_ber, double margin_dB = 0.0) const override;
};

/// Dual-Polarization 8-ary Quadrature Amplitude Modulation (DP-8QAM)
class DP_8QAM : public ModulationFormat {
public:
    std::string name() const override { return "DP-8QAM"; }
    int bitsPerSymbol() const override { return 6; }
    double netBitRateGbps() const override { return 150.0; }
    double berFromSnr(double snr_linear) const override;
    double requiredSnrLinear(double pre_fec_ber, double margin_dB = 0.0) const override;
};

/// Dual-Polarization 16-ary Quadrature Amplitude Modulation (DP-16QAM)
class DP_16QAM : public ModulationFormat {
public:
    std::string name() const override { return "DP-16QAM"; }
    int bitsPerSymbol() const override { return 8; }
    double netBitRateGbps() const override { return 200.0; }
    double berFromSnr(double snr_linear) const override;
    double requiredSnrLinear(double pre_fec_ber, double margin_dB = 0.0) const override;
};

/// Returns all standard supported modulation formats ordered from lowest to highest bitrate
std::vector<std::shared_ptr<ModulationFormat>> getStandardModulationFormats();

}  // namespace optiplan
