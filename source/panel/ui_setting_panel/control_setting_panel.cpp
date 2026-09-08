// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#include "control_setting_panel.hpp"

namespace zlpanel {
    ControlSettingPanel::ControlSettingPanel(PluginProcessor& p, zlgui::UIBase& base)
        : p_ref_(p),
          base_(base), name_laf_(base),
          sensitivity_sliders_{
              {
                  zlgui::slider::CompactLinearSlider < true, true, true > ("Rough", base),
                  zlgui::slider::CompactLinearSlider < true, true, true > ("Fine", base),
                  zlgui::slider::CompactLinearSlider < true, true, true > ("Rough", base),
                  zlgui::slider::CompactLinearSlider < true, true, true > ("Fine", base),
                  zlgui::slider::CompactLinearSlider < true, true, true > ("Menu", base)
              }
          },
          wheel_reverse_box_(zlstate::PWheelShiftReverse::kChoices, base),
          rotary_style_box_(zlstate::PRotaryStyle::kChoices, base),
          rotary_drag_sensitivity_slider_("Distance", base),
          slider_double_click_box_(zlstate::PSliderDoubleClickFunc::kChoices, base) {
        juce::ignoreUnused(p_ref_);
        name_laf_.setFontScale(zlgui::kFontHuge);

        wheel_label_.setText("Wheel Sensitivity", juce::dontSendNotification);
        wheel_label_.setJustificationType(juce::Justification::centredRight);
        wheel_label_.setLookAndFeel(&name_laf_);
        addAndMakeVisible(wheel_label_);
        slider_label_.setText("Slider Sensitivity", juce::dontSendNotification);
        slider_label_.setJustificationType(juce::Justification::centredRight);
        slider_label_.setLookAndFeel(&name_laf_);
        addAndMakeVisible(slider_label_);
        for (auto& s : sensitivity_sliders_) {
            s.getSlider().setRange(0.01, 1.0, 0.01);
            addAndMakeVisible(s);
        }
        addAndMakeVisible(wheel_reverse_box_);
        sensitivity_sliders_[0].getSlider().setDoubleClickReturnValue(true, 1.0);
        sensitivity_sliders_[1].getSlider().setDoubleClickReturnValue(true, 0.12);
        sensitivity_sliders_[2].getSlider().setDoubleClickReturnValue(true, 1.0);
        sensitivity_sliders_[3].getSlider().setDoubleClickReturnValue(true, 0.25);
        sensitivity_sliders_[4].getSlider().setDoubleClickReturnValue(true, 0.5);
        rotary_style_label_.setText("Rotary Slider Style", juce::dontSendNotification);
        rotary_style_label_.setJustificationType(juce::Justification::centredRight);
        rotary_style_label_.setLookAndFeel(&name_laf_);
        addAndMakeVisible(rotary_style_label_);
        addAndMakeVisible(rotary_style_box_);
        rotary_drag_sensitivity_slider_.getSlider().setRange(2.0, 32.0, 0.01);
        rotary_drag_sensitivity_slider_.getSlider().setDoubleClickReturnValue(true, 10.0);
        addAndMakeVisible(rotary_drag_sensitivity_slider_);
        slider_double_click_label_.setText("Slider Double Click", juce::dontSendNotification);
        slider_double_click_label_.setJustificationType(juce::Justification::centredRight);
        slider_double_click_label_.setLookAndFeel(&name_laf_);
        addAndMakeVisible(slider_double_click_label_);
        addAndMakeVisible(slider_double_click_box_);


    }

    ControlSettingPanel::~ControlSettingPanel() = default;

    void ControlSettingPanel::loadSetting() {
        for (size_t i = 0; i < sensitivity_sliders_.size(); ++i) {
            sensitivity_sliders_[i].getSlider().setValue(static_cast<double>(base_.getSensitivity(
                static_cast<zlgui::SensitivityIdx>(i))));
        }
        wheel_reverse_box_.getBox().setSelectedId(static_cast<int>(base_.getIsMouseWheelShiftReverse()) + 1);
        rotary_style_box_.getBox().setSelectedId(static_cast<int>(base_.getRotaryStyleID()) + 1);
        rotary_drag_sensitivity_slider_.getSlider().setValue(static_cast<double>(base_.getRotaryDragSensitivity()));
        slider_double_click_box_.getBox().setSelectedId(
            static_cast<int>(base_.getIsSliderDoubleClickOpenEditor()) + 1);
    }

