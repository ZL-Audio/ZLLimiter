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
#include <atomic>
#include <limits>

#include "../../../../PluginProcessor.hpp"
#include "../../../../chore/thread/notifier.hpp"
#include "../../../../dsp/analyzer/analyzer_base/fifo_transfer_buffer.hpp"
#include "../../../../dsp/analyzer/mag_analyzer/magnitude_receiver.hpp"
#include "../../../../dsp/analyzer/value_analyzer/loudness_receiver.hpp"
#include "../../../../dsp/analyzer/value_analyzer/stereo_statistics_receiver.hpp"
#include "../../../../gui/gui.hpp"
#include "../../../helper/helper.hpp"

namespace zlpanel {
    class ValueDisplayPanel final : public juce::Component {
    public:
        explicit ValueDisplayPanel(PluginProcessor& p, zlgui::UIBase& base);

        void prepare(double sample_rate, size_t num_channels);

        void reset();

        void run(zldsp::analyzer::FIFOTransferBuffer<zlp::Controller::kAnalyzerStreamNum>& transfer_buffer,
                 size_t consumer_id);

        void paint(juce::Graphics& g) override;

        void repaintCallBackSlow();

    private:
        enum ValueIdx : size_t {
            kTruePeak, kRMS, kIntegrated, kShortTerm, kLoudnessRange, kCorrelation, kNumValues
        };

        static constexpr float kUnavailable = std::numeric_limits<float>::quiet_NaN();

        zlgui::UIBase& base_;
        zldsp::analyzer::MagnitudeReceiver magnitude_receiver_;
        zldsp::analyzer::LoudnessReceiver loudness_receiver_;
        zldsp::analyzer::StereoStatisticsReceiver stereo_statistics_receiver_;
        float peak_hold_db_{-240.f};
        zlchore::thread::Notifier reset_requested_;

        std::array<std::atomic<float>, kNumValues> values_{};

        void mouseDoubleClick(const juce::MouseEvent& event) override;
    };
}
