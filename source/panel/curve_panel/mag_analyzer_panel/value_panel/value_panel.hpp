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
#include "../../../../chore/thread/notifier.hpp"
#include "../../../../gui/gui.hpp"
#include "../../../helper/helper.hpp"
#include "../../../../dsp/analyzer/mag_analyzer/magnitude_receiver.hpp"
#include "../../../../dsp/loudness/lufs_meter.hpp"


namespace zlpanel {
    class ValuePanel final : public juce::Component {
    public:
        explicit ValuePanel(PluginProcessor& p, zlgui::UIBase& base);

        ~ValuePanel() override;

        int getIdealWidth() const;

        void resized() override;

        void repaintCallBackSlow();

    private:
        zlgui::UIBase& base_;
    };
}
