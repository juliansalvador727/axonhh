#include "rk4.hpp"

#include <stdexcept>

#include "euler.hpp"

namespace axonhh::numerics {

State RK4Integrator::step(const RHSFn& rhs, double t_ms, const State& x, double dt_ms) const
{
    const Deriv k1 = rhs(t_ms, x);
    const Deriv k2 = rhs(t_ms + 0.5 * dt_ms, add_scaled(x, k1, 0.5 * dt_ms));
    const Deriv k3 = rhs(t_ms + 0.5 * dt_ms, add_scaled(x, k2, 0.5 * dt_ms));
    const Deriv k4 = rhs(t_ms + dt_ms, add_scaled(x, k3, dt_ms));

    const double w = dt_ms / 6.0;

    return State{
        x.V_mV + w * (k1.dV_dt + 2.0 * k2.dV_dt + 2.0 * k3.dV_dt + k4.dV_dt),
        x.m + w * (k1.dm_dt + 2.0 * k2.dm_dt + 2.0 * k3.dm_dt + k4.dm_dt),
        x.h + w * (k1.dh_dt + 2.0 * k2.dh_dt + 2.0 * k3.dh_dt + k4.dh_dt),
        x.n + w * (k1.dn_dt + 2.0 * k2.dn_dt + 2.0 * k3.dn_dt + k4.dn_dt),
    };
}

std::unique_ptr<Integrator> make_integrator(IntegratorKind kind)
{
    switch (kind) {
        case IntegratorKind::Euler:
            return std::make_unique<EulerIntegrator>();
        case IntegratorKind::RK4:
            return std::make_unique<RK4Integrator>();
        default:
            throw std::invalid_argument("unknown IntegratorKind");
    }
}

} // namespace axonhh::numerics
