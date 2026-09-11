// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include "../../../../PluginProcessor.hpp"
#include "../../../../chore/thread/notifier.hpp"
#include "../../../../gui/gui.hpp"
#include "../../../helper/helper.hpp"
#include "../../../../dsp/analyzer/analyzer_base/fifo_transfer_buffer.hpp"
#include "../../../../dsp/analyzer/mag_analyzer/limit_reduction_receiver.hpp"
#include "../../../../dsp/analyzer/mag_analyzer/magnitude_receiver.hpp"
#include "../../../../dsp/container/circular_minmax_buffer.hpp"
#include "../../mag_db_range.hpp"
#include "meter_top_panel.hpp"

namespace zlpanel {
    class MeterDisplayPanel final : public juce::Component {
    public:
        explicit MeterDisplayPanel(PluginProcessor& p, zlgui::UIBase& base);

        ~MeterDisplayPanel() override;

        void paint(juce::Graphics& g) override;

        void run(double next_time_stamp,
                 zldsp::analyzer::FIFOTransferBuffer<zlp::Controller::kAnalyzerStreamNum>& transfer_buffer,
                 size_t consumer_id, const MagDBRange& db_range);

        void resized() override;

        void reset();

        void repaintCallBackSlow();

    private:
        static constexpr float kReductionDecayPerSecond = 16.f;
        static constexpr float kMeterDecayPerSecond = 2.f;
        static constexpr double kMeterGapConvergenceSeconds = 1.0;
        zlgui::UIBase& base_;
        MeterTopPanel meter_top_panel_;

        std::atomic<float>& analyzer_mag_type_ref_;

        AtomicBound<float> pending_bound_;
        std::atomic<float> pending_thickness_{0.f};
        zlchore::thread::Notifier size_changed_{true};
        juce::Rectangle<float> bound_;
        std::array<float, 2> previous_reduction_{0.f, 0.f};
        std::array<float, 2> previous_pre_{-240.f, -240.f};
        std::array<float, 2> pre_decay_mul_{1.f, 1.f};
        std::array<float, 2> previous_out_{-240.f, -240.f};
        std::array<float, 2> out_decay_mul_{1.f, 1.f};
        std::array<double, 2> target_gap_db_{0.0, 0.0};
        std::array<double, 2> gap_remaining_seconds_{0.0, 0.0};
        std::array<AtomicBound<float>, 2> reduction_rect_{};
        std::array<AtomicBound<float>, 2> pre_rect_{};
        std::array<AtomicBound<float>, 2> out_rect_{};
        std::array<AtomicBound<float>, 2> out_arrow_{};

        double start_time_{0.0};
        double missing_seconds_{0.0};
        bool is_first_point_{true};

        zldsp::container::CircularMinMaxBuffer<float, zldsp::container::MinMaxBufferType::kFindMax> circular_min_max_;
        AtomicBound<float> reduction_max_rect_{};
        std::atomic<float> reduction_max_value_{0.f};

        zldsp::analyzer::LimitReductionReceiver limit_reduction_receiver_{};
        zldsp::analyzer::MagnitudeReceiver pre_receiver_{};
        zldsp::analyzer::MagnitudeReceiver out_receiver_{};

        std::atomic<float> reduction_peak_{0.f};
        std::atomic<float> out_peak_{-240.f};

        bool true_peak_{false};
        std::atomic<bool> reset_peaks_{false};

        void mouseDoubleClick(const juce::MouseEvent& event) override;

        void updateSize();

        static std::string formatValue(float value);

        void lookAndFeelChanged() override;
    };
}
