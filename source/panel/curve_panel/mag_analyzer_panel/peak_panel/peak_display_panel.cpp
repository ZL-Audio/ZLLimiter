// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#include "peak_display_panel.hpp"

namespace {
    constexpr auto kMaxAnalyzerPointNum = 300;
    constexpr auto kMissingDB = -10000.f;

    constexpr int chooseNumPointsPerSecond(const int sample_rate,
                                           const int preferred_num_points_per_second,
                                           const int time_length) {
        if (sample_rate <= 0 || preferred_num_points_per_second <= 0 || time_length <= 0) {
            return preferred_num_points_per_second;
        }

        const auto max_num_points_per_second = static_cast<int>(kMaxAnalyzerPointNum) / time_length;
        for (auto candidate = preferred_num_points_per_second;
             candidate <= max_num_points_per_second; ++candidate) {
            if (sample_rate % candidate == 0) {
                return candidate;
            }
        }
        return preferred_num_points_per_second;
    }
}

namespace zlpanel {
    PeakDisplayPanel::PeakDisplayPanel(PluginProcessor& p, zlgui::UIBase& base) :
        base_(base),
        pre_curve_display_ref_(*p.parameters_NA_.getRawParameterValue(zlstate::PPreCurveDisplay::kID)),
        post_curve_display_ref_(*p.parameters_NA_.getRawParameterValue(zlstate::PPostCurveDisplay::kID)),
        delta_curve_display_ref_(*p.parameters_NA_.getRawParameterValue(zlstate::PDeltaCurveDisplay::kID)),
        analyzer_mag_type_ref_(*p.parameters_NA_.getRawParameterValue(zlstate::PAnalyzerMagType::kID)),
        analyzer_time_length_ref_(*p.parameters_NA_.getRawParameterValue(zlstate::PAnalyzerTimeLength::kID)),
        analyzer_move_type_ref_(*p.parameters_NA_.getRawParameterValue(zlstate::PAnalyzerMoveType::kID)) {
        constexpr auto preallocateSpace = static_cast<int>(kMaxAnalyzerPointNum) * 3 + 1;
        for (auto& path : in_path_.getBuffer()) {
            path.preallocateSpace(preallocateSpace);
        }
        for (auto& path : out_path_.getBuffer()) {
            path.preallocateSpace(preallocateSpace);
        }
        for (auto& path : reduction_path_.getBuffer()) {
            path.preallocateSpace(preallocateSpace);
        }

        setInterceptsMouseClicks(false, false);
    }

    PeakDisplayPanel::~PeakDisplayPanel() = default;

    void PeakDisplayPanel::reset() {
        is_first_point_ = true;
        num_missing_points_ = 0;
        too_much_samples_ = 0;
        roll_next_point_ = 0;
        pre_receiver_.reset();
        out_receiver_.reset();
        limit_reduction_receiver_.reset();
    }

