// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#include "main_panel.hpp"

namespace zlpanel {
    MainPanel::MainPanel(PluginProcessor& processor, zlgui::UIBase& base) :
        p_ref_(processor), base_(base),
        tooltip_helper_(
            static_cast<multilingual::TooltipLanguage>(std::round(
                p_ref_.state_.getRawParameterValue(zlstate::PTooltipLang::kID)->load(std::memory_order::relaxed)))
            ),
        curve_panel_(processor, base_, tooltip_helper_),
        control_panel_(processor, base_, tooltip_helper_),
        preset_browser_(processor, base_),
        ui_setting_panel_(processor, base_),
        tooltipLAF(base_), tooltipWindow(base_, this),
        refresh_handler_(zlstate::PTargetRefreshSpeed::kRates[base_.getRefreshRateID()]) {
        juce::ignoreUnused(base_);
        addAndMakeVisible(curve_panel_);
        addAndMakeVisible(control_panel_);
        addChildComponent(ui_setting_panel_);
        preset_browser_.setBufferedToImage(true);
        addChildComponent(preset_browser_);
        preset_browser_.toFront(false);

        tooltipWindow.setLookAndFeel(&tooltipLAF);
        tooltipWindow.setOpaque(false);
        tooltipWindow.setBufferedToImage(true);

        base_.getPanelValueTree().addListener(this);

        startTimerHz(10);
    }

    MainPanel::~MainPanel() {
        base_.getPanelValueTree().removeListener(this);
        stopTimer();
    }

    void MainPanel::resized() {
        auto bound = getLocalBounds();

        const auto max_font_size = std::min(static_cast<float>(bound.getWidth()) * kFontSizeOverWidth,
                                            static_cast<float>(bound.getHeight()) / 16.f);
        const auto font_size = base_.getFontMode() == 0
            ? max_font_size * std::clamp(base_.getFontScale(), 0.25f, 0.9f)
            : std::clamp(base_.getStaticFontSize(), max_font_size * .25f, max_font_size * 0.9f);
        base_.setFontSize(font_size);
        const auto main_bound = bound;

        control_panel_.setBounds(0, bound.getBottom() - control_panel_.getIdealHeight(),
                                 control_panel_.getIdealWidth(), control_panel_.getIdealHeight());

        curve_panel_.setBounds(bound);

        const auto padding = getPaddingSize(font_size);
        const auto setting_width = juce::jmax(0, juce::jmin(ui_setting_panel_.getIdealWidth(),
                                                            main_bound.getWidth() - 4 * padding));
        const auto setting_height = juce::jmax(0, juce::jmin(ui_setting_panel_.getIdealHeight(),
                                                             main_bound.getHeight() - 4 * padding));
        ui_setting_panel_.setBounds(main_bound.withSizeKeepingCentre(setting_width, setting_height));

        const auto preset_width = juce::jmax(0, juce::jmin(preset_browser_.getIdealWidth(),
                                                           main_bound.getWidth() - 4 * padding));
        const auto preset_height = juce::jmax(0, juce::jmin(preset_browser_.getIdealHeight(),
                                                            main_bound.getHeight() - 4 * padding));
        preset_browser_.setBounds(main_bound.withSizeKeepingCentre(preset_width, preset_height));
    }

    void MainPanel::repaintCallBack(const double time_stamp) {
        const auto target = zlstate::PTargetRefreshSpeed::kRates[base_.getRefreshRateID()];
        if (std::abs(target - target_refresh_rate_) > .01) {
            target_refresh_rate_ = target;
            refresh_handler_ = RefreshHandler(target);
        }
        if (refresh_handler_.tick(time_stamp)) {
            if (time_stamp - previous_time_stamp_ > 0.1) {
                previous_time_stamp_ = time_stamp;
                control_panel_.repaintCallBackSlow();
                curve_panel_.repaintCallBackSlow();
            }

            if (ui_setting_panel_.isVisible()) {
                ui_setting_panel_.flushPendingScroll();
            }
            if (preset_browser_.isVisible()) {
                preset_browser_.flushPendingScroll();
            }
            curve_panel_.repaintCallBack(time_stamp);
        }
    }

    void MainPanel::valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier& property) {
        if (base_.isPanelIdentifier(zlgui::PanelSettingIdx::kUISettingPanel, property)) {
            const auto ui_setting_visibility = static_cast<bool>(base_.getPanelProperty(
                zlgui::PanelSettingIdx::kUISettingPanel));
            ui_setting_panel_.setVisible(ui_setting_visibility);
            if (ui_setting_visibility) {
                ui_setting_panel_.toFront(false);
            }
        }
    }

    void MainPanel::timerCallback() {
        if (juce::Process::isForegroundProcess()) {
            if (getCurrentlyFocusedComponent() != this) {
                grabKeyboardFocus();
            }
            stopTimer();
        }
    }
}
