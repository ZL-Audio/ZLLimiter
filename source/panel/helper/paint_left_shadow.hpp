// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace zlpanel {
    inline void paintLeftShadow(juce::Graphics& g, const juce::Rectangle<float> bound,
                                const juce::Colour background_colour) {
        juce::ColourGradient gradient;
        gradient.point1 = bound.getTopLeft();
        gradient.point2 = bound.getTopRight();

        gradient.addColour(0.0, background_colour.withAlpha(.75f));
        gradient.addColour(1.0, juce::Colours::transparentBlack);
        g.setGradientFill(gradient);
        g.fillRect(bound);
    }
}
