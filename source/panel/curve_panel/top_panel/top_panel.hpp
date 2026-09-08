// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include "logo_panel.hpp"
#include "top_control_panel.hpp"
#include "analyzer_label.hpp"

namespace zlpanel {
    class TopPanel final : public juce::Component {
    public:
        explicit TopPanel(PluginProcessor& p, zlgui::UIBase& base,
                          multilingual::TooltipHelper& tooltip_helper);

        void paint(juce::Graphics& g) override;

        void resized() override;

        int getIdealHeight() const;

        void repaintCallBackSlow();

    private:
        zlgui::UIBase& base_;
        LogoPanel logo_panel_;
        const std::unique_ptr<juce::Drawable> preset_drawable_;
        zlgui::button::ClickButton preset_button_;
        AnalyzerLabel analyzer_label_;
        TopControlPanel top_control_panel_;
    };
}
