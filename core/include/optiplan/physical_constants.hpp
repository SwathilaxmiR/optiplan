#pragma once

/// @file physical_constants.hpp
/// @brief Fundamental physical constants and unit conversion utilities.

#include <cmath>

namespace optiplan {
namespace constants {

/// Speed of light in vacuum [m/s]
constexpr double SPEED_OF_LIGHT = 299792458.0;

/// Planck constant [J*s]
constexpr double PLANCK_CONSTANT = 6.62607015e-34;

/// Pi
constexpr double PI = 3.14159265358979323846;

}  // namespace constants

namespace units {

/// Convert power from dBm to Watts [W]
inline double dBmToWatts(double dBm) {
    return std::pow(10.0, (dBm - 30.0) / 10.0);
}

/// Convert power from Watts [W] to dBm
inline double wattsTodBm(double watts) {
    return 10.0 * std::log10(watts) + 30.0;
}

/// Convert linear ratio to decibels [dB]
inline double linearTodB(double linear) {
    return 10.0 * std::log10(linear);
}

/// Convert decibels [dB] to linear ratio
inline double dBToLinear(double dB) {
    return std::pow(10.0, dB / 10.0);
}

/// Standard Gaussian Q-function: Q(x) = 0.5 * erfc(x / sqrt(2))
inline double qFunction(double x) {
    return 0.5 * std::erfc(x / std::sqrt(2.0));
}

/// Inverse Q-function: returns x such that Q(x) = p.
/// Uses rational approximation with Halley/Newton-Raphson refinement.
double inverseQFunction(double p);

}  // namespace units
}  // namespace optiplan
