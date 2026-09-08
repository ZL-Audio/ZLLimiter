// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#include "analyzer_setting_panel.hpp"

namespace zlpanel {
    AnalyzerSettingPanel::AnalyzerSettingPanel(PluginProcessor& p, zlgui::UIBase& base) :
        base_(base), background_(base),
        boxes_{
            zlgui::combobox::CompactCombobox(zlstate::PAnalyzerMagType::kChoices, base),
            zlgui::combobox::CompactCombobox(zlstate::PAnalyzerMoveType::kChoices, base),
            zlgui::combobox::CompactCombobox(zlstate::PAnalyzerTimeLength::kChoices, base),
            zlgui::combobox::CompactCombobox([] {
                auto choices = zlstate::PAnalyzerMinDB::kChoices;
                for (auto& choice : choices) {
                    choice += " dB";
                }
                return choices;
            }(), base)
        },
        buttons_{
            zlgui::button::ClickTextButton(base, "Pre"),
            zlgui::button::ClickTextButton(base, "Post"),
            zlgui::button::ClickTextButton(base, "Delta")
        } {
        background_.setInterceptsMouseClicks(false, false);
        addAndMakeVisible(background_);
        const std::array box_ids{zlstate::PAnalyzerMagType::kID, zlstate::PAnalyzerMoveType::kID,
                                 zlstate::PAnalyzerTimeLength::kID, zlstate::PAnalyzerMinDB::kID};
        for (size_t i = 0; i < boxes_.size(); ++i) {
            boxes_[i].setScrollEnabled(true);
            boxes_[i].getBox().setComponentID(box_ids[i]);
            box_attachments_[i] = std::make_unique<zlgui::attachment::ComboBoxAttachment<true>>(
                boxes_[i].getBox(), p.parameters_NA_, box_ids[i], updater_);
            addAndMakeVisible(boxes_[i]);
        }
        const std::array button_ids{zlstate::PPreCurveDisplay::kID, zlstate::PPostCurveDisplay::kID,
                                    zlstate::PDeltaCurveDisplay::kID};
        for (size_t i = 0; i < buttons_.size(); ++i) {
            buttons_[i].getButton().setClickingTogglesState(true);
            buttons_[i].getButton().setComponentID(button_ids[i]);
            buttons_[i].getLAF().setFontScale(1.5f);
            buttons_[i].getLAF().setJustification(juce::Justification::centred);
            button_attachments_[i] = std::make_unique<zlgui::attachment::ButtonAttachment<true>>(
                buttons_[i].getButton(), p.parameters_NA_, button_ids[i], updater_);
            addAndMakeVisible(buttons_[i]);
        }
        updater_.updateComponents();
        base_.getPanelValueTree().addListener(this);
    }

    AnalyzerSettingPanel::~AnalyzerSettingPanel() {
        base_.getPanelValueTree().removeListener(this);
    }

    int AnalyzerSettingPanel::getIdealWidth() const {
        const auto font_size = base_.getFontSize();
        const auto padding = getPaddingSize(font_size);
        const auto slider_width = getSliderWidth(font_size);

        return 7 * padding + 3 * (slider_width / 2);
    }

    int AnalyzerSettingPanel::getIdealHeight() const {
        const auto font_size = base_.getFontSize();
        const auto padding = getPaddingSize(font_size);
        const auto button_height = getButtonSize(font_size);

        return 4 * padding + 3 * button_height;
    }

    void AnalyzerSettingPanel::resized() {
        background_.setBounds(getLocalBounds());
        const auto padding = getPaddingSize(base_.getFontSize());
        auto bound = getLocalBounds().reduced(2 * padding, padding);
        const auto height = getButtonSize(base_.getFontSize());
        {
            auto row = bound.removeFromTop(height);
            boxes_[0].setBounds(row.removeFromLeft(row.getWidth() / 2 + 2 * padding));
            boxes_[1].setBounds(row);
        }
        bound.removeFromTop(padding);
        {
            auto row = bound.removeFromTop(height);
            const auto width = row.getWidth() / 3 - padding / 2;
            buttons_[0].setBounds(row.removeFromLeft(width - padding / 3));
            buttons_[1].setBounds(row.removeFromLeft(width - padding / 3));
            buttons_[2].setBounds(row);
        }
        bound.removeFromTop(padding);
        {
            auto row = bound.removeFromTop(height);
            boxes_[2].setBounds(row.removeFromLeft(row.getWidth() / 2));
            boxes_[3].setBounds(row);
        }
    }

    void AnalyzerSettingPanel::valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier& property) {
        if (base_.isPanelIdentifier(zlgui::kAnalyzerSettingPanel, property)) {
            setVisible(static_cast<double>(base_.getPanelProperty(zlgui::kAnalyzerSettingPanel)) > .5);
        }
    }

    void AnalyzerSettingPanel::repaintCallBackSlow() {
        updater_.updateComponents();
    }
}
