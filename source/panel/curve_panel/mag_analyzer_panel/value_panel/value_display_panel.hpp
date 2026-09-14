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
#include "../../../../chore/thread/tri_buffer.hpp"
#include "../../../../dsp/analyzer/analyzer_base/fifo_transfer_buffer.hpp"
#include "../../../../dsp/analyzer/mag_analyzer/magnitude_receiver.hpp"
#include "../../../../dsp/analyzer/value_analyzer/loudness_receiver.hpp"
#include "../../../../dsp/analyzer/value_analyzer/stereo_statistics_receiver.hpp"
#include "../../../../gui/gui.hpp"
#include "../../../helper/helper.hpp"
#include "../../mag_db_range.hpp"

namespace zlpanel {
    class ValueDisplayPanel final : public juce::Component {
    public:
        explicit ValueDisplayPanel(PluginProcessor& p, zlgui::UIBase& base);

        void prepare(double sample_rate, size_t num_channels);

        void reset();

        void run(zldsp::analyzer::FIFOTransferBuffer<zlp::Controller::kAnalyzerStreamNum>& transfer_buffer,
                 size_t consumer_id, const MagDBRange& db_range, bool measure_values = true);

        void paint(juce::Graphics& g) override;

        void resized() override;

        void repaintCallBackSlow(const std::array<bool, 6>& value_on, bool to_repaint);

    private:
        enum ValueIdx : size_t {
            kTruePeak, kCorrelation, kMomentary, kShortTerm, kLoudnessRange, kIntegrated,
            kAverageCorrelation, kMaxMomentary, kMaxShortTerm, kNumValues
        };

        static constexpr float kUnavailable = std::numeric_limits<float>::quiet_NaN();
        static constexpr size_t kHistogramBins = 108;

        zlgui::UIBase& base_;
        zldsp::analyzer::MagnitudeReceiver magnitude_receiver_;
        zldsp::analyzer::LoudnessReceiver loudness_receiver_;
        zldsp::analyzer::StereoStatisticsReceiver stereo_statistics_receiver_;
        float peak_hold_db_{-240.f};
        float max_short_term_{kUnavailable}, max_momentary_{kUnavailable};
        double weighted_correlation_sum_{0.0}, correlation_weight_sum_{0.0};
        zlchore::thread::Notifier& reset_requested_;

        std::array<std::atomic<float>, kNumValues> values_{};

        std::array<double, kHistogramBins> histogram_{};
        double histogram_max_{0.0};
        MagDBRange histogram_db_range_{0.f, 0.f};
        AtomicBound<float> pending_histogram_bound_;
        juce::Rectangle<float> histogram_bound_;
        zlchore::thread::TriBuffer<juce::Path> histogram_path_;
        bool histogram_dirty_{true};

        int callback_counts_{0};

        std::array<bool, 6> value_on_{true, true, true, true, true, true};

        void addToHistogram(float momentary);

        void updateHistogramPath();

        void mouseDoubleClick(const juce::MouseEvent& event) override;

        static std::string formatValue(float value);
    };
}
