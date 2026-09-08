// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../../PluginProcessor.hpp"
#include "../../../chore/thread/tri_buffer.hpp"
#include "../../../gui/gui.hpp"
#include "../../helper/helper.hpp"
#include "../../../dsp/analyzer/analyzer_base/fifo_transfer_buffer.hpp"
#include "../../../dsp/analyzer/mag_analyzer/magnitude_receiver.hpp"
#include "../mag_db_range.hpp"


namespace zlpanel {
    class PeakPanel final : public juce::Component {
    public:
        explicit PeakPanel(PluginProcessor& p, zlgui::UIBase& base);

        ~PeakPanel() override;

        void paint(juce::Graphics& g) override;

        void run(double next_time_stamp, zldsp::analyzer::FIFOTransferBuffer<zlp::Controller::kAnalyzerStreamNum>& transfer_buffer,
                 size_t consumer_id, const MagDBRange& db_range);

        void resized() override;

        void reset();

    private:
        static constexpr std::array<int, 4> kNumPointsPerSecond{40, 30, 20, 15};
        static constexpr int kPausedThreshold = 6;
        static constexpr int kTooMuchResetThreshold = 64;
        zlgui::UIBase& base_;

        std::atomic<float>& pre_curve_display_ref_;
        std::atomic<float>& post_curve_display_ref_;
        std::atomic<float>& delta_curve_display_ref_;
        std::atomic<float>& analyzer_mag_type_ref_;
        std::atomic<float>& analyzer_time_length_ref_;
        std::atomic<float>& analyzer_move_type_ref_;

        AtomicBound<float> atomic_bound_;
        zldsp::analyzer::MagnitudeReceiver pre_receiver_, out_receiver_, gained_pre_receiver_;
        bool true_peak_{false};

        float pre_db_{-240.f}, out_db_{-240.f}, reduction_db_{0.f};
        zldsp::vector::aligned_vector<float> xs_{}, pre_ys_{}, reduction_ys_{}, out_ys_{};
        zlchore::thread::TriBuffer<juce::Path> in_path_, out_path_, reduction_path_;

        float db_to_y_scale_{0.f};
        float db_to_y_bias_{0.f};
        float reduction_y_bias_{0.f};
        bool is_y_mapping_initialized_{false};

        float curve_thickness_{0.f};

        double start_time_{0.0};
        double motion_start_time_{0.0};

        // Matches PAnalyzerMoveType's choice order.
        enum class MoveType { kSync, kSlow, kRoll };
        MoveType move_type_{MoveType::kSync};
        size_t roll_next_point_{0};

        bool is_first_point_{true};
        int too_much_samples_{0};
        int num_missing_points_{0};

        double sample_rate_{0.};
        size_t max_num_samples_{0};
        float time_length_idx_{0.f}, time_length_{6.f};

        size_t num_points_{0};
        int num_samples_per_point_{0};
        int num_points_per_second_{0};
        double second_per_point_{0};

        void updateYMapping(juce::Rectangle<float> bound, const MagDBRange& db_range,
                            bool center_reduction);

        void updatePaths(juce::Rectangle<float> bound);

        void updateXs(juce::Rectangle<float> bound, double next_time_stamp);

        void lookAndFeelChanged() override;
    };
}
