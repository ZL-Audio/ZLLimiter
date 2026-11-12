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
#include "../../common/panel_background.hpp"

namespace zlpanel {
    class DitherSettingPanel final : public juce::Component, private juce::ValueTree::Listener {
    public:
        DitherSettingPanel(PluginProcessor& p, zlgui::UIState& base);

        ~DitherSettingPanel() override;

        void resized() override;

        int getIdealWidth() const;

        int getIdealHeight() const;

        void repaintCallBackSlow();

    private:
        zlgui::UIState& base_;
        zlgui::attachment::ComponentUpdater updater_;
        PanelBackground background_;
        zlgui::label::NameLookAndFeel label_laf_;
        std::array<juce::Label, 2> labels_;
        std::array<zlgui::combobox::CompactCombobox, 2> boxes_;
        std::array<std::unique_ptr<zlgui::attachment::ComboBoxAttachment<true>>, 2> box_attachments_;

        void valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier& property) override;
    };
}
