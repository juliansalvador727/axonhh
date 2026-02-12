#pragma once

#include <functional>
#include <memory>

#include "core/types.hpp"

namespace axonhh::numerics {

using RHSFn = std::function<Deriv(double t_ms, const State& x)>;

class Integrator {
public:
    virtual ~Integrator() = default;

    virtual State step(const RHSFn& rhs, double t_ms, const State& x, double dt_ms) const = 0;
};

std::unique_ptr<Integrator> make_integrator(IntegratorKind kind);

} // namespace axonhh::numerics
