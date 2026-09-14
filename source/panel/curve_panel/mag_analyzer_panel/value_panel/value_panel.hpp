// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.


#pragma once

#include "value_display_panel.hpp"
#include "value_background_panel.hpp"

namespace zlpanel {
    class ValuePanel final : public juce::Component {
    public:
        explicit ValuePanel(PluginProcessor& p, zlgui::UIBase& base);

        ~ValuePanel() override;

        ValueDisplayPanel& getDisplayPanel() {
            return value_display_panel_;
        }

        int getIdealWidth() const;

        void resized() override;

        void repaintCallBackSlow();

    private:
        zlgui::UIBase& base_;
        ValueDisplayPanel value_display_panel_;
        ValueBackgroundPanel value_background_panel_;

        struct AtomicBool {
            std::atomic<float>& ref;
            bool value;

            bool update() {
                const auto v = ref.load(std::memory_order::relaxed) > .5f;
                if (v != value) {
                    value = v;
                    return true;
                } else {
                    return false;
                }
            }
        };

        AtomicBool true_peak_on_;
        AtomicBool corr_on_;
        AtomicBool lufsm_on_;
        AtomicBool lufss_on_;
        AtomicBool lra_on_;
        AtomicBool lufsi_on_;
    };
}
