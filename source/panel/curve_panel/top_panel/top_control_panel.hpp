// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include "../../../PluginProcessor.hpp"
#include "../../../gui/gui.hpp"
#include "../../helper/helper.hpp"
#include "../../multilingual/tooltip_helper.hpp"

namespace zlpanel {
    class TopControlPanel final : public juce::Component {
    public:
        explicit TopControlPanel(PluginProcessor& p, zlgui::UIBase& base,
                                 multilingual::TooltipHelper& tooltip_helper);

        void resized() override;

        void repaintCallBackSlow();

    private:
        zlgui::UIBase& base_;
        zlgui::attachment::ComponentUpdater updater_;

        zlgui::label::NameLookAndFeel label_laf_;
        juce::Label input_label_, ceiling_label_, oversample_label_;

        zlgui::slider::CompactLinearSlider<false, false, false> input_slider_, ceiling_slider_;
        zlgui::attachment::SliderAttachment<true> input_attachment_, ceiling_attachment_;

        zlgui::combobox::CompactCombobox oversample_box_;
        zlgui::attachment::ComboBoxAttachment<true> oversample_attachment_;

        std::unique_ptr<juce::Drawable> true_peak_icon_;
        zlgui::button::ClickButton true_peak_button_;
        zlgui::attachment::ButtonAttachment<true> true_peak_attachment_;

        std::unique_ptr<juce::Drawable> delta_icon_;
        zlgui::button::ClickButton delta_button_;
        zlgui::attachment::ButtonAttachment<true> delta_attachment_;

        std::unique_ptr<juce::Drawable> bypass_icon_;
        zlgui::button::ClickButton bypass_button_;
        zlgui::attachment::ButtonAttachment<true> bypass_attachment_;
    };
}
