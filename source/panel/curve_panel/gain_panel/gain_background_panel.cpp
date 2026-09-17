// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.


#include "gain_background_panel.hpp"

#include "../../helper/panel_constants.hpp"
#include "../../helper/paint_left_shadow.hpp"

namespace zlpanel {
    GainBackgroundPanel::GainBackgroundPanel(zlgui::UIBase& base) :
        base_(base) {
        setInterceptsMouseClicks(false, false);
    }

    void GainBackgroundPanel::paint(juce::Graphics& g) {
        const auto font_size = base_.getFontSize();
        const auto padding = static_cast<float>(getPaddingSize(font_size));

        auto bound = getLocalBounds().toFloat();
        paintLeftShadow(g, bound, base_.getBackgroundColour());

        bound.removeFromLeft(padding * .5f);
        bound = bound.removeFromLeft(padding * .5f);

        const auto filled_colour = is_mouse_over_
            ? base_.getTextColour()
            : base_.getColourBlendedWithBackground(base_.getTextColour(), .5f);
        {
            const auto g_bound = bound.removeFromBottom(padding);
            juce::ColourGradient gradient;
            gradient.point1 = g_bound.getTopLeft();
            gradient.point2 = g_bound.getBottomLeft();
            gradient.isRadial = false;

            gradient.addColour(0.0, filled_colour);
            gradient.addColour(1.0, juce::Colours::transparentBlack);
            g.setGradientFill(gradient);
            g.fillRect(g_bound);
        }
        {
            const auto g_bound = bound.removeFromTop(padding);
            juce::ColourGradient gradient;
            gradient.point1 = g_bound.getTopLeft();
            gradient.point2 = g_bound.getBottomLeft();
            gradient.isRadial = false;

            gradient.addColour(0.0, juce::Colours::transparentBlack);
            gradient.addColour(1.0, filled_colour);
            g.setGradientFill(gradient);
            g.fillRect(g_bound);
        }
        {
            g.setColour(filled_colour);
            g.fillRect(bound);
        }
        bound.removeFromTop(padding);
        bound.removeFromBottom(padding);
        static constexpr std::array ps{.2f, .4f, .6f, .8f};
        static constexpr std::array gains{18, 12, 6, 0};
        const auto tick_x = bound.getRight();
        const auto tick_height = .5f * padding;
        const auto tick_width = 1.5f * padding;
        const auto text_x = tick_x + 3.f * padding;
        const auto text_height = 3.f * font_size;
        g.setColour(filled_colour);
        g.setFont(1.25f * font_size);
        for (size_t i = 0; i < ps.size(); ++i) {
            const auto y = bound.getY() + bound.getHeight() * ps[i];
            g.fillRect(juce::Rectangle{tick_x, y - tick_height * .5f, tick_width, tick_height});
            if (is_mouse_over_) {
                g.drawText(juce::String(gains[i]),
                           juce::Rectangle{text_x, y - text_height * .5f, text_height, text_height},
                           juce::Justification::centredLeft);
            }
        }
    }

    void GainBackgroundPanel::setMouseOver(const bool is_mouse_over) {
        is_mouse_over_ = is_mouse_over;
    }
}
