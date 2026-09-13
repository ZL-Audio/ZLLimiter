// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.


#include "value_panel.hpp"

namespace zlpanel {
    ValuePanel::ValuePanel(PluginProcessor& p, zlgui::UIBase& base) :
        base_(base),
        value_display_panel_(p, base) {
        addAndMakeVisible(value_display_panel_);
    }

    ValuePanel::~ValuePanel() {
    }

    int ValuePanel::getIdealWidth() const {
        return juce::roundToInt(base_.getFontSize() * 12.f);
    }

    void ValuePanel::resized() {
        value_display_panel_.setBounds(getLocalBounds());
    }

    void ValuePanel::repaintCallBackSlow() {
        value_display_panel_.repaintCallBackSlow();
    }
}
