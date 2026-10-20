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
#include "../../helper/panel_constants.hpp"
#include "../../multilingual/tooltip_helper.hpp"

namespace zlpanel {
    class TopControlPanel final : public juce::Component {
    public:
        explicit TopControlPanel(PluginProcessor& p, zlgui::UIState& base,
                                 const multilingual::TooltipHelper& tooltip_helper);

        void resized() override;

        void repaintCallBackSlow();

    private:
        zlgui::UIState& base_;
        zlgui::attachment::ComponentUpdater updater_;

        zlgui::label::NameLookAndFeel label_laf_;
        juce::Label ceiling_label_, oversample_label_;

        zlgui::slider::CompactLinearSlider<false, false, false> ceiling_slider_;
        zlgui::attachment::SliderAttachment<true> ceiling_attachment_;

        zlgui::combobox::CompactCombobox oversample_box_;
        zlgui::attachment::ComboBoxAttachment<true> oversample_attachment_;

        zlgui::button::ClickButton true_peak_button_;
        zlgui::attachment::ButtonAttachment<true> true_peak_attachment_;

        zlgui::button::ClickButton delta_button_;
        zlgui::attachment::ButtonAttachment<true> delta_attachment_;

        zlgui::button::ClickButton bypass_button_;
        zlgui::attachment::ButtonAttachment<true> bypass_attachment_;
    };
}