    void ControlSettingPanel::saveSetting() {
        for (size_t i = 0; i < sensitivity_sliders_.size(); ++i) {
            base_.setSensitivity(static_cast<float>(sensitivity_sliders_[i].getSlider().getValue()),
                                 static_cast<zlgui::SensitivityIdx>(i));
        }
        base_.setIsMouseWheelShiftReverse(static_cast<bool>(wheel_reverse_box_.getBox().getSelectedId() - 1));
        base_.setRotaryStyleID(static_cast<size_t>(rotary_style_box_.getBox().getSelectedId() - 1));
        base_.setRotaryDragSensitivity(static_cast<float>(rotary_drag_sensitivity_slider_.getSlider().getValue()));
        base_.setIsSliderDoubleClickOpenEditor(
            static_cast<bool>(slider_double_click_box_.getBox().getSelectedId() - 1));
        base_.saveToAPVTS();
    }

    void ControlSettingPanel::resetSetting() {
        constexpr std::array defaults{zlstate::PWheelSensitivity::kDefaultV,
            zlstate::PWheelFineSensitivity::kDefaultV, zlstate::PSliderSensitivity::kDefaultV,
            zlstate::PSliderFineSensitivity::kDefaultV, zlstate::PWheelComboboxSensitivity::kDefaultV};
        for (size_t i = 0; i < defaults.size(); ++i) {
            sensitivity_sliders_[i].getSlider().setValue(defaults[i]);
        }
        wheel_reverse_box_.getBox().setSelectedItemIndex(zlstate::PWheelShiftReverse::kDefaultI);
        rotary_style_box_.getBox().setSelectedItemIndex(zlstate::PRotaryStyle::kDefaultI);
        rotary_drag_sensitivity_slider_.getSlider().setValue(zlstate::PRotaryDragSensitivity::kDefaultV);
        slider_double_click_box_.getBox().setSelectedItemIndex(zlstate::PSliderDoubleClickFunc::kDefaultI);
        saveSetting();
    }

    int ControlSettingPanel::getIdealHeight() const {
        const auto padding = juce::roundToInt(base_.getFontSize() * kPaddingScale * 3.f);
        const auto slider_height = juce::roundToInt(base_.getFontSize() * kSliderHeightScale);

        return padding * 5 + slider_height * 4;
    }

    void ControlSettingPanel::resized() {
        const auto padding = juce::roundToInt(base_.getFontSize() * kPaddingScale * 3.f);
        const auto slider_width = juce::roundToInt(base_.getFontSize() * kSliderWidthScale);
        const auto slider_height = juce::roundToInt(base_.getFontSize() * kSliderHeightScale);
        static constexpr int kLabelWidth = 2;

        auto bound = getLocalBounds();
        {
            bound.removeFromTop(padding);
            auto local_bound = bound.removeFromTop(slider_height);
            wheel_label_.setBounds(local_bound.removeFromLeft(slider_width * kLabelWidth));
            local_bound.removeFromLeft(padding);
            sensitivity_sliders_[0].setBounds(local_bound.removeFromLeft(slider_width));
            local_bound.removeFromLeft(padding);
            sensitivity_sliders_[1].setBounds(local_bound.removeFromLeft(slider_width));
            local_bound.removeFromLeft(padding);
            sensitivity_sliders_[4].setBounds(local_bound.removeFromLeft(slider_width));
            local_bound.removeFromLeft(padding);
            wheel_reverse_box_.setBounds(local_bound.removeFromLeft(slider_width + padding).reduced(0, padding / 3));
        }
        {
            bound.removeFromTop(padding);
            auto local_bound = bound.removeFromTop(slider_height);
            slider_label_.setBounds(local_bound.removeFromLeft(slider_width * kLabelWidth));
            local_bound.removeFromLeft(padding);
            sensitivity_sliders_[2].setBounds(local_bound.removeFromLeft(slider_width));
            local_bound.removeFromLeft(padding);
            sensitivity_sliders_[3].setBounds(local_bound.removeFromLeft(slider_width));
        }

        {
            bound.removeFromTop(padding);
            auto local_bound = bound.removeFromTop(slider_height);
            rotary_style_label_.setBounds(local_bound.removeFromLeft(slider_width * kLabelWidth));
            local_bound.removeFromLeft(padding);
            rotary_style_box_.setBounds(local_bound.removeFromLeft(slider_width / 2 * 3).reduced(0, padding / 3));
            local_bound.removeFromLeft(padding);
            rotary_drag_sensitivity_slider_.setBounds(local_bound.removeFromLeft(slider_width));
        }
        {
            bound.removeFromTop(padding);
            auto local_bound = bound.removeFromTop(slider_height);
            slider_double_click_label_.setBounds(local_bound.removeFromLeft(slider_width * kLabelWidth));
            local_bound.removeFromLeft(padding);
            slider_double_click_box_.setBounds(local_bound.removeFromLeft(slider_width * 2).reduced(0, padding / 3));
        }

    }

} // zlpanel
