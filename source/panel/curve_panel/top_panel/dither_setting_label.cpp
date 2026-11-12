// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#include "dither_setting_label.hpp"

#include "../../helper/panel_constants.hpp"

namespace zlpanel {
    DitherSettingLabel::DitherSettingLabel(PluginProcessor& p, zlgui::UIState& base) :
        base_(base),
        dither_mode_ref_(*p.parameters_.getRawParameterValue(zlp::PDitherMode::kID)),
        dither_bit_ref_(*p.parameters_.getRawParameterValue(zlp::PDitherBitDepth::kID)),
        control_background_(base),
        label_laf_(base.gui),
        label_("", "") {
        // SettableTooltipClient::setTooltip(tooltip_text);

        control_background_.setInterceptsMouseClicks(false, false);
        addChildComponent(control_background_);

        label_laf_.setFontScale(1.5f);

        for (auto& l : {&label_}) {
            l->setInterceptsMouseClicks(false, false);
            l->setLookAndFeel(&label_laf_);
            l->setJustificationType(juce::Justification::centred);
            l->setBufferedToImage(true);
            addAndMakeVisible(l);
        }

        setInterceptsMouseClicks(true, false);

        base_.getPanelValueTree().addListener(this);
    }

    DitherSettingLabel::~DitherSettingLabel() {
        base_.getPanelValueTree().removeListener(this);
    }

    void DitherSettingLabel::resized() {
        const auto padding = 2 * getPaddingSize(base_.gui.getFontSize());
        const auto bound = getLocalBounds();
        control_background_.setBounds(0, -padding, bound.getWidth(), bound.getHeight() + padding + padding / 4);
        label_.setBounds(bound);
    }

    void DitherSettingLabel::repaintCallBackSlow() {
        const auto dither_mode_idx = dither_mode_ref_.load(std::memory_order::relaxed);
        const auto dither_bit_idx = dither_bit_ref_.load(std::memory_order::relaxed);
        if (std::abs(dither_mode_idx - dither_mode_idx_) > .1f ||
            std::abs(dither_bit_idx - dither_bit_idx_) > .1f) {
            dither_mode_idx_ = dither_mode_idx;
            dither_bit_idx_ = dither_bit_idx;

            label_.setAlpha(dither_mode_idx_ < .5f ? .5f : 1.f);
            label_.setText(dither_bit_idx_ < .5f ? "16 bit" : "24 bit", juce::dontSendNotification);
            label_.repaint();
        }
    }

    void DitherSettingLabel::mouseDown(const juce::MouseEvent&) {
        const auto f = static_cast<double>(base_.getPanelProperty(zlgui::kDitherSettingPanel));
        base_.setPanelProperty(zlgui::kDitherSettingPanel, f < .5 ? 1. : 0.);
    }

    void DitherSettingLabel::mouseEnter(const juce::MouseEvent&) {
        is_over_ = true;
        const auto f = static_cast<double>(base_.getPanelProperty(zlgui::kDitherSettingPanel));
        updateAlpha(f > .5);
    }

    void DitherSettingLabel::mouseExit(const juce::MouseEvent&) {
        is_over_ = false;
        const auto f = static_cast<double>(base_.getPanelProperty(zlgui::kDitherSettingPanel));
        updateAlpha(f > .5);
    }

    void DitherSettingLabel::valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier& property) {
        if (base_.isPanelIdentifier(zlgui::kDitherSettingPanel, property)) {
            const auto f = static_cast<double>(base_.getPanelProperty(zlgui::kDitherSettingPanel));
            updateAlpha(f > .5);
        }
    }

    void DitherSettingLabel::updateAlpha(const bool is_panel_open) {
        if (is_panel_open) {
            control_background_.setVisible(true);
            control_background_.setAlpha(1.f);
        } else if (is_over_) {
            control_background_.setVisible(true);
            control_background_.setAlpha(.75f);
        } else {
            control_background_.setVisible(false);
        }
    }
}
