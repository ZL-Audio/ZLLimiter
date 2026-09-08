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
#include "../../gui/gui.hpp"

namespace zlpanel {
    class CreditPanel final : public juce::Component {
    public:
        explicit CreditPanel(zlgui::UIBase& base);

        void paint(juce::Graphics& g) override;

        int getIdealHeight() const;

    private:
        zlgui::UIBase& base_;

        static constexpr auto kText =
            "ZL Limiter is Free and Open-source. ZL Limiter is licensed under AGPLv3, except for the logo of ZL Audio and the logo of ZL Limiter. You can obtain the corresponding source code at https://github.com/ZL-Audio/ZLLimiter or https://gitee.com/ZL-Audio/ZLLimiter.\n\n"
            "Copyright (c) 2025-2026 [zsliu98](https://github.com/zsliu98)\n\n"
            "JUCE framework from [JUCE](https://github.com/juce-framework/JUCE)\n\n"
            "JUCE template from [pamplejuce](https://github.com/sudara/pamplejuce)\n\n"
            "[Highway](https://github.com/google/highway) by [Google](https://github.com/google)\n\n"
            "[Material Symbols](https://github.com/google/material-design-icons) by [Google](https://github.com/google)\n\n"
            "[inter](https://github.com/rsms/inter) by [The Inter Project Authors](https://github.com/rsms/inter)\n\n"
            "Yuriy Ivantsov. On the ideal bilinear and biquadratic digital filter. (2025).";

        [[nodiscard]] juce::TextLayout getTipTextLayout(const juce::String& text,
                                                        const float w, const float h) const {
            juce::AttributedString s;
            s.setJustification(juce::Justification::topLeft);
            s.append(text, juce::FontOptions(base_.getFontSize() * 1.5f), base_.getTextColour());
            juce::TextLayout tl;
            tl.createLayout(s, w, h);
            return tl;
        }
    };
}
