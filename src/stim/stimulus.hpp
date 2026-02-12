#pragma once

#include <algorithm>
#include <cmath>

#include "core/types.hpp"

namespace axonhh::stim {

class Stimulus {
public:
    virtual ~Stimulus() = default;

    // Injected current at time t (uA/cm^2).
    virtual double current(double t_ms) const = 0;

    double operator()(double t_ms) const { return current(t_ms); }
};

inline bool in_window(double t_ms, double t0_ms, double t1_ms)
{
    return t_ms >= t0_ms && t_ms <= t1_ms;
}

inline double step_current(double t_ms, const StimulusConfig& cfg)
{
    return in_window(t_ms, cfg.t0_ms, cfg.t1_ms) ? cfg.amp_uA_cm2 : 0.0;
}

inline double pulse_current(double t_ms, const StimulusConfig& cfg)
{
    if (!in_window(t_ms, cfg.t0_ms, cfg.t1_ms)) {
        return 0.0;
    }

    if (cfg.period_ms <= 0.0) {
        return 0.0;
    }

    const double duty = std::clamp(cfg.duty, 0.0, 1.0);
    const double on_ms = duty * cfg.period_ms;

    const double elapsed_ms = t_ms - cfg.t0_ms;
    const double phase_ms = std::fmod(elapsed_ms, cfg.period_ms);
    const double wrapped_phase_ms = phase_ms < 0.0 ? (phase_ms + cfg.period_ms) : phase_ms;

    return wrapped_phase_ms <= on_ms ? cfg.amp_uA_cm2 : 0.0;
}

inline double current_from_config(double t_ms, const StimulusConfig& cfg)
{
    switch (cfg.kind) {
        case StimulusKind::Step:
            return step_current(t_ms, cfg);
        case StimulusKind::Pulse:
            return pulse_current(t_ms, cfg);
        default:
            return 0.0;
    }
}

} // namespace axonhh::stim
