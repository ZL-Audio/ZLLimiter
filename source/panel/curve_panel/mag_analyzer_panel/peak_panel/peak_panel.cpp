// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#include "peak_panel.hpp"

namespace zlpanel {
    PeakPanel::PeakPanel(PluginProcessor& p, zlgui::UIBase& base) :
        peak_background_panel_(p, base),
        peak_display_panel_(p, base) {
        peak_background_panel_.setBufferedToImage(true);
        addAndMakeVisible(peak_background_panel_);
        addAndMakeVisible(peak_display_panel_);
        setInterceptsMouseClicks(false, false);
    }

    void PeakPanel::resized() {
        const auto bounds = getLocalBounds();
        peak_background_panel_.setBounds(bounds);
        peak_display_panel_.setBounds(bounds);
    }

    void PeakPanel::repaintCallBackSlow() {
        peak_background_panel_.repaintCallBackSlow();
    }
}
