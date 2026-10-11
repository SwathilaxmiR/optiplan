#pragma once

/// @file gn_model.hpp
/// @brief Incoherent Gaussian Noise (GN) physical-layer QoT model.
///
/// Implements the closed-form analytical GN model for Nyquist-WDM systems
/// with multi-span coherence factor epsilon (Poggiolini et al., 2012 / Carena et al., 2014).

#include <vector>

#include "optiplan/qot_model.hpp"

namespace optiplan {

/// Policy for decomposing links into amplifier spans
enum class SpanSplitPolicy {
    EQUAL_SPANS,           ///< Split into ceil(length / span_length) equal spans
    FIXED_PLUS_FRACTIONAL  ///< Spans of span_length with a potentially shorter final span
};

/// Incoherent Gaussian Noise (GN) model implementation.
class GNModel : public IQoTModel {
public:
    /// Construct GNModel with multi-span coherence factor epsilon.
    /// Default epsilon = 0.05 (Poggiolini et al.).
    explicit GNModel(double epsilon = 0.05,
                     SpanSplitPolicy split_policy = SpanSplitPolicy::EQUAL_SPANS);

    /// Get coherence factor epsilon
    double epsilon() const { return epsilon_; }

    /// Set coherence factor epsilon
    void setEpsilon(double eps) { epsilon_ = eps; }

    /// Get span splitting policy
    SpanSplitPolicy splitPolicy() const { return split_policy_; }

    /// Set span splitting policy
    void setSplitPolicy(SpanSplitPolicy policy) { split_policy_ = policy; }

    /// Evaluate path QoT under the GN model.
    QoTResult evaluatePath(const std::vector<LinkSegment>& path_segments, double launch_power_dBm,
                           const Config& config) const override;

    /// Calculate span lengths for a given link length
    std::vector<double> getSpanLengths(double link_length_km, double max_span_length_km) const;

    /// Compute beta2 dispersion parameter [s^2/m] from dispersion D [ps/(nm*km)] and frequency nu
    /// [Hz]
    static double computeBeta2(double dispersion_ps_nm_km, double center_frequency_Hz);

    /// Compute linear attenuation coefficient [1/m] from alpha [dB/km]
    static double computeAlphaLinear(double attenuation_dB_per_km);

private:
    double epsilon_;
    SpanSplitPolicy split_policy_;
};

}  // namespace optiplan
