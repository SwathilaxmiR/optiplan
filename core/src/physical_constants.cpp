/// @file physical_constants.cpp
/// @brief Implementation of inverse Q-function and mathematical helpers.

#include "optiplan/physical_constants.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace optiplan {
namespace units {

double inverseQFunction(double p) {
    if (p <= 0.0 || p >= 1.0) {
        throw std::invalid_argument("inverseQFunction: p must be strictly in (0, 1)");
    }

    // For p > 0.5, use symmetry: Q(-x) = 1 - Q(x) => x = -inverseQFunction(1 - p)
    if (p > 0.5) {
        return -inverseQFunction(1.0 - p);
    }
    if (p == 0.5) {
        return 0.0;
    }

    // Winitzki-based rational initial guess for erfcinv(2*p):
    // Let y = 2*p, we want z = x / sqrt(2) such that erfc(z) = y.
    // Initial approximation using asymptotic expansion:
    const double t = std::sqrt(-2.0 * std::log(p));
    // Coefficients for rational approximation (similar to Abramowitz & Stegun 26.2.23)
    constexpr double c0 = 2.515517;
    constexpr double c1 = 0.802853;
    constexpr double c2 = 0.010328;
    constexpr double d1 = 1.432788;
    constexpr double d2 = 0.189269;
    constexpr double d3 = 0.001308;

    double x = t - ((c2 * t + c1) * t + c0) / (((d3 * t + d2) * t + d1) * t + 1.0);

    // Refine with Halley's method (cubic convergence)
    // f(x) = Q(x) - p
    // f'(x) = -phi(x) = -exp(-x^2 / 2) / sqrt(2 * pi)
    // f''(x) = x * phi(x) = -x * f'(x)
    constexpr double inv_sqrt_2pi = 0.39894228040143267794;
    for (int iter = 0; iter < 4; ++iter) {
        double q_curr = qFunction(x);
        double f = q_curr - p;
        if (std::abs(f) < 1e-15) {
            break;
        }
        double f_prime = -inv_sqrt_2pi * std::exp(-0.5 * x * x);
        if (std::abs(f_prime) < 1e-300) {
            break;
        }
        double f_prime2 = -x * f_prime;
        // Halley step: delta = f / (f_prime - (f * f_prime2) / (2 * f_prime))
        double denom = f_prime - (f * f_prime2) / (2.0 * f_prime);
        x -= f / denom;
    }

    return x;
}

}  // namespace units
}  // namespace optiplan
