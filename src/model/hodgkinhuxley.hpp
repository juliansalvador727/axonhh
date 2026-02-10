#pragma once

#include "core/types.hpp"

namespace axonhh {

class HodgkinHuxley {
public:
    explicit HodgkinHuxley(Params params);

    const Params& params() const;

    Currents currents(double t_ms, const State& x, double Iinj_uA_cm2) const;
    
    Deriv rhs(double t_ms, const State& x, double Iinj_uA_cm2) const;

    State steady_state(double V0_mV) const;

private:
    Params p_;
};
}