#include "euler.hpp"

namespace axonhh::numerics {

State EulerIntegrator::step(const RHSFn& rhs, double t_ms, const State& x, double dt_ms) const
{
    const Deriv k1 = rhs(t_ms, x);
    return add_scaled(x, k1, dt_ms);
}

} // namespace axonhh::numerics
