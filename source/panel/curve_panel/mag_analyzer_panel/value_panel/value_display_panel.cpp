// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#include "value_display_panel.hpp"

namespace zlpanel {
    ValueDisplayPanel::ValueDisplayPanel(PluginProcessor& p, zlgui::UIBase& base) :
        base_(base),
        true_peak_on_(*p.parameters_NA_.getRawParameterValue(zlstate::PValueTruePeakON::kID),
                      zlstate::PValueTruePeakON::kDefaultV),
        corr_on_(*p.parameters_NA_.getRawParameterValue(zlstate::PValueStereoCorrON::kID),
                 zlstate::PValueStereoCorrON::kDefaultV),
        lufsm_on_(*p.parameters_NA_.getRawParameterValue(zlstate::PValueLUFSMON::kID),
                  zlstate::PValueLUFSMON::kDefaultV),
        lufss_on_(*p.parameters_NA_.getRawParameterValue(zlstate::PValueLUFSSON::kID),
                  zlstate::PValueLUFSSON::kDefaultV),
        lra_on_(*p.parameters_NA_.getRawParameterValue(zlstate::PValueLRAON::kID),
                zlstate::PValueLRAON::kDefaultV),
        lufsi_on_(*p.parameters_NA_.getRawParameterValue(zlstate::PValueLUFSION::kID),
                  zlstate::PValueLUFSION::kDefaultV) {
        reset();
    }

    void ValueDisplayPanel::prepare(const double sample_rate, const size_t num_channels) {
        magnitude_receiver_.prepare(num_channels);
        loudness_receiver_.prepare(sample_rate, num_channels);
        stereo_statistics_receiver_.prepare(sample_rate, num_channels);
        reset();
    }

    void ValueDisplayPanel::reset() {
        magnitude_receiver_.reset();
        loudness_receiver_.reset();
        stereo_statistics_receiver_.reset();

        peak_hold_db_ = kUnavailable;
        max_short_term_ = kUnavailable;
        max_momentary_ = kUnavailable;
        weighted_correlation_sum_ = 0.0;
        correlation_weight_sum_ = 0.0;
        for (auto& value : values_) {
            value.store(kUnavailable, std::memory_order::relaxed);
        }
    }

    void ValueDisplayPanel::run(
        zldsp::analyzer::FIFOTransferBuffer<zlp::Controller::kAnalyzerStreamNum>& transfer_buffer,
        const size_t consumer_id) {
        auto& fifo = transfer_buffer.getMulticastFIFO();
        if (reset_requested_.check()) {
            reset();
            fifo.finishRead(consumer_id, fifo.getNumReady(consumer_id));
            return;
        }
        const auto num_ready = fifo.getNumReady(consumer_id);
        if (num_ready == 0) {
            return;
        }
        const auto range = fifo.prepareToRead(consumer_id, num_ready);
        const auto& samples = transfer_buffer.getSampleFIFOs()[zlp::Controller::kAnalyzerPostStream];
        magnitude_receiver_.run(range, samples, true);
        loudness_receiver_.run(range, samples, [this](const auto& meter) {
            const auto momentary = meter.getMomentaryLoudness();
            if (!std::isnan(momentary)) {
                max_momentary_ = std::isnan(max_momentary_)
                    ? momentary
                    : std::max(max_momentary_, momentary);
            }
            if (meter.isShortTermReady()) {
                const auto short_term = meter.getShortTermLoudness();
                if (!std::isnan(short_term)) {
                    max_short_term_ = std::isnan(max_short_term_)
                        ? short_term
                        : std::max(max_short_term_, short_term);
                }
            }
        });
        stereo_statistics_receiver_.run(range, samples,
                                        [this](const float correlation, const double energy_weight) {
                                            if (std::isfinite(correlation) && energy_weight > 0.0 && std::isfinite(
                                                energy_weight)) {
                                                weighted_correlation_sum_ += static_cast<double>(correlation) *
                                                    energy_weight;
                                                correlation_weight_sum_ += energy_weight;
                                            }
                                        });
        fifo.finishRead(consumer_id, num_ready);

        if (std::isnan(peak_hold_db_)) {
            peak_hold_db_ = magnitude_receiver_.getMaxDB();
        } else {
            peak_hold_db_ = std::max(peak_hold_db_, magnitude_receiver_.getMaxDB());
        }
        values_[kTruePeak].store(peak_hold_db_, std::memory_order::relaxed);
        values_[kCorrelation].store(stereo_statistics_receiver_.getCorrelation(), std::memory_order::relaxed);
        values_[kMaxShortTerm].store(max_short_term_, std::memory_order::relaxed);
        values_[kMaxMomentary].store(max_momentary_, std::memory_order::relaxed);
        values_[kAverageCorrelation].store(correlation_weight_sum_ > 0.0
                                           ? static_cast<float>(std::clamp(
                                               weighted_correlation_sum_ / correlation_weight_sum_, -1.0, 1.0))
                                           : kUnavailable,
                                           std::memory_order::relaxed);
        const auto& meter = loudness_receiver_.getMeter();
        values_[kMomentary].store(meter.isMomentaryReady() ? meter.getMomentaryLoudness() : kUnavailable,
                                  std::memory_order::relaxed);
        values_[kIntegrated].store(meter.isIntegratedReady() ? meter.getIntegratedLoudness() : kUnavailable,
                                   std::memory_order::relaxed);
        values_[kShortTerm].store(meter.isShortTermReady() ? meter.getShortTermLoudness() : kUnavailable,
                                  std::memory_order::relaxed);
        values_[kLoudnessRange].store(meter.isLoudnessRangeReady() ? meter.getLoudnessRange() : kUnavailable,
                                      std::memory_order::relaxed);
    }

