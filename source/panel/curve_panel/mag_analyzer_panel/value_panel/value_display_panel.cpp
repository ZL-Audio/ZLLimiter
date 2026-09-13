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
    ValueDisplayPanel::ValueDisplayPanel(PluginProcessor&, zlgui::UIBase& base) :
        base_(base) {
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

        peak_hold_db_ = -240.f;
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
        loudness_receiver_.run(range, samples);
        stereo_statistics_receiver_.run(range, samples);
        fifo.finishRead(consumer_id, num_ready);

        peak_hold_db_ = std::max(peak_hold_db_, magnitude_receiver_.getMaxDB());
        values_[kTruePeak].store(peak_hold_db_, std::memory_order::relaxed);
        values_[kRMS].store(stereo_statistics_receiver_.getRMSDB(), std::memory_order::relaxed);
        values_[kCorrelation].store(stereo_statistics_receiver_.getCorrelation(), std::memory_order::relaxed);
        const auto& meter = loudness_receiver_.getMeter();
        values_[kIntegrated].store(meter.isIntegratedReady() ? meter.getIntegratedLoudness() : kUnavailable,
                                   std::memory_order::relaxed);
        values_[kShortTerm].store(meter.isShortTermReady() ? meter.getShortTermLoudness() : kUnavailable,
                                  std::memory_order::relaxed);
        values_[kLoudnessRange].store(meter.isLoudnessRangeReady() ? meter.getLoudnessRange() : kUnavailable,
                                      std::memory_order::relaxed);
    }

    void ValueDisplayPanel::paint(juce::Graphics&) {
    }

    void ValueDisplayPanel::repaintCallBackSlow() {
    }

    void ValueDisplayPanel::mouseDoubleClick(const juce::MouseEvent&) {
        reset_requested_.signal();
    }
}
