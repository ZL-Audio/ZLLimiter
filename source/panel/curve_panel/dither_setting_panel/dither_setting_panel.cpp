// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#include "dither_setting_panel.hpp"

#include "../../helper/panel_constants.hpp"
#include "../../helper/combobox_helper.hpp"

namespace zlpanel {
    DitherSettingPanel::DitherSettingPanel(PluginProcessor& p, zlgui::UIState& base,
                                           const multilingual::TooltipHelper& tooltip_helper) :
        base_(base), background_(base), label_laf_(base.gui),
        labels_{
            juce::Label("Dither", "Dither"),
            juce::Label("Depth", "Depth")
        },
        boxes_{
            zlgui::combobox::CompactCombobox(zlp::PDitherMode::kChoices, base.gui,
                                             tooltip_helper.getToolTipText(multilingual::kDitherMode)),
            zlgui::combobox::CompactCombobox(zlp::PDitherBitDepth::kChoices, base.gui,
                                             tooltip_helper.getToolTipText(multilingual::kDitherBit))
        } {
        background_.setInterceptsMouseClicks(false, false);
        background_.setBufferedToImage(true);
        addAndMakeVisible(background_);

        label_laf_.setFontScale(1.5f);
        for (auto& label : labels_) {
            label.setLookAndFeel(&label_laf_);
            label.setJustificationType(juce::Justification::centredRight);
            label.setBufferedToImage(true);
            addAndMakeVisible(label);
        }

        constexpr std::array box_ids{zlp::PDitherMode::kID, zlp::PDitherBitDepth::kID};
        const auto box_alignment = combobox_helper::getAlignment(base_.gui.getComboboxAlignment());
        boxes_[0].getLAF().setAlignment(zlgui::combobox::Alignment::kCenter);
        boxes_[0].getLAF().setItemAlignment(box_alignment);
        boxes_[1].getLAF().setAlignment(zlgui::combobox::Alignment::kRight);
        boxes_[1].getLAF().setItemAlignment(zlgui::combobox::Alignment::kRight);
        for (size_t i = 0; i < boxes_.size(); ++i) {
            boxes_[i].setScrollEnabled(false);
            boxes_[i].getBox().setComponentID(box_ids[i]);
            box_attachments_[i] = std::make_unique<zlgui::attachment::ComboBoxAttachment<true>>(
                boxes_[i].getBox(), p.parameters_, box_ids[i], updater_);
            boxes_[i].setBufferedToImage(true);
            addAndMakeVisible(boxes_[i]);
        }

        updater_.updateComponents();
        base_.getPanelValueTree().addListener(this);
    }

    DitherSettingPanel::~DitherSettingPanel() {
        base_.getPanelValueTree().removeListener(this);
    }

    int DitherSettingPanel::getIdealWidth() const {
        const auto font_size = base_.gui.getFontSize();
        return 5 * getPaddingSize(font_size) + 2 * getButtonSize(font_size) + getSliderWidth(font_size);
    }

    int DitherSettingPanel::getIdealHeight() const {
        const auto font_size = base_.gui.getFontSize();
        return 3 * getPaddingSize(font_size) + 2 * getButtonSize(font_size);
    }

    void DitherSettingPanel::resized() {
        background_.setBounds(getLocalBounds());
        const auto font_size = base_.gui.getFontSize();
        const auto padding = getPaddingSize(font_size);
        const auto height = getButtonSize(font_size);
        auto bound = getLocalBounds().reduced(2 * padding, padding);
        for (size_t i = 0; i < boxes_.size(); ++i) {
            auto row = bound.removeFromTop(height);
            labels_[i].setBounds(row.removeFromLeft(2 * height));
            row.removeFromLeft(padding);
            boxes_[i].setBounds(row);
            bound.removeFromTop(padding);
        }
    }

    void DitherSettingPanel::valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier& property) {
        if (base_.isPanelIdentifier(zlgui::kDitherSettingPanel, property)) {
            setVisible(static_cast<double>(base_.getPanelProperty(zlgui::kDitherSettingPanel)) > .5);
        }
    }

    void DitherSettingPanel::repaintCallBackSlow() {
        updater_.updateComponents();
    }
}
