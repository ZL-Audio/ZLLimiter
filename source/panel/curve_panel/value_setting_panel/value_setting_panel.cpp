// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#include "value_setting_panel.hpp"

#include "../../helper/panel_constants.hpp"

namespace zlpanel {
    ValueSettingPanel::ValueSettingPanel(PluginProcessor& p, zlgui::UIBase& base) :
        base_(base), background_(base),
        click_buttons_{
            zlgui::button::ClickTextButton(base, "True Peak"),
            zlgui::button::ClickTextButton(base, "Correlation"),
            zlgui::button::ClickTextButton(base, "LUFS-M"),
            zlgui::button::ClickTextButton(base, "Histogram"),
            zlgui::button::ClickTextButton(base, "LUFS-S"),
            zlgui::button::ClickTextButton(base, "LRA"),
            zlgui::button::ClickTextButton(base, "LUFS-I")
        } {
        background_.setInterceptsMouseClicks(false, false);
        background_.setBufferedToImage(true);
        addAndMakeVisible(background_);

        constexpr std::array click_button_ids{
            zlstate::PValueTruePeakON::kID, zlstate::PValueStereoCorrON::kID,
            zlstate::PValueLUFSMON::kID, zlstate::PValueHistogramON::kID,
            zlstate::PValueLUFSSON::kID, zlstate::PValueLRAON::kID, zlstate::PValueLUFSION::kID
        };
        for (size_t i = 0; i < click_buttons_.size(); ++i) {
            click_buttons_[i].getButton().setClickingTogglesState(true);
            click_buttons_[i].getButton().setComponentID(click_button_ids[i]);
            click_buttons_[i].getLAF().setFontScale(1.5f);
            click_buttons_[i].getLAF().setJustification(juce::Justification::centred);
            click_button_attachments_[i] = std::make_unique<zlgui::attachment::ButtonAttachment<true>>(
                click_buttons_[i].getButton(), p.parameters_NA_, click_button_ids[i], updater_);
            click_buttons_[i].setBufferedToImage(true);
            addAndMakeVisible(click_buttons_[i]);
        }
        updater_.updateComponents();
        base_.getPanelValueTree().addListener(this);
    }

    ValueSettingPanel::~ValueSettingPanel() {
        base_.getPanelValueTree().removeListener(this);
    }

    int ValueSettingPanel::getIdealWidth() const {
        const auto font_size = base_.getFontSize();
        return 4 * getPaddingSize(font_size) + 5 * getSliderWidth(font_size) / 2;
    }

    int ValueSettingPanel::getIdealHeight() const {
        const auto font_size = base_.getFontSize();
        return 4 * getPaddingSize(font_size) + 3 * getButtonSize(font_size);
    }

    void ValueSettingPanel::resized() {
        background_.setBounds(getLocalBounds());
        const auto padding = getPaddingSize(base_.getFontSize());
        auto bound = getLocalBounds().reduced(2 * padding, padding);
        const auto height = getButtonSize(base_.getFontSize());
        size_t button_index = 0;
        for (const auto num_buttons : {2, 2, 3}) {
            auto row = bound.removeFromTop(height);
            for (auto remaining = num_buttons; remaining > 0; --remaining) {
                click_buttons_[button_index++].setBounds(row.removeFromLeft(row.getWidth() / remaining));
            }
            bound.removeFromTop(padding);
        }
    }

    void ValueSettingPanel::valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier& property) {
        if (base_.isPanelIdentifier(zlgui::kValueSettingPanel, property)) {
            setVisible(static_cast<double>(base_.getPanelProperty(zlgui::kValueSettingPanel)) > .5);
        }
    }

    void ValueSettingPanel::repaintCallBackSlow() {
        updater_.updateComponents();
    }
}
