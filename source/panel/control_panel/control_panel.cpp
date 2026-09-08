// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#include "control_panel.hpp"

namespace zlpanel {
    ControlPanel::ControlPanel(PluginProcessor& p, zlgui::UIBase& base, multilingual::TooltipHelper&) :
        base_(base), background_(base, .5f, {false, false, false, false}), label_laf_(base),
        sliders_{
            Rotary("Lookahead", base, "", 1.25f),
            Rotary("Recover", base, "", 1.25f),
            Rotary("Attack", base, "", 1.25f),
            Rotary("Release", base, "", 1.25f),
            Rotary("Delta", base, "", 1.25f)
        } {
        background_.setBufferedToImage(true);
        addAndMakeVisible(background_);

        label_laf_.setFontScale(1.5f);
        label_laf_.setMaximumNumberOfLines(2);
        const std::array names{"Lookahead", "Recover", "Attack", "Release", "Delta"};
        const std::array ids{zlp::PLookahead::kID, zlp::PRecover::kID, zlp::PAttack::kID,
                             zlp::PRelease::kID, zlp::PChannelDelta::kID};

        for (size_t i = 0; i < sliders_.size(); ++i) {
            labels_[i].setText(names[i], juce::dontSendNotification);
            labels_[i].setLookAndFeel(&label_laf_);
            labels_[i].setJustificationType(juce::Justification::centred);
            labels_[i].setBorderSize(juce::BorderSize<int>{0});
            labels_[i].setInterceptsMouseClicks(false, false);
            labels_[i].setBufferedToImage(true);
            addAndMakeVisible(labels_[i]);
            sliders_[i].setComponentID(ids[i]);
            sliders_[i].getSlider1().setComponentID(ids[i]);
            attachments_[i] = std::make_unique<zlgui::attachment::SliderAttachment<true>>(
                sliders_[i].getSlider1(), p.parameters_, ids[i], updater_);
            addAndMakeVisible(sliders_[i]);
        }

        setInterceptsMouseClicks(false, true);
    }

    int ControlPanel::getIdealWidth() const {
        const auto font_size = base_.getFontSize();
        const auto padding = getPaddingSize(font_size);
        const auto slider_width = getSliderWidth(font_size);

        return 5 * slider_width + 12 * padding;
    }

    int ControlPanel::getIdealHeight() const {
        const auto font_size = base_.getFontSize();
        const auto slider_width = getSliderWidth(font_size);
        const auto button_height = getButtonSize(font_size);
        const auto padding = getPaddingSize(font_size);

        return slider_width + button_height + 3 * padding;
    }

    void ControlPanel::resized() {
        const auto font_size = base_.getFontSize();
        const auto padding = getPaddingSize(font_size);
        auto bound = getLocalBounds();
        background_.setBounds(bound);
        bound.reduce(2 * padding, padding);
        auto label_bound = bound.removeFromTop(getButtonSize(font_size));
        const auto width = getSliderWidth(font_size);
        for (size_t i = 0; i < sliders_.size(); ++i) {
            const auto cell = bound.removeFromLeft(width);
            labels_[i].setBounds(label_bound.removeFromLeft(width));
            sliders_[i].setBounds(cell);
            sliders_[i].setMouseDragSensitivity(getSliderDraggingDistance(font_size));
            const auto l_padding = (i == 1 || i == 3) ? 3 * padding : padding;
            bound.removeFromLeft(l_padding);
            label_bound.removeFromLeft(l_padding);
        }
    }

    // void ControlPanel::paintOverChildren(juce::Graphics& g) {
    //     for (const auto& divider : dividers_) {
    //         juce::ColourGradient gradient(base_.getTextColour().withAlpha(0.f), divider.getTopLeft(),
    //                                       base_.getTextColour().withAlpha(0.f), divider.getBottomLeft(), false);
    //         gradient.addColour(.5, base_.getTextColour().withAlpha(.22f));
    //         g.setGradientFill(gradient);
    //         g.fillRect(divider);
    //     }
    // }
}
