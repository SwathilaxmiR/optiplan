/// @file gn_model.cpp
/// @brief Implementation of the Incoherent Gaussian Noise (GN) physical-layer model.

#include "optiplan/gn_model.hpp"

#include <cmath>
#include <numeric>
#include <stdexcept>

#include "optiplan/physical_constants.hpp"

namespace optiplan {

GNModel::GNModel(double epsilon, SpanSplitPolicy split_policy)
    : epsilon_(epsilon), split_policy_(split_policy) {}

double GNModel::computeBeta2(double dispersion_ps_nm_km, double center_frequency_Hz) {
    // D [ps/(nm*km)] -> D_SI [s/m^2]
    // 1 ps = 1e-12 s, 1 nm = 1e-9 m, 1 km = 1e3 m => 1 ps/(nm*km) = 1e-6 s/m^2
    const double D_si = dispersion_ps_nm_km * 1e-6;
    const double lambda = constants::SPEED_OF_LIGHT / center_frequency_Hz;
    // beta2 = - (lambda^2 / (2 * pi * c)) * D_si
    return -(lambda * lambda / (2.0 * constants::PI * constants::SPEED_OF_LIGHT)) * D_si;
}

double GNModel::computeAlphaLinear(double attenuation_dB_per_km) {
    // alpha_dB = 10 * log10(e) * alpha_linear_km
    // alpha_linear_km = alpha_dB / (10 * log10(e)) = alpha_dB * ln(10) / 10
    // alpha_linear_m = alpha_linear_km / 1000
    constexpr double ln10_over_10 = 0.2302585092994045684;  // ln(10)/10
    return attenuation_dB_per_km * ln10_over_10 * 1e-3;
}

std::vector<double> GNModel::getSpanLengths(double link_length_km,
                                            double max_span_length_km) const {
    if (link_length_km <= 0.0) {
        return {};
    }
    if (max_span_length_km <= 0.0) {
        throw std::invalid_argument("max_span_length_km must be positive");
    }

    int num_spans = static_cast<int>(std::ceil(link_length_km / max_span_length_km));
    if (num_spans <= 0)
        num_spans = 1;

    std::vector<double> spans;
    spans.reserve(num_spans);

    if (split_policy_ == SpanSplitPolicy::EQUAL_SPANS) {
        double span_len = link_length_km / static_cast<double>(num_spans);
        for (int i = 0; i < num_spans; ++i) {
            spans.push_back(span_len);
        }
    } else {
        // FIXED_PLUS_FRACTIONAL
        double remaining = link_length_km;
        for (int i = 0; i < num_spans - 1; ++i) {
            spans.push_back(max_span_length_km);
            remaining -= max_span_length_km;
        }
        spans.push_back(remaining);
    }
    return spans;
}

QoTResult GNModel::evaluatePath(const std::vector<LinkSegment>& path_segments,
                                double launch_power_dBm, const Config& config) const {
    QoTResult result;
    if (path_segments.empty()) {
        return result;
    }

    // Physical and grid parameters in SI units
    const double nu = config.channel.center_frequency_THz * 1e12;             // [Hz]
    const double B_ch = config.channel.symbol_rate_GBaud * 1e9;               // [Hz]
    const double B_ref = config.reference_bandwidth_GHz * 1e9;                // [Hz]
    const double channel_spacing = config.channel.channel_spacing_GHz * 1e9;  // [Hz]
    const double h = constants::PLANCK_CONSTANT;                              // [J*s]
    const double nf_lin = units::dBToLinear(config.amplifier.noise_figure_dB);

    const double alpha_lin = computeAlphaLinear(config.fiber.attenuation_dB_per_km);  // [1/m]
    const double beta2 = computeBeta2(config.fiber.dispersion_ps_nm_km, nu);          // [s^2/m]
    const double abs_beta2 = std::abs(beta2);
    const double gamma_si = config.fiber.gamma_per_W_km * 1e-3;  // [1/(W*m)]
    const double L_eff_a = 1.0 / alpha_lin;                      // [m]

    double total_p_ase = 0.0;
    double total_p_ase_ref = 0.0;
    double eta_sum_intermediate = 0.0;
    int span_count = 0;

    const double inv_one_plus_eps = 1.0 / (1.0 + epsilon_);

    for (const auto& segment : path_segments) {
        if (segment.length_km <= 0.0)
            continue;

        auto span_lengths_km = getSpanLengths(segment.length_km, config.fiber.span_length_km);
        int active_channels = std::max(1, segment.occupied_channels);
        double B_wdm = active_channels * channel_spacing;  // [Hz]

        for (double span_len_km : span_lengths_km) {
            span_count++;
            const double L_span_m = span_len_km * 1e3;
            const double G_span = std::exp(alpha_lin * L_span_m);

            // ASE noise per span
            const double p_ase_span = nf_lin * h * nu * G_span * B_ch;
            const double p_ase_span_ref = nf_lin * h * nu * G_span * B_ref;
            total_p_ase += p_ase_span;
            total_p_ase_ref += p_ase_span_ref;

            // NLI coefficient per span: eta_span [1/W^2]
            // L_eff = (1 - exp(-alpha * L)) / alpha
            const double L_eff = (1.0 - std::exp(-alpha_lin * L_span_m)) / alpha_lin;

            // Argument of asinh: (pi^2 / 2) * |beta2| * L_eff_a * B_wdm^2
            const double asinh_arg =
                (constants::PI * constants::PI * 0.5) * abs_beta2 * L_eff_a * (B_wdm * B_wdm);
            const double denom = constants::PI * abs_beta2 * L_eff_a;

            // eta_span = (8/27) * (gamma^2 * L_eff^2 / B_ch^2) * asinh(...) / (pi * |beta2| *
            // L_eff_a)
            const double eta_span = (8.0 / 27.0) * (gamma_si * gamma_si) * (L_eff * L_eff) /
                                    (B_ch * B_ch) * (std::asinh(asinh_arg) / denom);

            if (epsilon_ == 0.0) {
                eta_sum_intermediate += eta_span;
            } else {
                eta_sum_intermediate += std::pow(eta_span, inv_one_plus_eps);
            }
        }
    }

    result.total_spans = span_count;
    result.p_ase_w = total_p_ase;

    if (span_count == 0) {
        return result;
    }

    // Total NLI coefficient across spans
    double total_eta = 0.0;
    if (epsilon_ == 0.0) {
        total_eta = eta_sum_intermediate;
    } else {
        total_eta = std::pow(eta_sum_intermediate, 1.0 + epsilon_);
    }
    result.eta_nli = total_eta;

    // Power calculations
    const double P_ch = units::dBmToWatts(launch_power_dBm);
    const double P_nli = total_eta * (P_ch * P_ch * P_ch);
    result.p_nli_w = P_nli;

    // SNR and OSNR
    const double total_noise = total_p_ase + P_nli;
    if (total_noise > 0.0) {
        result.snr_linear = P_ch / total_noise;
        result.snr_db = units::linearTodB(result.snr_linear);
    }

    if (total_p_ase_ref > 0.0) {
        result.osnr_linear = P_ch / total_p_ase_ref;
        result.osnr_db = units::linearTodB(result.osnr_linear);
    }

    // Optimum launch power: P_opt = (P_ase / (2 * eta))^(1/3)
    if (total_eta > 0.0 && total_p_ase > 0.0) {
        result.p_opt_w = std::cbrt(total_p_ase / (2.0 * total_eta));
        result.p_opt_dbm = units::wattsTodBm(result.p_opt_w);
    }

    return result;
}

}  // namespace optiplan
