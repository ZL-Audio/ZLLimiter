// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include "../../../state/ui_state.hpp"

#include "../../../PluginProcessor.hpp"
#include "../../../gui/gui.hpp"
#include "../../multilingual/tooltip_helper.hpp"
#include "../../common/panel_background.hpp"

namespace zlpanel {
    class DitherSettingLabel final : public juce::Component,
                                       public juce::SettableTooltipClient,
                                       private juce::ValueTree::Listener {
    public:
        explicit DitherSettingLabel(PluginProcessor& p, zlgui::UIState& base);

        ~DitherSettingLabel() override;

        void resized() override;

        void repaintCallBackSlow();

    private:
        zlgui::UIState& base_;
        std::atomic<float>& dither_mode_ref_;
        std::atomic<float>& dither_bit_ref_;
        float dither_mode_idx_{-1.f};
        float dither_bit_idx_{-1.f};

        PanelBackground control_background_;

        zlgui::label::NameLookAndFeel label_laf_;
        juce::Label label_;

        bool is_over_{false};

        void mouseDown(const juce::MouseEvent&) override;

        void mouseEnter(const juce::MouseEvent&) override;

        void mouseExit(const juce::MouseEvent&) override;

        void valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier& property) override;

        void updateAlpha(bool is_panel_open);
    };
}
