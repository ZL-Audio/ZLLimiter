// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#include "meter_display_panel.hpp"

namespace zlpanel {
    MeterDisplayPanel::MeterDisplayPanel(PluginProcessor& p, zlgui::UIBase& base) :
        base_(base),
        meter_top_panel_(base),
        analyzer_mag_type_ref_(*p.parameters_NA_.getRawParameterValue(zlstate::PAnalyzerMagType::kID)) {

        const auto target_refresh_id = p.state_.getRawParameterValue(
            zlstate::PTargetRefreshSpeed::kID)->load(std::memory_order::relaxed);
        const auto circular_capacity = static_cast<size_t>(
            zlstate::PTargetRefreshSpeed::kRates[static_cast<size_t>(std::round(target_refresh_id))]);
        circular_min_max_.setCapacity(circular_capacity);
        circular_min_max_.setSize(circular_capacity);

        meter_top_panel_.setBufferedToImage(true);
        addAndMakeVisible(meter_top_panel_);
    }

    MeterDisplayPanel::~MeterDisplayPanel() = default;

    void MeterDisplayPanel::reset() {
        is_first_point_ = true;
        missing_seconds_ = 0.0;
        pre_receiver_.reset();
        out_receiver_.reset();
        gained_pre_receiver_.reset();
        previous_pre_.fill(-240.f);
        previous_out_.fill(-240.f);
        previous_reduction_.fill(0.f);
        pre_decay_mul_.fill(1.f);
        out_decay_mul_.fill(1.f);
        circular_min_max_.clear();
        reset_peaks_.store(true, std::memory_order::relaxed);
    }

    void MeterDisplayPanel::paint(juce::Graphics& g) {
        g.setFont(base_.getFontSize());
        const auto text_height = base_.getFontSize() * 1.25f;
        g.setColour(base_.getColourByIdx(zlgui::ColourIdx::kReductionColour));
        for (auto& a_bound : reduction_rect_) {
            g.fillRect(a_bound.load());
        }
        const auto reduction_max_rect = reduction_max_rect_.load();
        if (reduction_max_rect.getY() > .5f * text_height) {
            g.fillRect(reduction_max_rect);

            const auto reduction_max = reduction_max_value_.load(std::memory_order::relaxed);
            g.setColour(base_.getTextColour());
            g.drawText(formatValue(std::abs(reduction_max)),
                       juce::Rectangle<float>{reduction_max_rect.getX(),
                                              reduction_max_rect.getY(),
                                              reduction_max_rect.getWidth(), text_height},
                       juce::Justification::centred, false);
        }
        g.setColour(base_.getColourByIdx(zlgui::ColourIdx::kPreColour));
        for (auto& a_bound : pre_rect_) {
            g.fillRect(a_bound.load());
        }
        g.setColour(base_.getColourByIdx(zlgui::ColourIdx::kPostColour));
        for (auto& a_bound : out_rect_) {
            g.fillRect(a_bound.load());
        }

    }

    void MeterDisplayPanel::resized() {
        const auto bound = getLocalBounds();
        const auto font_size = base_.getFontSize();
        pending_bound_.store(bound.toFloat());
        pending_reduction_max_height_.store(font_size * .25f, std::memory_order::relaxed);
        size_changed_.signal();
        meter_top_panel_.setBounds(bound.withHeight(juce::roundToInt(font_size * 1.25f)));
    }

    void MeterDisplayPanel::updateSize() {
        if (!size_changed_.check()) {
            return;
        }
        bound_ = pending_bound_.load();
        const auto meter_width = bound_.getWidth() * .2f;
        const auto meter_padding = meter_width * .5f;

        constexpr auto x1 = 0.f;
        const auto x2 = x1 + meter_width + meter_padding * .5f;
        const auto x3 = x2 + meter_width + meter_padding;
        const auto x4 = x3 + meter_width + meter_padding * .5f;

        reduction_rect_[0].store({x1, 0.f, meter_width, 0.f});
        reduction_rect_[1].store({x2, 0.f, meter_width, 0.f});

        pre_rect_[0].store({x3, 0.f, meter_width, 0.f});
        pre_rect_[1].store({x4, 0.f, meter_width, 0.f});

        out_rect_[0].store({x3, 0.f, meter_width, 0.f});
        out_rect_[1].store({x4, 0.f, meter_width, 0.f});

        reduction_max_rect_.store({x1, 0.f, x2 + meter_width,
                                   pending_reduction_max_height_.load(std::memory_order::relaxed)});
    }

    void MeterDisplayPanel::repaintCallBackSlow() {
        meter_top_panel_.updateValue(reduction_peak_.load(std::memory_order::relaxed),
                                     out_peak_.load(std::memory_order::relaxed));
    }

