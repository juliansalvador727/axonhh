#include "rates.hpp";

namespace axonhh::rates {
// Sodium activation (m)
double alpha_m(double V_mV) { return 0.1 * vtrap(V_mV + 40.0, 10.0); }
double beta_m(double V_mV) { return 4.0 * exp_safe(-(V_mV + 65.0) / 18.0); }

// Sodium inactivation (h)
double alpha_h(double V_mV) { return 0.07 * exp_safe(-(V_mV + 65.0) / 20.0); }
double beta_h(double V_mV) { return 1.0 / (1.0 + exp_safe(-(V_mV + 35.0) / 10.0)); }

// Potassium activation (n)
double alpha_n(double V_mV) { return 0.01 * vtrap(V_mV + 55.0, 10.0); }
double beta_n(double V_mV) { return 0.125 * exp_safe(-(V_mV + 65.0 ) / 80.0 ); }

// Helpers
// steady-state value x_inf(V) = alpha / (alpha + beta)
double x_inf(double alpha, double beta) { return alpha / (alpha + beta); }
// time constant tau(V) = 1 / (alpha + beta)
double tau(double alpha, double beta) { return 1.0 / (alpha + beta); }

// safe exponential for numerical stability
// clamp inputs to avoid overflow/underflow
double exp_safe(double x) {
    // IEEE 754 limits for double precision
    const double MAX_EXP = 709.0;
    const double MIN_EXP = -708.0;

    if (std::isnan(x)) return x;
    if (x > MAX_EXP) return std::numeric_limits<double>::infinity();
    if (x < MIN_EXP) return 0.0;

    return std::exp(x);
}

// handle expressions of the form x/(1 - exp(-x/k)) with a removable singularity
// useful for alpha_m & alpha_N for when V is near the singular point.
// lets use a second order taylor approximation for good accuracy lol
double vtrap(double x, double k) {

    const double eps = 1e-6;

    if (std::abs(x) < eps) {
        return k + 0.5 * x + (x * x) / (12.0 * k);
    }

    return x / (1.0 - exp_safe(-x/k));
}


}