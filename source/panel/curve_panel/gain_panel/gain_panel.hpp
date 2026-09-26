// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include "../../multilingual/tooltip_helper.hpp"
#include "gain_background_panel.hpp"
#include "gain_footer_panel.hpp"
#include "gain_display_panel.hpp"

namespace zlpanel {
    class GainPanel final : public juce::Component {
    public:
        explicit GainPanel(PluginProcessor& p, zlgui::UIBase& base,
                           const multilingual::TooltipHelper& tooltip_helper);

        int getIdealWidth() const;

        void resized() override;

        void repaintCallBackSlow();

    private:
        zlgui::UIBase& base_;
        GainBackgroundPanel gain_background_panel;
        GainFooterPanel gain_footer_panel;
        GainDisplayPanel gain_display_panel;

        bool is_mouse_over_{false};
    };
}
