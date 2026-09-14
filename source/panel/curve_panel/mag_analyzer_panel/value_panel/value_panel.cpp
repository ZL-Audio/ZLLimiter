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
        value_display_panel_(p, base),
        value_background_panel_(p, base),
        true_peak_on_(*p.parameters_NA_.getRawParameterValue(zlstate::PValueTruePeakON::kID),
                      zlstate::PValueTruePeakON::kDefaultV),
        corr_on_(*p.parameters_NA_.getRawParameterValue(zlstate::PValueStereoCorrON::kID),
                 zlstate::PValueStereoCorrON::kDefaultV),
        lufsm_on_(*p.parameters_NA_.getRawParameterValue(zlstate::PValueLUFSMON::kID),
                  zlstate::PValueLUFSMON::kDefaultV),
        lufss_on_(*p.parameters_NA_.getRawParameterValue(zlstate::PValueLUFSSON::kID),
                  zlstate::PValueLUFSSON::kDefaultV),
        lra_on_(*p.parameters_NA_.getRawParameterValue(zlstate::PValueLRAON::kID),
                zlstate::PValueLRAON::kDefaultV),
        lufsi_on_(*p.parameters_NA_.getRawParameterValue(zlstate::PValueLUFSION::kID),
                  zlstate::PValueLUFSION::kDefaultV) {
        value_background_panel_.setBufferedToImage(true);
        addAndMakeVisible(value_background_panel_);
        value_display_panel_.setBufferedToImage(true);
        addAndMakeVisible(value_display_panel_);
    }

    ValuePanel::~ValuePanel() = default;

    int ValuePanel::getIdealWidth() const {
        return juce::roundToInt(base_.getFontSize() * 10.f);
    }

    void ValuePanel::resized() {
        const auto font_size = base_.getFontSize();
        auto bound = getLocalBounds();
        value_background_panel_.setBounds(getLocalBounds());
        bound.removeFromTop(getTopPanelHeight(font_size));
        value_display_panel_.setBounds(bound);
    }

    void ValuePanel::repaintCallBackSlow() {
        bool to_repaint = true_peak_on_.update();
        to_repaint = corr_on_.update() || to_repaint;
        to_repaint = lufsm_on_.update() || to_repaint;
        to_repaint = lufss_on_.update() || to_repaint;
        to_repaint = lra_on_.update() || to_repaint;
        to_repaint = lufsi_on_.update() || to_repaint;
        const std::array<bool, 6> value_on{
            true_peak_on_.value, corr_on_.value, lufsm_on_.value,
            lufss_on_.value, lra_on_.value, lufsi_on_.value
        };
        value_background_panel_.repaintCallBackSlow(value_on, to_repaint);
        value_display_panel_.repaintCallBackSlow(value_on, to_repaint);
    }
}
