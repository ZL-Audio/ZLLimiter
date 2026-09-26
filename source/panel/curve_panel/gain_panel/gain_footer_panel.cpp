// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.


#include "gain_footer_panel.hpp"

#include "../../helper/panel_constants.hpp"
#include "../../helper/paint_left_shadow.hpp"

namespace zlpanel {
    GainFooterPanel::GainFooterPanel(PluginProcessor& p, zlgui::UIBase& base,
                                     const multilingual::TooltipHelper& tooltip_helper) :
        base_(base), updater_(),
        link_box_(zlp::PInputOutputLink::kChoices, base,
                  tooltip_helper.getToolTipText(multilingual::kInOutLinkType)),
        link_attach_(link_box_.getBox(), p.parameters_, zlp::PInputOutputLink::kID, updater_),
        input_slider_("", base,
                      tooltip_helper.getToolTipText(multilingual::kInputGain)),
        input_attach_(input_slider_.getSlider(), p.parameters_, zlp::PInputGain::kID, updater_) {
        addAndMakeVisible(link_box_);
        addAndMakeVisible(input_slider_);
    }

    void GainFooterPanel::paint(juce::Graphics& g) {
        const auto bound = getLocalBounds().toFloat();
        paintLeftShadow(g, bound, base_.getBackgroundColour());
    }

    int GainFooterPanel::getIdealHeight() const {
        const auto font_size = base_.getFontSize();
        const auto button_height = getButtonSize(font_size);
        const auto padding = getPaddingSize(font_size);
        return 2 * button_height + padding;
    }

    void GainFooterPanel::resized() {
        const auto font_size = base_.getFontSize();
        const auto button_height = getButtonSize(font_size);
        const auto padding = getPaddingSize(font_size);

        auto bound = getLocalBounds();
        bound.removeFromLeft(padding);
        bound.removeFromRight(padding);
        link_box_.setBounds(bound.removeFromTop(button_height));
        input_slider_.setBounds(bound.removeFromBottom(button_height));
    }

    void GainFooterPanel::repaintCallBackSlow() {
        updater_.updateComponents();
    }
}
