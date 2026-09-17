// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#include "value_background_panel.hpp"

#include "../../../helper/panel_constants.hpp"

namespace zlpanel {
    ValueBackgroundPanel::ValueBackgroundPanel(PluginProcessor& p, zlgui::UIBase& base) :
        base_(base),
        label_(p, base, "Value", zlgui::PanelSettingIdx::kValueSettingPanel) {
        label_.setBufferedToImage(true);
        addAndMakeVisible(label_);

        setInterceptsMouseClicks(false, true);
    }

    void ValueBackgroundPanel::paint(juce::Graphics& g) {
        auto bound = getLocalBounds().toFloat();
        const auto font_size = base_.getFontSize();
        bound.removeFromTop(static_cast<float>(getTopPanelHeight(font_size)));

        const auto thickness = base_.getFontSize() * 0.125f;
        g.setColour(base_.getTextColour().withAlpha(.1f));
        for (const auto scale : {0.f, 1.f, 2.f, 3.f, 4.f, 5.f}) {
            const auto y = bound.getHeight() * scale / 6.f + bound.getY() - thickness * .5f;
            const auto rect = juce::Rectangle<float>({bound.getX(), y, bound.getWidth(), thickness});
            g.fillRect(rect);
        }

        const auto height = bound.getHeight() / 12.f;
        g.setFont(base_.getFontSize() * 1.75f);
        g.setColour(base_.getTextColour());

        const std::array<juce::String, 6> labels = {
            "True Peak", "Correlation", "LUFS-M", "LUFS-S", "LRA", "LUFS-I"
        };
        for (size_t i = 0; i < labels.size(); ++i) {
            if (value_on_[i]) {
                g.drawText(labels[i], bound.removeFromTop(height), juce::Justification::centred, false);
                bound.removeFromTop(height);
            }
        }
    }

    void ValueBackgroundPanel::repaintCallBackSlow(const std::array<bool, 6>& value_on, bool to_repaint) {
        value_on_ = value_on;
        if (to_repaint) {
            repaint();
        }
    }

    void ValueBackgroundPanel::resized() {
        const auto font_size = base_.getFontSize();
        const auto padding = getPaddingSize(font_size);
        const auto bound = getLocalBounds().removeFromTop(getTopPanelHeight(font_size));

        label_.setBounds(bound.withSizeKeepingCentre(juce::roundToInt(font_size * 8.f),
                                                     bound.getHeight() - (padding / 2) * 2));
    }
}
