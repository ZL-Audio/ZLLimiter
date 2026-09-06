// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <array>

#include <juce_audio_processors/juce_audio_processors.h>

#include "controller.hpp"
#include "zlp_definitions.hpp"

namespace zlp {
    class LimiterAttach final : private juce::AudioProcessorValueTreeState::Listener {
    public:
        LimiterAttach(juce::AudioProcessor& processor,
                      juce::AudioProcessorValueTreeState& parameters,
                      Controller& controller);

        ~LimiterAttach() override;

    private:
        juce::AudioProcessor& processor_ref_;
        juce::AudioProcessorValueTreeState& parameters_ref_;
        Controller& controller_ref_;

        static constexpr std::array kIDs{PInputGain::kID, POutputCeiling::kID,
                                         PTruePeak::kID, POversampling::kID,
                                         PLookahead::kID, PAttack::kID, PRelease::kID, PChannelDelta::kID,
                                         PBypass::kID, PDelta::kID, PRecover::kID,
        };

        void parameterChanged(const juce::String& parameter_id, float value) override;
    };
}
