#include <iostream>
#include <cmath>
#include <cstdlib>

#include "model/rates.hpp"

namespace {

constexpr double TOLERANCE = 1e-6;

void assert_true(bool condition, const char* msg) 
{
    if (!condition) {
        std::cerr << "[FAIL] " << msg << "\n";
        std::exit(1);
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

void test_finiteness_over_voltage_sweep()
{
    const double V_min = -100.0;
    const double V_max =  60.0;
    const double dV    =   0.1;

    for (double V = V_min; V <= V_max; V += dV) {
        assert_finite_nonneg(axonhh::rates::alpha_m(V), "alpha_m finiteness");
        assert_finite_nonneg(axonhh::rates::beta_m(V),  "beta_m finiteness");
        assert_finite_nonneg(axonhh::rates::alpha_h(V), "alpha_h finiteness");
        assert_finite_nonneg(axonhh::rates::beta_h(V),  "beta_h finiteness");
        assert_finite_nonneg(axonhh::rates::alpha_n(V), "alpha_n finiteness");
        assert_finite_nonneg(axonhh::rates::beta_n(V),  "beta_n finiteness");
    }
}

void test_singularity_limits();
void test_at_resting_potential();
void test_monotonicity_properties();

} // anonymous namespace to prevent multiple definition errors

int main() {
    // see README.md under test rates section for philosophy on testing
    using namespace axonhh::rates;

    std::cout << "[test_rates] starting\n";

    test_finiteness_over_voltage_sweep();
    test_singularity_limits();
    test_at_resting_potential();
    test_monotonicity_properties();
    
    std::cout << "[test_rates] successful\n";
    return 0;
}

