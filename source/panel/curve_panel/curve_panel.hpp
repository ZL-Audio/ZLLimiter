// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include "mag_analyzer_panel/peak_panel/peak_panel.hpp"
#include "mag_analyzer_panel/meter_panel/meter_panel.hpp"
#include "analyzer_setting_panel/analyzer_setting_panel.hpp"
#include "top_panel/top_panel.hpp"

#include "../multilingual/tooltip_helper.hpp"

namespace zlpanel {
    class CurvePanel final : public juce::Component, private juce::Thread {
    public:
        CurvePanel(PluginProcessor& p, zlgui::UIBase& base,
                   multilingual::TooltipHelper& tooltip_helper);

        ~CurvePanel() override;

        void paint(juce::Graphics& g) override;

        void paintOverChildren(juce::Graphics& g) override;

        void resized() override;

        void repaintCallBack(double time_stamp);

        void repaintCallBackSlow();

    private:
        PluginProcessor& p_ref_;
        zlgui::UIBase& base_;
        std::atomic<float>& min_db_ref_;
        TopPanel top_panel_;
        PeakPanel peak_panel_;
        MeterPanel meter_panel_;
        AnalyzerSettingPanel analyzer_setting_panel_;
        zldsp::analyzer::FIFOTransferBuffer<zlp::Controller::kAnalyzerStreamNum> transfer_buffer_;
        size_t peak_consumer_, meter_consumer_;
        uint64_t generation_{std::numeric_limits<uint64_t>::max()};
        std::atomic<double> next_stamp_{0.0};

        void run() override;

        void mouseDown(const juce::MouseEvent& event) override;
    };
}
