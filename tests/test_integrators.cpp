#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "core/types.hpp"
#include "model/hodgkinhuxley.hpp"
#include "numerics/integrator.hpp"

namespace {

constexpr double TOLERANCE = 1e-12;

void assert_true(bool condition, const char* msg)
{
    if (!condition) {
        std::cerr << "[FAIL] " << msg << "\n";
        std::exit(1);
    }
}

void assert_near(double got, double expected, double tolerance, const char* msg)
{
    if (std::abs(got - expected) > tolerance) {
        std::cerr << "[FAIL] " << msg << " (got " << got << ", expected " << expected << ")\n";
        std::exit(1);
    }
}

void assert_state_near(const axonhh::State& got, const axonhh::State& expected, double tolerance, const char* msg)
{
    assert_near(got.V_mV, expected.V_mV, tolerance, msg);
    assert_near(got.m, expected.m, tolerance, msg);
    assert_near(got.h, expected.h, tolerance, msg);
    assert_near(got.n, expected.n, tolerance, msg);
}

void assert_state_finite(const axonhh::State& x, const char* msg)
{
    if (!std::isfinite(x.V_mV) || !std::isfinite(x.m) || !std::isfinite(x.h) || !std::isfinite(x.n)) {
        std::cerr << "[FAIL] " << msg << " (non-finite state)\n";
        std::exit(1);
    }
}

void test_zero_rhs_invariance()
{
    const axonhh::State x0{-65.0, 0.1, 0.6, 0.3};
    const double t0 = 0.0;
    const double dt = 0.05;

    const axonhh::numerics::RHSFn rhs = [](double, const axonhh::State&) {
        return axonhh::Deriv{0.0, 0.0, 0.0, 0.0};
    };

    auto euler = axonhh::numerics::make_integrator(axonhh::IntegratorKind::Euler);
    auto rk4 = axonhh::numerics::make_integrator(axonhh::IntegratorKind::RK4);

    const axonhh::State xe = euler->step(rhs, t0, x0, dt);
    const axonhh::State xr = rk4->step(rhs, t0, x0, dt);

    assert_state_near(xe, x0, TOLERANCE, "Euler preserves state for zero RHS");
    assert_state_near(xr, x0, TOLERANCE, "RK4 preserves state for zero RHS");
}

void test_constant_rhs_exactness_single_step()
{
    const axonhh::State x0{-60.0, 0.2, 0.4, 0.1};
    const axonhh::Deriv c{3.0, -0.2, 0.1, 0.05};

    const double t0 = 1.0;
    const double dt = 0.2;

    const axonhh::State expected{
        x0.V_mV + dt * c.dV_dt,
        x0.m + dt * c.dm_dt,
        x0.h + dt * c.dh_dt,
        x0.n + dt * c.dn_dt,
    };

    const axonhh::numerics::RHSFn rhs = [c](double, const axonhh::State&) { return c; };

    auto euler = axonhh::numerics::make_integrator(axonhh::IntegratorKind::Euler);
    auto rk4 = axonhh::numerics::make_integrator(axonhh::IntegratorKind::RK4);

    const axonhh::State xe = euler->step(rhs, t0, x0, dt);
    const axonhh::State xr = rk4->step(rhs, t0, x0, dt);

    assert_state_near(xe, expected, TOLERANCE, "Euler exact for constant RHS (single step)");
    assert_state_near(xr, expected, TOLERANCE, "RK4 exact for constant RHS (single step)");
}

void test_rk4_more_accurate_than_euler_for_exponential_growth()
{
    const axonhh::State x0{1.0, 1.0, 1.0, 1.0};
    const double a = 1.0;

    const double t0 = 0.0;
    const double dt = 1.0;

    const axonhh::numerics::RHSFn rhs = [a](double, const axonhh::State& x) {
        return axonhh::Deriv{a * x.V_mV, a * x.m, a * x.h, a * x.n};
    };

    const double exact = std::exp(a * dt);

    auto euler = axonhh::numerics::make_integrator(axonhh::IntegratorKind::Euler);
    auto rk4 = axonhh::numerics::make_integrator(axonhh::IntegratorKind::RK4);

    const axonhh::State xe = euler->step(rhs, t0, x0, dt);
    const axonhh::State xr = rk4->step(rhs, t0, x0, dt);

    const double err_euler = std::abs(xe.V_mV - exact);
    const double err_rk4 = std::abs(xr.V_mV - exact);

    assert_true(err_rk4 < err_euler, "RK4 error is smaller than Euler error for y' = y");
}

void test_hh_rhs_single_step_finite()
{
    const axonhh::Params p = axonhh::default_params();
    const axonhh::HodgkinHuxley hh(p);

    const double v0 = -65.0;
    const axonhh::State x0 = hh.steady_state(v0);

    const double t0 = 0.0;
    const double dt = 0.01;
    const double iinj = 10.0;

    const axonhh::numerics::RHSFn rhs = [&hh, iinj](double t_ms, const axonhh::State& x) {
        return hh.rhs(t_ms, x, iinj);
    };

    auto euler = axonhh::numerics::make_integrator(axonhh::IntegratorKind::Euler);
    auto rk4 = axonhh::numerics::make_integrator(axonhh::IntegratorKind::RK4);

    const axonhh::State xe = euler->step(rhs, t0, x0, dt);
    const axonhh::State xr = rk4->step(rhs, t0, x0, dt);

    assert_state_finite(xe, "Euler HH single step is finite");
    assert_state_finite(xr, "RK4 HH single step is finite");
}

void test_integrator_factory_behavior()
{
    const auto euler = axonhh::numerics::make_integrator(axonhh::IntegratorKind::Euler);
    const auto rk4 = axonhh::numerics::make_integrator(axonhh::IntegratorKind::RK4);

    assert_true(static_cast<bool>(euler), "factory creates Euler integrator");
    assert_true(static_cast<bool>(rk4), "factory creates RK4 integrator");

    bool threw = false;
    try {
        const auto bad = axonhh::numerics::make_integrator(static_cast<axonhh::IntegratorKind>(255));
        (void)bad;
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    assert_true(threw, "factory rejects invalid IntegratorKind");
}

} // namespace

int main()
{
    std::cout << "[test_integrators] starting\n";

    test_zero_rhs_invariance();
    test_constant_rhs_exactness_single_step();
    test_rk4_more_accurate_than_euler_for_exponential_growth();
    test_hh_rhs_single_step_finite();
    test_integrator_factory_behavior();

    std::cout << "[test_integrators] successful\n";
    return 0;
}
