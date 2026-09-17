// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.


#include "gain_panel.hpp"

namespace zlpanel {
    GainPanel::GainPanel(PluginProcessor& p, zlgui::UIBase& base, multilingual::TooltipHelper& helper) :
        base_(base),
        gain_background_panel(base),
        gain_footer_panel(p, base, helper) {
        gain_background_panel.setBufferedToImage(true);
        addAndMakeVisible(gain_background_panel);
        gain_footer_panel.setBufferedToImage(true);
        addAndMakeVisible(gain_footer_panel);
    }

    int GainPanel::getIdealWidth() const {
        const auto font_size = base_.getFontSize();
        return getSliderWidth(font_size);
    }

    void GainPanel::resized() {
        auto bound = getLocalBounds();
        gain_footer_panel.setBounds(bound.removeFromBottom(gain_footer_panel.getIdealHeight()));
        gain_background_panel.setBounds(bound);
    }

    void GainPanel::repaintCallBackSlow() {
        const auto is_mouse_over = isMouseOverOrDragging(true);
        if (is_mouse_over != is_mouse_over_) {
            is_mouse_over_ = is_mouse_over;
            gain_background_panel.setMouseOver(is_mouse_over);
            gain_background_panel.repaint();
        }
        gain_footer_panel.repaintCallBackSlow();
    }
}
