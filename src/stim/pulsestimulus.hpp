#pragma once

#include "stimulus.hpp"

namespace axonhh::stim {

class PulseStimulus final : public Stimulus {
public:
    explicit PulseStimulus(StimulusConfig cfg);
    PulseStimulus(double amp_uA_cm2, double t0_ms, double t1_ms, double period_ms, double duty);

    double current(double t_ms) const override;

    const StimulusConfig& config() const;

private:
    StimulusConfig cfg_;
};

} // namespace axonhh::stim
