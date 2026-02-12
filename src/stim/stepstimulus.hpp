#pragma once

#include "stimulus.hpp"

namespace axonhh::stim {

class StepStimulus final : public Stimulus {
public:
    explicit StepStimulus(StimulusConfig cfg);
    StepStimulus(double amp_uA_cm2, double t0_ms, double t1_ms);

    double current(double t_ms) const override;

    const StimulusConfig& config() const;

private:
    StimulusConfig cfg_;
};

} // namespace axonhh::stim
