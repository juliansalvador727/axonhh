#include <iostream>
#include <cmath>
#include <cstdlib>

#include "model/rates.hpp";

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
    const double V1 = -100.0;
    const double V_MAX =  60.0;
    const double DV    =   0.1;

    for (double V = V1; V <= V_MAX; V += DV) {
        assert_finite_nonneg(axonhh::rates::alpha_m(V), "alpha_m finiteness");
        assert_finite_nonneg(axonhh::rates::beta_m(V),  "beta_m finiteness");
        assert_finite_nonneg(axonhh::rates::alpha_h(V), "alpha_h finiteness");
        assert_finite_nonneg(axonhh::rates::beta_h(V),  "beta_h finiteness");
        assert_finite_nonneg(axonhh::rates::alpha_n(V), "alpha_n finiteness");
        assert_finite_nonneg(axonhh::rates::beta_n(V),  "beta_n finiteness");
    }
}

void test_singularity_limits()
{
    const double V_ALPHA_M = -40.0;
    const double V_ALPHA_N = -55.0;

    assert_near(axonhh::rates::alpha_m(V_ALPHA_M), 1.0, TOLERANCE, "alpha_m(-40) singularity limit");
    assert_near(axonhh::rates::alpha_n(V_ALPHA_N), 0.1, TOLERANCE, "alpha_n(-55) singularity limit");
}

void test_at_resting_potential()
{
    const double V_rest = -65.0;

    const double am = axonhh::rates::alpha_m(V_rest);
    const double bm = axonhh::rates::beta_m(V_rest);
    const double ah = axonhh::rates::alpha_h(V_rest);
    const double bh = axonhh::rates::beta_h(V_rest);
    const double an = axonhh::rates::alpha_n(V_rest);
    const double bn = axonhh::rates::beta_n(V_rest);

    assert_finite_nonneg(am, "alpha_m(-65) finite");
    assert_finite_nonneg(bm, "beta_m(-65) finite");
    assert_finite_nonneg(ah, "alpha_h(-65) finite");
    assert_finite_nonneg(bh, "beta_h(-65) finite");
    assert_finite_nonneg(an, "alpha_n(-65) finite");
    assert_finite_nonneg(bn, "beta_n(-65) finite");

    // m-gate: activation mostly closed at rest
    assert_true(am < bm, "alpha_m < beta_m at rest (m mostly closed)");

    // h-gate: inactivation partially open at rest
    assert_true(ah < bh || std::abs(ah - bh) < 1.0,
                "alpha_h and beta_h comparable at rest");

    // n-gate: activation mostly closed at rest
    assert_true(an < bn, "alpha_n < beta_n at rest (n mostly closed)");
}

void test_monotonicity_properties()
{
    const double V1 = -65.0;
    const double V2 = -20.0;

    assert_true(axonhh::rates::alpha_m(V1) < axonhh::rates::alpha_m(V2), "alpha_m increases");
    assert_true(axonhh::rates::beta_m(V1) > axonhh::rates::beta_m(V2), "beta_m decreases");
    assert_true(axonhh::rates::alpha_n(V1) < axonhh::rates::alpha_n(V2), "alpha_n increases");
}

}

int main() {
    // see README.md
    using namespace axonhh::rates;

    std::cout << "[test_rates] starting\n";

    test_finiteness_over_voltage_sweep();
    test_singularity_limits();
    test_at_resting_potential();
    test_monotonicity_properties();
    
    std::cout << "[test_rates] successful\n";
    return 0;
}

