// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#include "limiter_attach.hpp"

#include <cmath>

namespace zlp {
    LimiterAttach::LimiterAttach(juce::AudioProcessor& processor,
                                 juce::AudioProcessorValueTreeState& parameters,
                                 Controller& controller) :
        processor_ref_(processor),
        parameters_ref_(parameters),
        controller_ref_(controller) {
        juce::ignoreUnused(processor_ref_);
        for (const auto* id : kIDs) {
            parameters_ref_.addParameterListener(id, this);
            parameterChanged(id, parameters_ref_.getRawParameterValue(id)->load(std::memory_order_relaxed));
        }
    }

    LimiterAttach::~LimiterAttach() {
        for (const auto* id : kIDs) {
            parameters_ref_.removeParameterListener(id, this);
        }
    }

    void LimiterAttach::parameterChanged(const juce::String& parameter_id, const float value) {
        if (parameter_id == PInputGain::kID) {
            controller_ref_.setInputGain(value);
        } else if (parameter_id == POutputCeiling::kID) {
            controller_ref_.setOutputCeiling(value);
        } else if (parameter_id == PTruePeak::kID) {
            controller_ref_.setTruePeakEnabled(value >= 0.5f);
        } else if (parameter_id == POversampling::kID) {
            controller_ref_.setOversamplingIndex(static_cast<int>(std::lround(value)));
        } else if (parameter_id == PLookahead::kID) {
            controller_ref_.setLookahead(value);
        } else if (parameter_id == PAttack::kID) {
            controller_ref_.setAttack(value);
        } else if (parameter_id == PRelease::kID) {
            controller_ref_.setRelease(value);
        } else if (parameter_id == PRecovery::kID) {
            controller_ref_.setRecovery(value);
        } else if (parameter_id == PChannelDelta::kID) {
            controller_ref_.setChannelDelta(value);
        } else if (parameter_id == PBypass::kID) {
            controller_ref_.setBypassEnabled(value > 0.5f);
        } else if (parameter_id == PDelta::kID) {
            controller_ref_.setDeltaEnabled(value > 0.5f);
        }
    }
}
