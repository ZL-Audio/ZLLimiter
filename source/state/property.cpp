// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#include "property.hpp"

#include "state_schema.hpp"

namespace zlstate {
    Property::Property() {
    }

    Property::Property(juce::AudioProcessorValueTreeState& apvts) {
        loadAPVTS(apvts);
    }

    void Property::loadAPVTS(juce::AudioProcessorValueTreeState& apvts) {
        std::lock_guard<std::mutex> lock_guard{mutex_};
        if (checkCreateDirectory()) {
            if (const auto xml = juce::XmlDocument::parse(kUIPath); xml) {
                const auto loaded_state = juce::ValueTree::fromXml(*xml);
                if (!loaded_state.hasType(zlstate::schema::kUISettings)) {
                    return;
                }

                juce::ValueTree migrated_state(zlstate::schema::kUISettings);
                migrated_state.copyPropertiesAndChildrenFrom(loaded_state, nullptr);
                apvts.replaceState(migrated_state);

            }
        }
    }

    void Property::saveAPVTS(juce::AudioProcessorValueTreeState& apvts) {
        std::lock_guard<std::mutex> lock{mutex_};
        if (checkCreateDirectory()) {
            if (const auto xml = apvts.copyState().createXml(); xml) {
                if (!xml->writeTo(kUIPath)) {
                    return;
                }
            }
        }
    }

    bool Property::checkCreateDirectory() const {
        // create directory if not exists
        if (!kPath.isDirectory()) {
            if (!kPath.createDirectory()) {
                return false;
            }
        }
        // check if UI preset exists
        if (kUIPath.existsAsFile()) {
            return true;
        }
        // no -> create a blank UI preset
        const auto res = kUIPath.create();
        return res.wasOk();
    }
}
