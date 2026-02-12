#include "pulsestimulus.hpp"

namespace axonhh::stim {

PulseStimulus::PulseStimulus(StimulusConfig cfg) : cfg_(cfg)
{
    cfg_.kind = StimulusKind::Pulse;
}

PulseStimulus::PulseStimulus(double amp_uA_cm2, double t0_ms, double t1_ms, double period_ms, double duty)
    : cfg_{StimulusKind::Pulse, amp_uA_cm2, t0_ms, t1_ms, period_ms, duty}
{
}

double PulseStimulus::current(double t_ms) const
{
    return pulse_current(t_ms, cfg_);
}

const StimulusConfig& PulseStimulus::config() const
{
    return cfg_;
}

} // namespace axonhh::stim
