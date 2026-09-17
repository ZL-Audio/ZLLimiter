// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include "../../gui/gui.hpp"

namespace zlpanel {
    class ControlBackground final : public juce::Component {
    public:
        /**
         *
         * @param base
         * @param alpha shadow colour alpha
         */
        explicit ControlBackground(zlgui::UIBase& base, float alpha = .5f);

        void paint(juce::Graphics& g) override;

    private:
        zlgui::UIBase& base_;
        const float alpha_;

        void drawSplit(juce::Graphics& g, juce::Rectangle<float> bound) const;
    };
}
