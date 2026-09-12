// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#include "curve_panel.hpp"

namespace zlpanel {
    CurvePanel::CurvePanel(PluginProcessor& p, zlgui::UIBase& base,
                           multilingual::TooltipHelper& tooltip_helper) :
        juce::Thread("ZL Limiter Analyzer"), p_ref_(p), base_(base),
        min_db_ref_(*p.parameters_NA_.getRawParameterValue(zlstate::PAnalyzerMinDB::kID)),
        is_meter_on_ref_(*p.parameters_NA_.getRawParameterValue(zlstate::PMeterDisplayON::kID)),
        is_value_on_ref_(*p.parameters_NA_.getRawParameterValue(zlstate::PValueDisplayON::kID)),
        top_panel_(p, base, tooltip_helper),
        peak_panel_(p, base),
        meter_panel_(p, base),
        value_panel_(p, base),
        analyzer_setting_panel_(p, base),
        peak_consumer_(transfer_buffer_.getMulticastFIFO().addConsumer()),
        meter_consumer_(transfer_buffer_.getMulticastFIFO().addConsumer()) {
        addAndMakeVisible(peak_panel_);
        addAndMakeVisible(meter_panel_);
        value_panel_.setBufferedToImage(true);
        addAndMakeVisible(value_panel_);
        top_panel_.setBufferedToImage(true);
        addAndMakeVisible(top_panel_);
        analyzer_setting_panel_.setBufferedToImage(true);
        addChildComponent(analyzer_setting_panel_);
        addMouseListener(this, true);
        startThread(juce::Thread::Priority::low);
    }

    CurvePanel::~CurvePanel() {
        removeMouseListener(this);
        if (isThreadRunning()) {
            stopThread(-1);
        }
    }

    void CurvePanel::paint(juce::Graphics& g) {
        g.fillAll(base_.getBackgroundColour());
    }

    void CurvePanel::paintOverChildren(juce::Graphics&) {
        notify();
    }

    void CurvePanel::resized() {
        updateBounds();

        auto bound = getLocalBounds();
        const auto font_size = base_.getFontSize();
        const auto padding = getPaddingSize(font_size);
        const auto setting_width = analyzer_setting_panel_.getIdealWidth();
        const auto setting_height = analyzer_setting_panel_.getIdealHeight();
        const auto setting_right = getButtonSize(font_size) * 3 + padding * 3 + padding / 2 + juce::roundToInt(
            font_size * 8.f);
        bound.removeFromTop(top_panel_.getIdealHeight());
        analyzer_setting_panel_.setBounds(setting_right - setting_width, bound.getY(), setting_width, setting_height);
    }

    void CurvePanel::repaintCallBack(const double time_stamp) {
        next_stamp_.store(time_stamp, std::memory_order_relaxed);
        repaint();
    }

    void CurvePanel::repaintCallBackSlow() {
        const auto is_meter_on = is_meter_on_ref_.load(std::memory_order_relaxed) > .5f;
        const auto is_value_on = is_value_on_ref_.load(std::memory_order_relaxed) > .5f;
        if (is_meter_on != is_meter_on_ || is_value_on != is_value_on_) {
            is_meter_on_ = is_meter_on;
            is_value_on_ = is_value_on;
            updateBounds();
        }
        analyzer_setting_panel_.repaintCallBackSlow();
        meter_panel_.repaintCallBackSlow();
        peak_panel_.repaintCallBackSlow();
        value_panel_.repaintCallBackSlow();
        top_panel_.repaintCallBackSlow();
    }

    void CurvePanel::run() {
        juce::ScopedNoDenormals no_denormals;
        while (!threadShouldExit()) {
            (void)wait(-1);
            if (threadShouldExit()) {
                return;
            }
            auto& controller = p_ref_.getController();
            auto& sender = controller.getMagAnalyzerSender();
            {
                const std::lock_guard guard(sender.getLock());
                if (sender.getSampleRate() <= 0.0 || sender.getMaxNumSamples() == 0) {
                    continue;
                }
                const auto generation = controller.getAnalyzerGeneration();
                if (generation != generation_) {
                    generation_ = generation;
                    auto& fifo = sender.getAbstractFIFO();
                    const auto ready = fifo.getNumReady();
                    if (ready > 0) {
                        fifo.finishRead(ready);
                    }
                    const auto capacity_seconds = std::max(0.5,
                                                           4.0 * static_cast<double>(sender.getMaxNumSamples()) / sender
                                                           .getSampleRate());
                    transfer_buffer_.prepare(sender.getSampleRate(), sender.getMaxNumSamples(),
                                             {2, 2, 2}, capacity_seconds);
                    peak_panel_.getDisplayPanel().reset();
                    meter_panel_.getDisplayPanel().reset();
                }
                transfer_buffer_.processTransfer(sender.getAbstractFIFO(), sender.getSampleFIFOs());
            }
            if (threadShouldExit()) {
                return;
            }
            const MagDBRange range(0.f, zlstate::PAnalyzerMinDB::getDBFromIndex(
                                       min_db_ref_.load(std::memory_order_relaxed)));
            const auto stamp = next_stamp_.load(std::memory_order_relaxed);
            peak_panel_.getDisplayPanel().run(stamp, transfer_buffer_, peak_consumer_, range);
            if (threadShouldExit()) {
                return;
            }
            meter_panel_.getDisplayPanel().run(stamp, transfer_buffer_, meter_consumer_, range);
        }
    }

    void CurvePanel::mouseDown(const juce::MouseEvent& event) {
        if (event.originalComponent != &analyzer_setting_panel_ &&
            !analyzer_setting_panel_.isParentOf(event.originalComponent) &&
            !top_panel_.isParentOf(event.originalComponent)) {
            base_.setPanelProperty(zlgui::kAnalyzerSettingPanel, 0.f);
        }
    }

    void CurvePanel::updateBounds() {
        auto bound = getLocalBounds();
        value_panel_.setVisible(is_value_on_);
        if (is_value_on_) {
            value_panel_.setBounds(bound.removeFromRight(value_panel_.getIdealWidth()));
        }
        meter_panel_.setVisible(is_meter_on_);
        if (is_meter_on_) {
            meter_panel_.setBounds(bound.removeFromRight(meter_panel_.getIdealWidth()));
        }
        peak_panel_.setBounds(bound);
        top_panel_.setBounds(bound.removeFromTop(top_panel_.getIdealHeight()));
    }
}
