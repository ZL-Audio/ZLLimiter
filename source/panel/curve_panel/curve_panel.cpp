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
    CurvePanel::CurvePanel(PluginProcessor& p, zlgui::UIBase& base, multilingual::TooltipHelper&) :
        juce::Thread("ZL Limiter Analyzer"), p_ref_(p), base_(base),
        min_db_ref_(*p.parameters_NA_.getRawParameterValue(zlstate::PAnalyzerMinDB::kID)),
        peak_panel_(p, base), meter_panel_(p, base), analyzer_setting_panel_(p, base),
        peak_consumer_(transfer_buffer_.getMulticastFIFO().addConsumer()),
        meter_consumer_(transfer_buffer_.getMulticastFIFO().addConsumer()) {
        addAndMakeVisible(peak_panel_);
        addAndMakeVisible(meter_panel_);
        addChildComponent(analyzer_setting_panel_);
        addMouseListener(this, true);
        startThread(juce::Thread::Priority::low);
    }

    CurvePanel::~CurvePanel() {
        removeMouseListener(this);
        signalThreadShouldExit();
        notify();
        waitForThreadToExit(-1);
    }

    void CurvePanel::resized() {
        auto bound = getLocalBounds();
        const auto font_size = base_.getFontSize();
        const auto padding = getPaddingSize(font_size);
        meter_panel_.setBounds(bound.removeFromRight(juce::roundToInt(base_.getFontSize() * 6.f)));
        plot_bound_ = bound;
        peak_panel_.setBounds(bound);

        const auto setting_width = analyzer_setting_panel_.getIdealWidth();
        const auto setting_height = analyzer_setting_panel_.getIdealHeight();
        const auto setting_right = getButtonSize(font_size) * 3 + padding * 3 + padding / 2 + juce::roundToInt(
            font_size * 8.f);
        analyzer_setting_panel_.setBounds(setting_right - setting_width, 0, setting_width, setting_height);
    }

    void CurvePanel::paint(juce::Graphics& g) {
        g.fillAll(base_.getBackgroundColour());
        const auto bound = plot_bound_.toFloat();
        const auto min_db = zlstate::PAnalyzerMinDB::getDBFromIndex(min_db_ref_.load(std::memory_order_relaxed));
        const auto text_height = base_.getFontSize() * 1.6f;
        g.setFont(base_.getFontSize());
        for (int i = 0; i <= 6; ++i) {
            const auto proportion = static_cast<float>(i) / 6.f;
            const auto y = bound.getY() + bound.getHeight() * proportion;
            g.setColour(base_.getColourByIdx(zlgui::kGridColour));
            g.fillRect(bound.getX(), y, bound.getWidth(), std::max(1.f, base_.getFontSize() * .125f));
            g.setColour(base_.getTextColour().withAlpha(.5f));
            g.drawText(i == 0 ? juce::String("0") : juce::String(min_db * proportion, std::abs(min_db) < 18.f ? 1 : 0),
                       bound.withY(std::clamp(y - text_height, bound.getY(), bound.getBottom() - text_height))
                       .withHeight(text_height), juce::Justification::right, false);
        }
    }

    void CurvePanel::repaintCallBack(const double time_stamp) {
        next_stamp_.store(time_stamp, std::memory_order_relaxed);
        notify();
        repaint();
    }

    void CurvePanel::repaintCallBackSlow() {
        analyzer_setting_panel_.repaintCallBackSlow();
        meter_panel_.repaintCallBackSlow();
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
                    peak_panel_.reset();
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
            peak_panel_.run(stamp, transfer_buffer_, peak_consumer_, range);
            if (threadShouldExit()) {
                return;
            }
            meter_panel_.getDisplayPanel().run(stamp, transfer_buffer_, meter_consumer_, range);
        }
    }

    void CurvePanel::mouseDown(const juce::MouseEvent& event) {
        if (event.originalComponent != &analyzer_setting_panel_ &&
            !analyzer_setting_panel_.isParentOf(event.originalComponent)) {
            base_.setPanelProperty(zlgui::kAnalyzerSettingPanel, 0.f);
        }
    }
}
