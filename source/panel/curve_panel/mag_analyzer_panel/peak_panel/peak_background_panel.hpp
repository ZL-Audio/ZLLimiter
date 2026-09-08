// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include "../../../../PluginProcessor.hpp"
#include "../../../helper/helper.hpp"
#include "../../../../gui/gui.hpp"

namespace zlpanel {
    class PeakBackgroundPanel final : public juce::Component {
    public:
        explicit PeakBackgroundPanel(PluginProcessor& p, zlgui::UIBase& base);

        void paint(juce::Graphics& g) override;

        void repaintCallBackSlow();

    private:
        zlgui::UIBase& base_;
        std::atomic<float>& mag_min_db_id_ref_;
        float mag_min_db_id_{-1.f};
        float mag_range_db_{-54.f};
    };
}
