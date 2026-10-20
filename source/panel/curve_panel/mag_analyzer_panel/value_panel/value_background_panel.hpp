// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include "../../../../state/ui_state.hpp"

#include "../../../../PluginProcessor.hpp"
#include "../../../../gui/gui.hpp"
#include "../../../common/setting_label.hpp"

namespace zlpanel {
    class ValueBackgroundPanel final : public juce::Component {
    public:
        explicit ValueBackgroundPanel(PluginProcessor& p, zlgui::UIState& base);

        void paint(juce::Graphics& g) override;

        void repaintCallBackSlow(const std::array<bool, 6>& value_on, bool to_repaint);

        void resized() override;

    private:
        zlgui::UIState& base_;
        SettingLabel label_;

        std::array<bool, 6> value_on_{true, true, true, true, true, true};
    };
}