    void ValueDisplayPanel::paint(juce::Graphics& g) {
        const auto num_values = static_cast<int>(true_peak_on_.value) + static_cast<int>(corr_on_.value) +
            static_cast<int>(lufsm_on_.value) + static_cast<int>(lufss_on_.value) +
            static_cast<int>(lra_on_.value) + static_cast<int>(lufsi_on_.value);

        auto bound = getLocalBounds().toFloat();
        const auto height = .5f * bound.getHeight() / static_cast<float>(num_values);

        g.setFont(base_.getFontSize() * 1.75f);
        g.setColour(base_.getTextColour());

        if (true_peak_on_.value) {
            bound.removeFromTop(height);
            const auto v = values_[kTruePeak].load(std::memory_order::relaxed);
            g.drawText(std::isfinite(v) && v > -220.f ? formatValue(v) : "--",
                       bound.removeFromTop(height),
                       juce::Justification::centred, false);
        }
        if (corr_on_.value) {
            bound.removeFromTop(height);
            auto t_bound = bound.removeFromTop(height);
            {
                const auto v = values_[kCorrelation].load(std::memory_order::relaxed);
                g.drawText(std::isfinite(v) ? formatValue(v) : "--",
                           t_bound.removeFromLeft(bound.getWidth() * .5f),
                           juce::Justification::centred, false);
            }
            {
                const auto v = values_[kAverageCorrelation].load(std::memory_order::relaxed);
                g.drawText(std::isfinite(v) ? formatValue(v) : "--",
                           t_bound,
                           juce::Justification::centred, false);
            }
        }
        if (lufsm_on_.value) {
            bound.removeFromTop(height);
            auto t_bound = bound.removeFromTop(height);
            {
                const auto v = values_[kMomentary].load(std::memory_order::relaxed);
                g.drawText(std::isfinite(v) && v > -220.f ? formatValue(v) : "--",
                           t_bound.removeFromLeft(bound.getWidth() * .5f),
                           juce::Justification::centred, false);
            }
            {
                const auto v = values_[kMaxMomentary].load(std::memory_order::relaxed);
                g.drawText(std::isfinite(v) ? formatValue(v) : "--",
                           t_bound,
                           juce::Justification::centred, false);
            }
        }
        if (lufss_on_.value) {
            bound.removeFromTop(height);
            auto t_bound = bound.removeFromTop(height);
            {
                const auto v = values_[kShortTerm].load(std::memory_order::relaxed);
                g.drawText(std::isfinite(v) && v > -220.f ? formatValue(v) : "--",
                           t_bound.removeFromLeft(bound.getWidth() * .5f),
                           juce::Justification::centred, false);
            }
            {
                const auto v = values_[kMaxShortTerm].load(std::memory_order::relaxed);
                g.drawText(std::isfinite(v) && v > -220.f ? formatValue(v) : "--",
                           t_bound,
                           juce::Justification::centred, false);
            }
        }
        if (lra_on_.value) {
            bound.removeFromTop(height);
            const auto v = values_[kLoudnessRange].load(std::memory_order::relaxed);
            g.drawText(std::isfinite(v) ? formatValue(v) : "--",
                       bound.removeFromTop(height),
                       juce::Justification::centred, false);
        }
        if (lufsi_on_.value) {
            bound.removeFromTop(height);
            const auto v = values_[kIntegrated].load(std::memory_order::relaxed);
            g.drawText(std::isfinite(v) ? formatValue(v) : "--",
                       bound.removeFromTop(height),
                       juce::Justification::centred, false);
        }
    }

    void ValueDisplayPanel::repaintCallBackSlow() {
        bool to_repaint = false;

        to_repaint = to_repaint || true_peak_on_.update();
        to_repaint = to_repaint || corr_on_.update();
        to_repaint = to_repaint || lufsm_on_.update();
        to_repaint = to_repaint || lufss_on_.update();
        to_repaint = to_repaint || lra_on_.update();
        to_repaint = to_repaint || lufsi_on_.update();

        callback_counts_ += 1;
        if (callback_counts_ == 5) {
            callback_counts_ = 0;
            to_repaint = true;
        }
        if (to_repaint) {
            repaint();
        }
    }

    void ValueDisplayPanel::mouseDoubleClick(const juce::MouseEvent&) {
        reset_requested_.signal();
    }

    std::string ValueDisplayPanel::formatValue(const float value) {
        std::stringstream ss;
        const auto abs_value = std::abs(value);
        if (abs_value < 10.f) {
            ss << std::fixed << std::setprecision(2) << value;
        } else if (abs_value < 100.f) {
            ss << std::fixed << std::setprecision(1) << value;
        } else {
            ss << std::fixed << std::setprecision(0) << value;
        }
        return ss.str();
    }
}
