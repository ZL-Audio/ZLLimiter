// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#include "top_control_panel.hpp"
#include "BinaryData.h"

namespace zlpanel {
    TopControlPanel::TopControlPanel(PluginProcessor& p, zlgui::UIBase& base, multilingual::TooltipHelper&) :
        base_(base), label_laf_(base),
        input_label_("Input", "Input"), ceiling_label_("Ceiling", "Ceiling"),
        oversample_label_("Oversample", "Oversample"),
        input_slider_("", base),
        ceiling_slider_("", base),
        input_attachment_(input_slider_.getSlider(), p.parameters_, zlp::PInputGain::kID, updater_),
        ceiling_attachment_(ceiling_slider_.getSlider(), p.parameters_, zlp::POutputCeiling::kID, updater_),
        oversample_box_(zlp::POversampling::kChoices, base),
        oversample_attachment_(oversample_box_.getBox(), p.parameters_, zlp::POversampling::kID, updater_),
        true_peak_icon_(juce::Drawable::createFromImageData(BinaryData::dline_tp_svg, BinaryData::dline_tp_svgSize)),
        true_peak_button_(base, true_peak_icon_.get(), true_peak_icon_.get()),
        true_peak_attachment_(true_peak_button_.getButton(), p.parameters_, zlp::PTruePeak::kID, updater_),
        delta_icon_(juce::Drawable::createFromImageData(BinaryData::delta_svg, BinaryData::delta_svgSize)),
        delta_button_(base, delta_icon_.get(), delta_icon_.get()),
        delta_attachment_(delta_button_.getButton(), p.parameters_, zlp::PDelta::kID, updater_),
        bypass_icon_(juce::Drawable::createFromImageData(BinaryData::bypass_svg, BinaryData::bypass_svgSize)),
        bypass_button_(base, bypass_icon_.get(), bypass_icon_.get()),
        bypass_attachment_(bypass_button_.getButton(), p.parameters_, zlp::PBypass::kID, updater_) {
        label_laf_.setFontScale(1.5f);
        for (auto* label : {&input_label_, &ceiling_label_, &oversample_label_}) {
            label->setLookAndFeel(&label_laf_);
            label->setJustificationType(juce::Justification::centredRight);
            label->setBufferedToImage(true);
            addAndMakeVisible(label);
        }
        for (auto* slider : {&input_slider_, &ceiling_slider_}) {
            slider->setFontScale(1.5f);
            slider->setJustification(juce::Justification::centred);
            slider->setBufferedToImage(true);
            slider->getSlider().setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            slider->getSlider().setSliderSnapsToMousePosition(false);
            addAndMakeVisible(slider);
        }
        input_slider_.getSlider().setComponentID(zlp::PInputGain::kID);
        ceiling_slider_.getSlider().setComponentID(zlp::POutputCeiling::kID);

        oversample_box_.setScrollEnabled(true);
        oversample_box_.getLAF().setFontScale(1.5f);
        oversample_box_.setBufferedToImage(true);
        addAndMakeVisible(oversample_box_);

        for (auto* button : {&true_peak_button_, &delta_button_, &bypass_button_}) {
            button->getButton().setClickingTogglesState(true);
            button->setImageAlpha(.5f, .75f, 1.f, 1.f);
            addAndMakeVisible(button);
        }
        updater_.updateComponents();
    }

    void TopControlPanel::resized() {
        auto bound = getLocalBounds();
        const auto font_size = base_.getFontSize();
        const auto padding = getPaddingSize(font_size);
        const auto button_width = getButtonSize(font_size);
        const auto label_width = juce::roundToInt(font_size * kSliderWidthScale * 1.25f);
        const auto value_width = juce::roundToInt(font_size * kSmallSliderWidthScale * .75f);
        bypass_button_.setBounds(bound.removeFromRight(button_width));
        bound.removeFromRight(padding);

        delta_button_.setBounds(bound.removeFromRight(button_width));
        delta_button_.getButton().setEdgeIndent(juce::roundToInt(font_size * .225f));
        bound.removeFromRight(padding);

        true_peak_button_.setBounds(bound.removeFromRight(button_width));
        bound.removeFromRight(padding);

        oversample_box_.setBounds(bound.removeFromRight(value_width));
        bound.removeFromRight(padding);
        oversample_label_.setBounds(bound.removeFromRight(label_width));
        bound.removeFromRight(padding);
        ceiling_slider_.setBounds(bound.removeFromRight(value_width));
        bound.removeFromRight(padding);
        ceiling_label_.setBounds(bound.removeFromRight(getSliderWidth(font_size)));
        bound.removeFromRight(padding);
        input_slider_.setBounds(bound.removeFromRight(value_width));
        bound.removeFromRight(padding);
        input_label_.setBounds(bound.removeFromRight(getSmallSliderWidth(font_size)));

        for (auto* slider : {&input_slider_, &ceiling_slider_}) {
            slider->setMouseDragSensitivity(getSliderDraggingDistance(font_size));
        }
    }

    void TopControlPanel::repaintCallBackSlow() {
        updater_.updateComponents();
    }
}
