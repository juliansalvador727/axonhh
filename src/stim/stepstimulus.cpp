#include "stepstimulus.hpp"

namespace axonhh::stim {

StepStimulus::StepStimulus(StimulusConfig cfg) : cfg_(cfg)
{
    cfg_.kind = StimulusKind::Step;
}

StepStimulus::StepStimulus(double amp_uA_cm2, double t0_ms, double t1_ms)
    : cfg_{StimulusKind::Step, amp_uA_cm2, t0_ms, t1_ms, 0.0, 0.0}
{
}

double StepStimulus::current(double t_ms) const
{
    return step_current(t_ms, cfg_);
}

const StimulusConfig& StepStimulus::config() const
{
    return cfg_;
}

} // namespace axonhh::stim
