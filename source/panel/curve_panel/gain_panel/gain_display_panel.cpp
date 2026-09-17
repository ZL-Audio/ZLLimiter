// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.


#include "gain_display_panel.hpp"

#include "../../helper/panel_constants.hpp"

namespace zlpanel {
    GainDisplayPanel::GainDisplayPanel(PluginProcessor& p, zlgui::UIBase& base, multilingual::TooltipHelper&) :
        base_(base), updater_(),
        gain_slider_(base, ""),
        gain_slider_attach_(gain_slider_, p.parameters_, zlp::PInputGain::kID, updater_) {
        for (auto const s : {&gain_slider_}) {
            s->setSliderStyle(base_.getRotaryStyle());
            s->setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, true, 0, 0);
            s->setDoubleClickReturnValue(true, 0.0);
            s->setScrollWheelEnabled(true);
            s->setInterceptsMouseClicks(false, false);
        }
        addChildComponent(gain_slider_);

        gain_slider_.addListener(this);
    }

    GainDisplayPanel::~GainDisplayPanel() {
        gain_slider_.removeListener(this);
    }

    void GainDisplayPanel::paint(juce::Graphics& g) {
        const auto font_size = base_.getFontSize();
        const auto padding = static_cast<float>(getPaddingSize(font_size));

        auto bound = getLocalBounds().toFloat();

        bound.removeFromLeft(padding);
        bound.removeFromBottom(2.f * padding);
        bound.removeFromTop(2.f * padding);

        const auto p = gain_slider_.getNormalisableRange().convertTo0to1(gain_slider_.getValue());
        const auto y = bound.getY() + static_cast<float>(1.0 - p) * bound.getHeight();
        const auto filled_colour = base_.getTextColour().withAlpha(is_mouse_over_ ? 1.f : .5f);
        g.setColour(filled_colour);

        juce::Path path;
        path.addRoundedRectangle(bound.getX(), y - padding, 2.f * padding, 2.f * padding,
                                 1.f * padding, 1.f * padding, false, true, false, true);
        g.fillPath(path);
    }

    void GainDisplayPanel::resized() {
        const auto bound = getLocalBounds();
        gain_slider_.setBounds(bound);

        const auto font_size = base_.getFontSize();
        const auto padding = getPaddingSize(font_size);
        drag_distance_ = bound.getHeight() - 2 * padding;
    }

    void GainDisplayPanel::repaintCallBackSlow() {
        updater_.updateComponents();
    }

    void GainDisplayPanel::setMouseOver(const bool is_mouse_over) {
        is_mouse_over_ = is_mouse_over;
    }

    void GainDisplayPanel::sliderValueChanged(juce::Slider*) {
        repaint();
    }

    void GainDisplayPanel::mouseUp(const juce::MouseEvent& event) {
        gain_slider_.mouseUp(event);
    }

    void GainDisplayPanel::mouseDown(const juce::MouseEvent& event) {
        updateDragDistance(event.mods.isShiftDown());
        gain_slider_.mouseDown(event);
    }

    void GainDisplayPanel::mouseDrag(const juce::MouseEvent& event) {
        gain_slider_.mouseDrag(event);
    }

    void GainDisplayPanel::mouseEnter(const juce::MouseEvent& event) {
        gain_slider_.mouseEnter(event);
    }

    void GainDisplayPanel::mouseExit(const juce::MouseEvent& event) {
        gain_slider_.mouseExit(event);
    }

    void GainDisplayPanel::mouseDoubleClick(const juce::MouseEvent& event) {
        gain_slider_.mouseDoubleClick(event);
    }

    void GainDisplayPanel::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) {
        gain_slider_.mouseWheelMove(event, wheel);
    }

    void GainDisplayPanel::updateDragDistance(const bool is_shift_pressed) {
        int actual_drag_distance;
        if (is_shift_pressed) {
            actual_drag_distance = juce::roundToInt(
                static_cast<float>(drag_distance_) * 10.f / base_.getSensitivity(
                    zlgui::SensitivityIdx::kMouseSliderFine));
        } else {
            actual_drag_distance = juce::roundToInt(
                static_cast<float>(drag_distance_) / base_.getSensitivity(zlgui::SensitivityIdx::kMouseSlider));
        }
        actual_drag_distance = std::max(actual_drag_distance, 1);
        gain_slider_.setMouseDragSensitivity(actual_drag_distance);
    }
}
