#pragma once

#include <cmath>
#include <cfloat>
#include <limits>

namespace axonhh::rates {

/*** 
 * Hodgkin–Huxley rate functions (modern absolute-voltage convention)
 * Units: V in mV, returned rates in 1/ms.
 * These are the classic HH (1952) kinetics rewritten for absolute membrane voltage.
 * Two functions (alpha_m and alpha_n) have removable singularities; the implementation
 * must handle those limits robustly near V = -40 mV and V = -55 mV respectively.
***/

// Sodium activation (m)
double alpha_m(double V_mV);
double beta_m(double V_mV);

// Sodium inactivation (h)
double alpha_h(double V_mV);
double beta_h(double V_mV);

// Potassium activation (n)
double alpha_n(double V_mV);
double beta_n(double V_mV);

/***
 * Helpers
***/

// steady-state value x_inf(V) = alpha / (alpha + beta)
double x_inf(double alpha, double beta);

// time constant tau(V) = 1 / (alpha + beta)
double tau(double alpha, double beta);

// safe exponential for numerical stability
// clamp inputs to avoid overflow/underflow
double exp_safe(double x);

// handle expressions of the form x/(1 - exp(-x/k)) with a removable singularity
// useful for alpha_m & alpha_N for when V is near the singular point.
double vtrap(double x, double k);

} // namespace axonhh::rates