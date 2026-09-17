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

#include "../../../PluginProcessor.hpp"
#include "../../../gui/gui.hpp"
#include "../../multilingual/tooltip_helper.hpp"

namespace zlpanel {
    class GainDisplayPanel final : public juce::Component,
                                   private juce::Slider::Listener {
    public:
        explicit GainDisplayPanel(PluginProcessor& p, zlgui::UIBase& base, multilingual::TooltipHelper&);

        ~GainDisplayPanel() override;

        void paint(juce::Graphics& g) override;

        void resized() override;

        void repaintCallBackSlow();

        void setMouseOver(bool is_mouse_over);

        void mouseUp(const juce::MouseEvent& event) override;

        void mouseDown(const juce::MouseEvent& event) override;

        void mouseDrag(const juce::MouseEvent& event) override;

        void mouseEnter(const juce::MouseEvent& event) override;

        void mouseExit(const juce::MouseEvent& event) override;

        void mouseDoubleClick(const juce::MouseEvent& event) override;

        void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

    private:
        zlgui::UIBase& base_;
        zlgui::attachment::ComponentUpdater updater_;

        zlgui::slider::SnappingSlider gain_slider_;
        zlgui::attachment::SliderAttachment<true> gain_slider_attach_;

        bool is_mouse_over_{false};

        int drag_distance_{10};

        void sliderValueChanged(juce::Slider*) override;

        void updateDragDistance(bool is_shift_pressed);
    };
}
