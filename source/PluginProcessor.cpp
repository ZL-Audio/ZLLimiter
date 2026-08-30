// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#include "PluginProcessor.hpp"

#include <numbers>

#include "PluginEditor.hpp"

namespace {
    juce::ValueTree copyWithType(const juce::ValueTree& source, const juce::Identifier& type) {
        if (!source.isValid()) {
            return {};
        }

        juce::ValueTree result(type);
        result.copyPropertiesAndChildrenFrom(source, nullptr);
        return result;
    }

    juce::ValueTree getChildWithLegacyFallback(const juce::ValueTree& parent,
                                               const juce::Identifier& type,
                                               const juce::Identifier& legacy_type) {
        const auto child = parent.getChildWithName(type);
        return child.isValid() ? child : parent.getChildWithName(legacy_type);
    }
}

//==============================================================================
PluginProcessor::PluginProcessor() :
    AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withInput("Aux", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
        ),
    dummy_processor_(),
    parameters_(*this, nullptr,
                juce::Identifier(zlstate::schema::kParameterState),
                zlp::getParameterLayout()),
    parameters_NA_(dummy_processor_, nullptr,
                   juce::Identifier(zlstate::schema::kNonAutomatableState),
                   zlstate::getNAParameterLayout()),
    controller_(*this) {
}

PluginProcessor::~PluginProcessor() = default;

const juce::String PluginProcessor::getName() const {
    return JucePlugin_Name;
}

bool PluginProcessor::acceptsMidi() const {
    return false;
}

bool PluginProcessor::producesMidi() const {
    return false;
}

bool PluginProcessor::isMidiEffect() const {
    return false;
}

double PluginProcessor::getTailLengthSeconds() const {
    return 0.0;
}

int PluginProcessor::getNumPrograms() {
    return 1;
}

int PluginProcessor::getCurrentProgram() {
    return 0;
}

void PluginProcessor::setCurrentProgram(int) {
}

const juce::String PluginProcessor::getProgramName(int) {
    return "Default";
}

void PluginProcessor::changeProgramName(int, const juce::String&) {
}

void PluginProcessor::prepareToPlay(const double sample_rate, const int samples_per_block) {
    sample_rate_.store(sample_rate, std::memory_order::relaxed);

    const juce::PluginHostType host_type;
    update_channel_layout_per_call_ = host_type.isMaschine();

    controller_.prepare(sample_rate, static_cast<size_t>(samples_per_block));

    updateChannelLayout();
}

void PluginProcessor::releaseResources() {
}

bool PluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    if (layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo() &&
        layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo() &&
        (layouts.getChannelSet(true, 1).isDisabled() ||
            layouts.getChannelSet(true, 1) == juce::AudioChannelSet::mono() ||
            layouts.getChannelSet(true, 1) == juce::AudioChannelSet::stereo())) {
        return true;
    }
    if (layouts.getMainInputChannelSet() == juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono() &&
        (layouts.getChannelSet(true, 1).isDisabled() ||
            layouts.getChannelSet(true, 1) == juce::AudioChannelSet::mono() ||
            layouts.getChannelSet(true, 1) == juce::AudioChannelSet::stereo())) {
        return true;
    }
    return false;
}

void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    processBlockInternal(buffer, false);
}

void PluginProcessor::processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    processBlockInternal(buffer, true);
}

bool PluginProcessor::hasEditor() const {
    return true;
}

juce::AudioProcessorEditor* PluginProcessor::createEditor() {
    return new juce::GenericAudioProcessorEditor (*this);
    // return new PluginEditor(*this);
}

void PluginProcessor::getStateInformation(juce::MemoryBlock& dest_data) {
    auto temp_tree = juce::ValueTree(zlstate::schema::kProcessorState);
    temp_tree.appendChild(parameters_.copyState(), nullptr);
    temp_tree.appendChild(parameters_NA_.copyState(), nullptr);
    const std::unique_ptr<juce::XmlElement> xml(temp_tree.createXml());
    copyXmlToBinary(*xml, dest_data);
}

void PluginProcessor::setStateInformation(const void* data, const int size_in_bytes) {
    std::unique_ptr<juce::XmlElement> xml_state(getXmlFromBinary(data, size_in_bytes));
    if (xml_state == nullptr ||
        (!xml_state->hasTagName(zlstate::schema::kProcessorState) &&
            !xml_state->hasTagName(zlstate::schema::legacy::kProcessorState))) {
        return;
    }

    const auto temp_tree = juce::ValueTree::fromXml(*xml_state);
    const auto parameter_state = getChildWithLegacyFallback(
        temp_tree,
        juce::Identifier(zlstate::schema::kParameterState),
        juce::Identifier(zlstate::schema::legacy::kParameterState));
    const auto non_automatable_state = getChildWithLegacyFallback(
        temp_tree,
        juce::Identifier(zlstate::schema::kNonAutomatableState),
        juce::Identifier(zlstate::schema::legacy::kNonAutomatableState));
    if (!parameter_state.isValid() || !non_automatable_state.isValid()) {
        return;
    }

    parameters_.replaceState(copyWithType(parameter_state,
                                          juce::Identifier(zlstate::schema::kParameterState)));
    parameters_NA_.replaceState(copyWithType(non_automatable_state,
                                             juce::Identifier(zlstate::schema::kNonAutomatableState)));
}

void PluginProcessor::updateChannelLayout() {
    const auto* main_bus = getBus(true, 0);
    channel_layout_ = kInvalid;
    if (main_bus == nullptr) {
        return;
    }
    if (main_bus->getCurrentLayout() == juce::AudioChannelSet::mono()) {
        channel_layout_ = kMain1;
    } else if (main_bus->getCurrentLayout() == juce::AudioChannelSet::stereo()) {
        channel_layout_ = kMain2;
    }
}

void PluginProcessor::processBlockInternal(juce::AudioBuffer<float>& buffer, const bool bypass) {
    juce::ScopedNoDenormals noDenormals;
    if (buffer.getNumSamples() == 0) {
        return; // ignore empty blocks
    }
    if (update_channel_layout_per_call_) {
        updateChannelLayout();
    }
    controller_.prepareBuffer();

}

juce::AudioProcessor*JUCE_CALLTYPE

createPluginFilter() {
    return new PluginProcessor();
}
