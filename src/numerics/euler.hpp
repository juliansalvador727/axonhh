#pragma once

#include "integrator.hpp"

namespace axonhh::numerics {

class EulerIntegrator final : public Integrator {
public:
    State step(const RHSFn& rhs, double t_ms, const State& x, double dt_ms) const override;
};

} // namespace axonhh::numerics
