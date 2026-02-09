#include <iostream>
#include <cmath>
#include <cstdio>

#include "model/rates.hpp"

namespace {

constexpr double TOLERANCE = 1e-6;

void assert_true(bool condition, const char* msg) 
{
    if (!condition) {
        std::cerr << "[FAIL] " << msg << "\n";
    }
}
void assert_near(double alpha, double beta, double tolerance, const char* msg) 
{
    if (std::abs(alpha - beta) > tolerance) {
        std::cerr << "[FAIL] " << msg << " (got " << alpha << ", expected " << beta << ")\n";
        std::exit(1);
    }
}

void assert_finite_nonneg(double x, const char* msg)
{
    if (!std::isfinite(x)) {
        std::cerr << "[FAIL] " << msg << " (non-finite)\n";
        std::exit(1);
    }

    if (x < 0.0) {
        std::cerr << "[FAIL] " << msg << " (negative)\n";
        std::exit(1);
    }
}

} // anonymous namespac to prevent multiple definition errors

int main() {
    using namespace axonhh::rates;
    return 0;
}

/* TODO: Implement this shit

TEST 1: Finiteness over a voltage sweep
sweep over voltage for [-100mV, +60mV]
for each voltage assert
value is finite
value is not Nan
value is >= 0
to catch division by zero, exp overflow, sign errors and if vtrap is broken

TEST 2: correct signularity limits
specifically check alpha_m(-40) = 1.0 and alpha_n(-55) = 0.1 with a tolerance of 1e-6.
if this succeeds vtrap, voltage shifts and constants are validated.

TEST 3: (VISUAL)
at v = -65 mV check rough expectations of the following:
alpha_m is small
beta_m is large
alpha_h is moderate
beta_h is moderate
alpha_n is small
beta_n is moderate

TEST 4: Monotonicitry
alpha_m(V) should increase with V
beta_m(V) should decrease with V
alpha_n(V) should increase with V
for each of these pick two voltages and assert ordering.
*/