    void MeterDisplayPanel::run(const double next_time_stamp,
                                zldsp::analyzer::FIFOTransferBuffer<zlp::Controller::kAnalyzerStreamNum>& transfer_buffer,
                                const size_t consumer_id, const MagDBRange& db_range) {
        updateSize();
        const auto true_peak = analyzer_mag_type_ref_.load(std::memory_order::relaxed) > .5f;
        if (true_peak != true_peak_) {
            true_peak_ = true_peak;
            reset();
        }
        if (is_first_point_) {
            is_first_point_ = false;
            start_time_ = next_time_stamp;
            return;
        }
        if (reset_peaks_.exchange(false, std::memory_order::relaxed)) {
            reduction_peak_.store(0.f, std::memory_order::relaxed);
            out_peak_.store(-240.f, std::memory_order::relaxed);
        }

        const auto delta_time = std::clamp(next_time_stamp - start_time_, 0.01, 1.0);
        start_time_ = next_time_stamp;
        // run meter receiver
        auto& fifo{transfer_buffer.getMulticastFIFO()};
        const auto delta_num_samples = static_cast<int>(delta_time * transfer_buffer.getSampleRate());
        const auto num_ready = fifo.getNumReady(consumer_id);
        const auto threshold = 2 * std::max(static_cast<int>(transfer_buffer.getMaxNumSamples()),
                                            delta_num_samples);
        const int num_to_read = num_ready > threshold
            ? num_ready - threshold
            : std::min(num_ready, delta_num_samples);

        const auto range = fifo.prepareToRead(consumer_id, num_to_read);
        pre_receiver_.run(range, transfer_buffer.getSampleFIFOs()[zlp::Controller::kAnalyzerPreStream], true_peak);
        gained_pre_receiver_.run(range, transfer_buffer.getSampleFIFOs()[zlp::Controller::kAnalyzerGainedPreStream], true_peak);
        out_receiver_.run(range, transfer_buffer.getSampleFIFOs()[zlp::Controller::kAnalyzerPostStream], true_peak);
        fifo.finishRead(consumer_id, num_to_read);
        const auto& pre_dbs = pre_receiver_.getDBs();
        const auto& gained_pre_dbs = gained_pre_receiver_.getDBs();
        const auto& out_dbs = out_receiver_.getDBs();
        const std::array<float, 2> reduction_dbs{
            std::min(0.f, out_dbs[0] - gained_pre_dbs[0]),
            std::min(0.f, out_dbs[1] - gained_pre_dbs[1])};
        missing_seconds_ = num_to_read == 0 ? missing_seconds_ + delta_time : 0.0;
        if (missing_seconds_ > 0.25) {
            pre_receiver_.reset();
            out_receiver_.reset();
            gained_pre_receiver_.reset();
        }

        const auto bound = bound_;
        // update reduction peak
        const auto reduction_peak = std::min(reduction_dbs[0], reduction_dbs[1]);
        reduction_peak_.store(std::min(reduction_peak, reduction_peak_.load(std::memory_order::relaxed)),
                              std::memory_order::relaxed);

        // update reduction meter
        for (size_t chan = 0; chan < 2; ++chan) {
            const auto current_reduction = reduction_dbs[chan];
            const auto previous_reduction = previous_reduction_[chan];
            previous_reduction_[chan] = std::min(
                previous_reduction + static_cast<float>(delta_time) * kReductionDecayPerSecond,
                current_reduction);
            reduction_rect_[chan].setHeight(
                db_range.getReductionYProportion(previous_reduction_[chan]) * bound.getHeight());

        }
        // update reduction short-term max display
        float reduction_max_value;
        float reduction_max_pos;
        reduction_max_value = std::max(0.f, std::max(-reduction_dbs[0], -reduction_dbs[1]));
        reduction_max_value = circular_min_max_.push(reduction_max_value);
        reduction_max_pos = -db_range.getReductionYProportion(reduction_max_value) * bound.getHeight();

        reduction_max_rect_.setY(reduction_max_pos - reduction_max_rect_.getHeight() * .5f);
        reduction_max_value_.store(reduction_max_value, std::memory_order::relaxed);
        // update pre meter
        for (size_t chan = 0; chan < 2; ++chan) {
            const auto current_pre = pre_dbs[chan];
            const auto previous_pre = previous_pre_[chan];
            if (current_pre > previous_pre) {
                previous_pre_[chan] = current_pre;
                pre_decay_mul_[chan] = 1.f;
            } else {
                previous_pre_[chan] = std::max(
                    previous_pre - pre_decay_mul_[chan] * static_cast<float>(delta_time) * kMeterDecayPerSecond,
                    current_pre);
                pre_decay_mul_[chan] = std::min(pre_decay_mul_[chan] * (1.f + 3.f * static_cast<float>(delta_time)),
                                                10.f);
            }
            const auto pre_y = std::clamp(db_range.getYProportion(previous_pre_[chan]), 0.f, 1.f) *
                bound.getHeight();
            pre_rect_[chan].setY(pre_y);
            pre_rect_[chan].setHeight(bound.getHeight() - pre_y);
        }
        // update out peak
        const auto out_peak = std::max(out_dbs[0], out_dbs[1]);
        out_peak_.store(std::max(out_peak, out_peak_.load(std::memory_order::relaxed)),
                        std::memory_order::relaxed);
        // update out meter
        for (size_t chan = 0; chan < 2; ++chan) {
            const auto current_out = out_dbs[chan];
            const auto previous_out = previous_out_[chan];
            if (current_out > previous_out) {
                previous_out_[chan] = current_out;
                out_decay_mul_[chan] = 1.f;
            } else {
                previous_out_[chan] = std::max(
                    previous_out - out_decay_mul_[chan] * static_cast<float>(delta_time) * kMeterDecayPerSecond,
                    current_out);
                out_decay_mul_[chan] = std::min(out_decay_mul_[chan] * (1.f + 3.f * static_cast<float>(delta_time)),
                                                10.f);
            }
            const auto out_y = std::clamp(db_range.getYProportion(previous_out_[chan]), 0.f, 1.f) *
                bound.getHeight();
            out_rect_[chan].setY(out_y);
            out_rect_[chan].setHeight(bound.getHeight() - out_y);
        }
    }

    void MeterDisplayPanel::mouseDoubleClick(const juce::MouseEvent&) {
        reset_peaks_.store(true, std::memory_order::relaxed);
    }

    std::string MeterDisplayPanel::formatValue(const float value) {
        std::stringstream ss;
        if (std::abs(value) < 100.f) {
            ss << std::fixed << std::setprecision(1) << value;
        } else {
            ss << std::fixed << std::setprecision(0) << value;
        }
        return ss.str();
    }
}
