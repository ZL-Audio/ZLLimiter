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
#include "../helper/helper.hpp"
#include "../multilingual/tooltip_helper.hpp"
#include "control_background.hpp"

namespace zlpanel {
    class ControlPanel final : public juce::Component {
    public:
        ControlPanel(PluginProcessor& p, zlgui::UIBase& base, multilingual::TooltipHelper&);

        void resized() override;

        int getIdealWidth() const;

        int getIdealHeight() const;

        void repaintCallBackSlow() {
            updater_.updateComponents();
        }

    private:
        using Rotary = zlgui::slider::TwoValueRotarySlider<false, false, false>;
        zlgui::UIBase& base_;
        zlgui::attachment::ComponentUpdater updater_;
        ControlBackground background_;
        zlgui::label::NameLookAndFeel label_laf_;
        std::array<juce::Label, 5> labels_;
        std::array<Rotary, 5> sliders_;
        std::array<std::unique_ptr<zlgui::attachment::SliderAttachment<true>>, 5> attachments_;
    };
}