    void PeakDisplayPanel::paint(juce::Graphics& g) {
        if (pre_curve_display_ref_.load(std::memory_order::relaxed) > .5f) {
            in_path_.pull();
            g.setColour(base_.getColourByIdx(zlgui::ColourIdx::kPreColour));
            g.fillPath(in_path_.getReader());
        }
        if (post_curve_display_ref_.load(std::memory_order::relaxed) > .5f) {
            out_path_.pull();
            g.setColour(base_.getColourByIdx(zlgui::ColourIdx::kPostColour));
            g.strokePath(out_path_.getReader(),
                         juce::PathStrokeType(curve_thickness_,
                                              juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
        }
        if (delta_curve_display_ref_.load(std::memory_order::relaxed) > .5f) {
            reduction_path_.pull();
            g.setColour(base_.getColourByIdx(zlgui::ColourIdx::kReductionColour));
            g.strokePath(reduction_path_.getReader(),
                         juce::PathStrokeType(curve_thickness_,
                                              juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
        }
    }

    void PeakDisplayPanel::resized() {
        auto bound = getLocalBounds();
        const auto font_size = base_.getFontSize();
        bound.removeFromTop(getTopPanelHeight(font_size));
        atomic_bound_.store(bound.toFloat());
        lookAndFeelChanged();
    }

    void PeakDisplayPanel::run(const double next_time_stamp, zldsp::analyzer::FIFOTransferBuffer<
                                   zlp::Controller::kAnalyzerStreamNum>& transfer_buffer,
                               const size_t consumer_id, const MagDBRange& db_range) {
        const auto bound = atomic_bound_.load();
        const auto true_peak = analyzer_mag_type_ref_.load(std::memory_order::relaxed) > .5f;
        if (true_peak != true_peak_) {
            true_peak_ = true_peak;
            reset();
        }
        const auto time_length_idx = analyzer_time_length_ref_.load(std::memory_order::relaxed);
        const auto move_type = static_cast<MoveType>(std::round(
            analyzer_move_type_ref_.load(std::memory_order::relaxed)));

        const auto sample_rate = transfer_buffer.getSampleRate();
        const auto max_num_samples = transfer_buffer.getMaxNumSamples();

        if (std::abs(sample_rate_ - sample_rate) > 0.1 ||
            max_num_samples_ != max_num_samples ||
            std::abs(time_length_idx_ - time_length_idx) > 0.1 || move_type_ != move_type) {
            sample_rate_ = sample_rate;
            max_num_samples_ = max_num_samples;
            time_length_idx_ = time_length_idx;
            move_type_ = move_type;
            const auto time_idx = static_cast<size_t>(std::round(time_length_idx_));
            time_length_ = zlstate::PAnalyzerTimeLength::kLength[time_idx];
            const auto rounded_sample_rate = static_cast<int>(std::round(sample_rate_));
            num_points_per_second_ = chooseNumPointsPerSecond(
                rounded_sample_rate, kNumPointsPerSecond[time_idx], static_cast<int>(std::round(time_length_)));
            num_samples_per_point_ = rounded_sample_rate / num_points_per_second_;
            reset();
            num_missing_points_ = 0;
            too_much_samples_ = 0;
            roll_next_point_ = 0;
            num_points_ = static_cast<size_t>(num_points_per_second_) * static_cast<size_t>(time_length_);
            second_per_point_ = static_cast<double>(time_length_) / static_cast<double>(num_points_);

            const auto history_size = move_type_ == MoveType::kRoll ? num_points_ : num_points_ + 2;
            xs_.resize(history_size);
            pre_ys_.resize(history_size);
            out_ys_.resize(history_size);
            reduction_ys_.resize(history_size);
        }

        updateYMapping(bound, db_range, false);
        const auto missing_y = std::fma(kMissingDB, db_to_y_scale_, db_to_y_bias_);

        auto& fifo{transfer_buffer.getMulticastFIFO()};
        if (!is_first_point_ && next_time_stamp - start_time_ > static_cast<double>(time_length_) + 1.0) {
            reset();
        }
        if (!is_first_point_) {
            // update ys
            while (next_time_stamp - start_time_ > second_per_point_) {
                // if not enough samples
                if (fifo.getNumReady(consumer_id) >= num_samples_per_point_) {
                    const auto range = fifo.prepareToRead(consumer_id, num_samples_per_point_);
                    pre_receiver_.run(range, transfer_buffer.getSampleFIFOs()[zlp::Controller::kAnalyzerPreStream],
                                      true_peak);
                    out_receiver_.run(range, transfer_buffer.getSampleFIFOs()[zlp::Controller::kAnalyzerPostStream],
                                      true_peak);
                    limit_reduction_receiver_.run(
                        range, transfer_buffer.getSampleFIFOs()[zlp::Controller::kAnalyzerGainedPreStream],
                        transfer_buffer.getSampleFIFOs()[zlp::Controller::kAnalyzerPostStream]);
                    pre_db_ = pre_receiver_.getMaxDB();
                    out_db_ = out_receiver_.getMaxDB();
                    reduction_db_ = limit_reduction_receiver_.getMinDB();
                    fifo.finishRead(consumer_id, num_samples_per_point_);
                    num_missing_points_ = 0;
                } else {
                    if (num_missing_points_ < kPausedThreshold) {
                        num_missing_points_ += 1;
                    } else if (num_missing_points_ == kPausedThreshold) {
                        pre_receiver_.reset();
                        out_receiver_.reset();
                        limit_reduction_receiver_.reset();
                        const auto shift = static_cast<ptrdiff_t>(
                            pre_ys_.size() - static_cast<size_t>(kPausedThreshold));
                        std::ranges::fill(pre_ys_.begin() + shift, pre_ys_.end(), missing_y);
                        std::ranges::fill(out_ys_.begin() + shift, out_ys_.end(), missing_y);
                        std::ranges::fill(reduction_ys_.begin() + shift, reduction_ys_.end(), reduction_y_bias_);
                    }
                }
                {
                    const auto too_many_missing = num_missing_points_ >= kPausedThreshold;
                    std::ranges::rotate(pre_ys_, pre_ys_.begin() + 1);
                    pre_ys_.back() = too_many_missing
                        ? missing_y
                        : std::fma(pre_db_, db_to_y_scale_, db_to_y_bias_);
                    std::ranges::rotate(out_ys_, out_ys_.begin() + 1);
                    out_ys_.back() = too_many_missing
                        ? missing_y
                        : std::fma(out_db_, db_to_y_scale_, db_to_y_bias_);
                    std::ranges::rotate(reduction_ys_, reduction_ys_.begin() + 1);
                    reduction_ys_.back() = too_many_missing
                        ? reduction_y_bias_
                        : std::fma(reduction_db_, db_to_y_scale_, reduction_y_bias_);
                }
                if (move_type_ == MoveType::kRoll) {
                    roll_next_point_ = (roll_next_point_ + 1) % xs_.size();
                }
                start_time_ += second_per_point_;
            }
            // if too much samples
            const auto num_ready = fifo.getNumReady(consumer_id);
            const auto threshold = 2 * std::max(static_cast<int>(max_num_samples_), num_samples_per_point_);
            if (num_ready > threshold) {
                too_much_samples_ += (num_ready - threshold) / num_samples_per_point_;
                if (too_much_samples_ > kTooMuchResetThreshold) {
                    (void)fifo.prepareToRead(consumer_id, num_ready - threshold);
                    fifo.finishRead(consumer_id, num_ready - threshold);
                    pre_receiver_.reset();
                    out_receiver_.reset();
                    limit_reduction_receiver_.reset();
                    too_much_samples_ = 0;
                }
            } else {
                too_much_samples_ = 0;
            }
        } else {
            if (fifo.getNumReady(consumer_id) >= num_samples_per_point_) {
                is_first_point_ = false;
                start_time_ = next_time_stamp;
                motion_start_time_ = next_time_stamp;
                std::ranges::fill(pre_ys_, missing_y);
                std::ranges::fill(out_ys_, missing_y);
                std::ranges::fill(reduction_ys_, reduction_y_bias_);
            }
        }

        if (!is_first_point_) {
            updateXs(bound, next_time_stamp);
            updatePaths(bound);
        } else {
            in_path_.getWriter().clear();
            out_path_.getWriter().clear();
            reduction_path_.getWriter().clear();
        }
        in_path_.publish();
        out_path_.publish();
        reduction_path_.publish();
    }

    void PeakDisplayPanel::updateXs(const juce::Rectangle<float> bound, const double next_time_stamp) {
        if (move_type_ == MoveType::kRoll) {
            const auto delta_x = static_cast<double>(bound.getWidth()) / static_cast<double>(xs_.size() - 1);
            for (size_t i = 0; i < xs_.size(); ++i) {
                xs_[i] = bound.getX() + static_cast<float>(
                    static_cast<double>((roll_next_point_ + i) % xs_.size()) * delta_x);
            }
            return;
        }

        const auto length = static_cast<double>(time_length_);
        const auto x_scale = static_cast<double>(bound.getWidth()) / length;
        const auto delta_x = second_per_point_ * x_scale;
        auto x0 = static_cast<double>(bound.getX()) - (next_time_stamp - start_time_) * x_scale;
        if (move_type_ == MoveType::kSlow) {
            constexpr auto fast_length = 0.5;
            constexpr auto restart_position = 0.25;
            const auto slow_length = length - fast_length;
            const auto phase = std::fmod(std::max(0.0, next_time_stamp - motion_start_time_), length);
            constexpr auto sweep = 1.0 - restart_position;
            const auto head = phase < slow_length
                ? restart_position + sweep * phase / slow_length
                : 1.0 - sweep * (phase - slow_length) / fast_length;
            x0 += head * static_cast<double>(bound.getWidth()) -
                static_cast<double>(xs_.size() - 1) * delta_x;
        }
        for (size_t i = 0; i < xs_.size(); ++i) {
            xs_[i] = static_cast<float>(x0);
            x0 += delta_x;
        }
    }

    void PeakDisplayPanel::updateYMapping(const juce::Rectangle<float> bound,
                                          const MagDBRange& db_range,
                                          const bool center_reduction) {
        const auto range_db = db_range.getRangeDB();
        const auto effective_height = std::max(bound.getHeight(), 1.f);
        const auto next_scale = std::abs(range_db) > std::numeric_limits<float>::epsilon()
            ? effective_height / range_db
            : 0.f;
        const auto next_bias = std::fma(-db_range.getMaxDB(), next_scale, bound.getY());
        const auto next_reduction_bias = center_reduction ? bound.getCentreY() : bound.getY();

        if (is_y_mapping_initialized_ && !is_first_point_) {
            const auto scale_changed = std::abs(next_scale - db_to_y_scale_) > 1e-6f;
            const auto bias_changed = std::abs(next_bias - db_to_y_bias_) > 1e-6f;
            const auto reduction_bias_changed = std::abs(next_reduction_bias - reduction_y_bias_) > 1e-6f;
            if ((scale_changed || bias_changed || reduction_bias_changed) &&
                std::abs(db_to_y_scale_) > std::numeric_limits<float>::epsilon()) {
                const auto scale = next_scale / db_to_y_scale_;
                const auto size = pre_ys_.size();
                if (scale_changed || bias_changed) {
                    const auto magnitude_bias = std::fma(-db_to_y_bias_, scale, next_bias);
                    zldsp::vector::fma(pre_ys_.data(), scale, magnitude_bias, size);
                    zldsp::vector::fma(out_ys_.data(), scale, magnitude_bias, size);
                }
                if (scale_changed || reduction_bias_changed) {
                    const auto reduction_bias = std::fma(-reduction_y_bias_, scale, next_reduction_bias);
                    zldsp::vector::fma(reduction_ys_.data(), scale, reduction_bias, size);
                }
            }
        }

        db_to_y_scale_ = next_scale;
        db_to_y_bias_ = next_bias;
        reduction_y_bias_ = next_reduction_bias;
        is_y_mapping_initialized_ = true;
    }

    void PeakDisplayPanel::updatePaths(const juce::Rectangle<float> bound) {
        auto& next_in_path{in_path_.getWriter()};
        auto& next_out_path{out_path_.getWriter()};
        auto& next_reduction_path{reduction_path_.getWriter()};
        next_in_path.clear();
        next_out_path.clear();
        next_reduction_path.clear();

        const auto size = pre_ys_.size();

        next_in_path.startNewSubPath(xs_[0], bound.getBottom());
        next_in_path.lineTo(xs_[0], pre_ys_[0]);
        next_out_path.startNewSubPath(xs_[0], out_ys_[0]);
        next_reduction_path.startNewSubPath(xs_[0], reduction_ys_[0]);
        for (size_t i = 1; i < size; ++i) {
            if (xs_[i] < xs_[i - 1]) {
                next_in_path.lineTo(xs_[i - 1], bound.getBottom());
                next_in_path.closeSubPath();
                next_in_path.startNewSubPath(xs_[i], bound.getBottom());
                next_in_path.lineTo(xs_[i], pre_ys_[i]);
                next_out_path.startNewSubPath(xs_[i], out_ys_[i]);
                next_reduction_path.startNewSubPath(xs_[i], reduction_ys_[i]);
                continue;
            }
            next_in_path.lineTo(xs_[i], pre_ys_[i]);
            next_out_path.lineTo(xs_[i], out_ys_[i]);
            next_reduction_path.lineTo(xs_[i], reduction_ys_[i]);
        }
        next_in_path.lineTo(xs_[size - 1], bound.getBottom());
    }

    void PeakDisplayPanel::lookAndFeelChanged() {
        curve_thickness_ = base_.getFontSize() * .2f * base_.getMagCurveThickness();
    }
}
