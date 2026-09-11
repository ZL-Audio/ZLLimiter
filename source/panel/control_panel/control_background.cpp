// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#include "control_background.hpp"

namespace zlpanel {
    ControlBackground::ControlBackground(zlgui::UIBase& base, const float alpha,
                                         const std::array<bool, 4> hide_shadow) :
        base_(base), alpha_(alpha), hide_shadow_(hide_shadow) {
        setInterceptsMouseClicks(false, false);
        setAlpha(.9f);
    }

    void ControlBackground::paint(juce::Graphics& g) {
        auto original_bound = getLocalBounds();
        if (hide_shadow_[0] || hide_shadow_[2]) {
            const auto width = original_bound.getWidth();
            original_bound.setWidth(original_bound.getWidth() * 2);
            if (hide_shadow_[0] && hide_shadow_[2]) {
                original_bound.setX(-width / 2);
            } else if (hide_shadow_[0]) {
                original_bound.setX(-width);
            }
        }
        if (hide_shadow_[1] || hide_shadow_[3]) {
            const auto height = original_bound.getHeight();
            original_bound.setHeight(original_bound.getHeight() * 2);
            if (hide_shadow_[1] && hide_shadow_[3]) {
                original_bound.setY(-height / 2);
            } else if (hide_shadow_[1]) {
                original_bound.setY(-height);
            }
        }
        {
            const auto padding = getPaddingSize(base_.getFontSize());
            const auto bound = original_bound.reduced(padding);
            juce::Path path;
            path.addRoundedRectangle(bound.toFloat(), static_cast<float>(padding));

            const juce::DropShadow shadow{base_.getTextColour().withAlpha(alpha_), padding, {0, 0}};
            shadow.drawForPath(g, path);
            g.setColour(base_.getBackgroundColour());
            g.fillPath(path);
        }
        {
            const auto font_size = base_.getFontSize();
            const auto padding = getPaddingSize(font_size);
            const auto width = getSliderWidth(font_size);

            auto bound = getLocalBounds();
            bound.reduce(2 * padding, padding);

            bound.removeFromLeft(2 * (width + padding));
            drawSplit(g, bound.removeFromLeft(padding).toFloat());
            bound.removeFromLeft(2 * (width + padding) + padding);
            drawSplit(g, bound.removeFromLeft(padding).toFloat());
        }
    }

    void ControlBackground::drawSplit(juce::Graphics& g, juce::Rectangle<float> bound) const {
        const auto split_colour = base_.getTextColour().withMultipliedAlpha(.25f);

        const auto font_size = base_.getFontSize();
        const auto tb_padding = font_size * kPaddingScale * 2.f;
        const auto lr_padding = font_size * kPaddingScale * .3f;

        bound.removeFromLeft(lr_padding);
        bound.removeFromRight(lr_padding);
        const auto top_bound = bound.removeFromTop(tb_padding);
        const auto bottom_bound = bound.removeFromBottom(tb_padding);

        g.setColour(split_colour);
        g.fillRect(bound);

        auto gradient = juce::ColourGradient{};
        gradient.addColour(0.0, juce::Colours::transparentWhite);
        gradient.addColour(1.0, split_colour);

        gradient.point1 = top_bound.getTopLeft();
        gradient.point2 = top_bound.getBottomLeft();
        g.setGradientFill(gradient);
        g.fillRect(top_bound);

        gradient.point1 = bottom_bound.getBottomLeft();
        gradient.point2 = bottom_bound.getTopLeft();
        g.setGradientFill(gradient);
        g.fillRect(bottom_bound);
    }
}
