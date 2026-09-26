// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include "../../PluginProcessor.hpp"
#include "../../gui/gui.hpp"
#include "../multilingual/tooltip_helper.hpp"
#include "panel_background.hpp"

namespace zlpanel {
    class SettingLabel final : public juce::Component,
                               public juce::SettableTooltipClient,
                               private juce::ValueTree::Listener {
    public:
        explicit SettingLabel(PluginProcessor&, zlgui::UIBase& base,
                              juce::String label, zlgui::PanelSettingIdx setting_idx,
                              const juce::String& tooltip_text = "");

        ~SettingLabel() override;

        void resized() override;

    private:
        zlgui::UIBase& base_;
        zlgui::PanelSettingIdx setting_idx_;

        PanelBackground control_background_;

        zlgui::label::NameLookAndFeel label_laf_;
        juce::Label setting_label_;

        bool is_over_{false};

        void mouseDown(const juce::MouseEvent&) override;

        void mouseEnter(const juce::MouseEvent&) override;

        void mouseExit(const juce::MouseEvent&) override;

        void valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier& property) override;

        void updateAlpha(bool is_panel_open);
    };
}
