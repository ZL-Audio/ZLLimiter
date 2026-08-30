// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "../dsp/over_sample/over_sample.hpp"

namespace zlp {
    namespace hn = hwy::HWY_NAMESPACE;

    class Controller final : private juce::AsyncUpdater {
    public:
        static constexpr size_t kFilterSize = 16;

        explicit Controller(juce::AudioProcessor& p);

        void prepare(double sample_rate, size_t max_num_samples);

        void prepareBuffer();

        void process(const std::array<float*, 2>& buffer, size_t num_samples, bool is_bypass);

    private:
        static constexpr hn::ScalableTag<float> d{};
        static constexpr size_t lanes = hn::MaxLanes(d);

        juce::AudioProcessor& p_ref_;

        void handleAsyncUpdate() override;
    };
}
