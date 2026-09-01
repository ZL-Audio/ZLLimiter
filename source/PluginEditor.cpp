// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#include "PluginEditor.hpp"

PluginEditor::PluginEditor(PluginProcessor& p) :
    AudioProcessorEditor(&p),
    p_ref_(p),
    state_(dummy_processor_, nullptr,
           juce::Identifier(zlstate::schema::kUISettings),
           zlstate::getStateParameterLayout()),
    property_(state_) {
    // set font
    sendLookAndFeelChange();
}

PluginEditor::~PluginEditor() {
}

void PluginEditor::paint(juce::Graphics& g) {
    juce::ignoreUnused(g);
}

void PluginEditor::resized() {
}

void PluginEditor::visibilityChanged() {
    updateIsShowing();
}

void PluginEditor::parentHierarchyChanged() {
    updateIsShowing();
}

void PluginEditor::minimisationStateChanged(bool) {
    updateIsShowing();
}

void PluginEditor::valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier&) {
}

void PluginEditor::handleAsyncUpdate() {
    sendLookAndFeelChange();
}

void PluginEditor::timerCallback(const int timer_id) {
    if (timer_id == kVisibilityTimer) {
        updateIsShowing();
    } else if (timer_id == kPropertySaveTimer) {
        flushPendingPropertySave();
    }
}

void PluginEditor::schedulePropertySave() {
    startTimer(kPropertySaveTimer, kPropertySaveDelayMS);
}

void PluginEditor::flushPendingPropertySave() {
    if (isTimerRunning(kPropertySaveTimer)) {
        stopTimer(kPropertySaveTimer);
        property_.saveAPVTS(state_);
    }
}

void PluginEditor::updateIsShowing() {
}

int PluginEditor::getControlParameterIndex(Component& c) {
    const auto id = c.getComponentID();
    if (id.isEmpty()) {
        return -1;
    }
    if (const auto para = p_ref_.parameters_.getParameter(id); para == nullptr) {
        return -1;
    } else {
        return para->getParameterIndex();
    }
}

void PluginEditor::mouseDown(const juce::MouseEvent& event) {
    if (event.mods.isRightButtonDown() && event.getNumberOfClicks() == 1) {
        if (event.originalComponent != nullptr) {
            if (const auto id = event.originalComponent->getComponentID(); !id.isEmpty()) {
                if (const auto para = p_ref_.parameters_.getParameter(id); para != nullptr) {
                    if (const auto* context = getHostContext(); context != nullptr) {
                        if (auto menu = context->getContextMenuForParameter(para)) {
                            menu->showNativeMenu(juce::Component::getMouseXYRelative());
                        }
                    }
                }
            }
        }
    }
}
