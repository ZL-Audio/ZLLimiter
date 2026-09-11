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
        top_labels_{
            juce::Label{"", "Main"},
            juce::Label{"", "Support"},
            juce::Label{"", "Stereo"},
        },
        labels_{
            juce::Label{"", "Lookahead"},
            juce::Label{"", "Recover"},
            juce::Label{"", "Attack"},
            juce::Label{"", "Release"},
            juce::Label{"", "Delta"}
        },
        sliders_{
            Rotary("", base, "", 1.25f),
            Rotary("", base, "", 1.25f),
            Rotary("", base, "", 1.25f),
            Rotary("", base, "", 1.25f),
            Rotary("", base, "", 1.25f)
        } {
        background_.setBufferedToImage(true);
        addAndMakeVisible(background_);

        label_laf_.setFontScale(1.5f);
        label_laf_.setMaximumNumberOfLines(2);
        const std::array ids{zlp::PLookahead::kID, zlp::PRecover::kID, zlp::PAttack::kID,
                             zlp::PRelease::kID, zlp::PChannelDelta::kID};

        for (auto& label : top_labels_) {
            label.setLookAndFeel(&label_laf_);
            label.setJustificationType(juce::Justification::centred);
            label.setBorderSize(juce::BorderSize<int>{0});
            label.setInterceptsMouseClicks(false, false);
            label.setBufferedToImage(true);
            addAndMakeVisible(label);
        }
        for (auto& label : labels_) {
            label.setLookAndFeel(&label_laf_);
            label.setJustificationType(juce::Justification::centred);
            label.setBorderSize(juce::BorderSize<int>{0});
            label.setInterceptsMouseClicks(false, false);
            label.setBufferedToImage(true);
            addAndMakeVisible(label);
        }

        for (size_t i = 0; i < sliders_.size(); ++i) {
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

        return slider_width + 2 * button_height + 3 * padding;
    }

    void ControlPanel::resized() {
        const auto font_size = base_.getFontSize();
        const auto padding = getPaddingSize(font_size);
        auto bound = getLocalBounds();
        background_.setBounds(bound);
        bound.reduce(2 * padding, padding);

        const auto width = getSliderWidth(font_size);
        {
            auto top_label_bound = bound.removeFromTop(getButtonSize(font_size));
            top_labels_[0].setBounds(top_label_bound.removeFromLeft(2 * width + padding));
            top_label_bound.removeFromLeft(3 * padding);
            top_labels_[1].setBounds(top_label_bound.removeFromLeft(2 * width + padding));
            top_label_bound.removeFromLeft(3 * padding);
            top_labels_[2].setBounds(top_label_bound.removeFromLeft(width));
        }
        auto label_bound = bound.removeFromTop(getButtonSize(font_size));
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

    void ControlPanel::repaintCallBackSlow() {
        updater_.updateComponents();
    }
}
