// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../PluginProcessor.hpp"
#include "../../gui/gui.hpp"
#include "../multilingual/tooltip_helper.hpp"
#include "../helper/helper.hpp"

namespace zlpanel {
    class GainFooterPanel final : public juce::Component {
    public:
        explicit GainFooterPanel(PluginProcessor& p, zlgui::UIBase& base, multilingual::TooltipHelper&);

        void paint(juce::Graphics& g) override;

        int getIdealHeight() const;

        void resized() override;

        void repaintCallBackSlow();

    private:
        zlgui::UIBase& base_;
        zlgui::attachment::ComponentUpdater updater_;

        zlgui::combobox::CompactCombobox link_box_;
        zlgui::attachment::ComboBoxAttachment<true> link_attach_;

        zlgui::slider::CompactLinearSlider<false, false, false> input_slider_;
        zlgui::attachment::SliderAttachment<true> input_attach_;
    };
}
