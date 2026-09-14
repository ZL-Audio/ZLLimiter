// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#include "PluginProcessor.hpp"

#include <array>
#include <limits>

#include "PluginEditor.hpp"


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
    state_(dummy_processor_, nullptr, juce::Identifier(zlstate::schema::kUISettings),
           zlstate::getStateParameterLayout()),
    controller_(*this),
    limiter_attach_(*this, parameters_, controller_) {
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
    value_measurement_active_.store(true, std::memory_order::release);
    value_free_running_ = true;
    expected_playhead_sample_.reset();

    const juce::PluginHostType host_type;
    update_channel_layout_per_call_ = host_type.isMaschine();

    controller_.prepare(sample_rate, static_cast<size_t>(samples_per_block));

    updateChannelLayout();
}

void PluginProcessor::releaseResources() {
    value_measurement_active_.store(false, std::memory_order::release);
    expected_playhead_sample_.reset();
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
    return new PluginEditor(*this);
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
    if (xml_state == nullptr || !xml_state->hasTagName(zlstate::schema::kProcessorState)) {
        return;
    }

    const auto temp_tree = juce::ValueTree::fromXml(*xml_state);
    const auto parameter_state = temp_tree.getChildWithName(
        juce::Identifier(zlstate::schema::kParameterState));
    const auto non_automatable_state = temp_tree.getChildWithName(
        juce::Identifier(zlstate::schema::kNonAutomatableState));

    if (!parameter_state.isValid() || !non_automatable_state.isValid()) {
        return;
    }

    parameters_.replaceState(parameter_state);
    parameters_NA_.replaceState(non_automatable_state);
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

void PluginProcessor::updateValueTransport(const int num_samples) {
    const auto* playhead = getPlayHead();
    const auto position = playhead != nullptr
        ? playhead->getPosition()
        : juce::Optional<juce::AudioPlayHead::PositionInfo>{};
    bool reset_values = !value_measurement_active_.load(std::memory_order::relaxed);
    if (position.hasValue()) {
        if (!position->getIsPlaying()) {
            value_measurement_active_.store(false, std::memory_order::release);
            expected_playhead_sample_.reset();
            return;
        }

        reset_values = reset_values || value_free_running_;
        value_free_running_ = false;
        if (const auto sample = position->getTimeInSamples()) {
            if (expected_playhead_sample_.has_value()) {
                const auto expected = *expected_playhead_sample_;

                const auto later = std::max(*sample, expected);
                const auto earlier = std::min(*sample, expected);
                reset_values = reset_values || (later != earlier && later - 1 != earlier);
            }
            expected_playhead_sample_ = *sample;
        }
    } else if (reset_values) {
        value_free_running_ = true;
    }
    value_measurement_active_.store(true, std::memory_order::release);
    if (reset_values) {
        value_reset_requested_.signal();
    }

    if (expected_playhead_sample_.has_value()) {
        if (*expected_playhead_sample_ <= std::numeric_limits<int64_t>::max() - num_samples) {
            *expected_playhead_sample_ += num_samples;
        } else {
            expected_playhead_sample_.reset();
        }
    }
}

void PluginProcessor::processBlockInternal(juce::AudioBuffer<float>& buffer, const bool bypass) {
    juce::ScopedNoDenormals noDenormals;
    updateValueTransport(buffer.getNumSamples());
    if (buffer.getNumSamples() == 0) {
        return; // ignore empty blocks
    }
    if (update_channel_layout_per_call_) {
        updateChannelLayout();
    }
    const auto num_main_channels = channel_layout_ == kMain1 ? 1 : (channel_layout_ == kMain2 ? 2 : 0);
    if (num_main_channels == 0) {
        buffer.clear();
        return;
    }
    std::array<float*, 2> main_pointers{};
    for (int channel = 0; channel < num_main_channels; ++channel) {
        main_pointers[static_cast<size_t>(channel)] = buffer.getWritePointer(channel);
    }
    controller_.process(std::span<float*>{main_pointers.data(), static_cast<size_t>(num_main_channels)},
                        static_cast<size_t>(buffer.getNumSamples()), bypass);
}

juce::AudioProcessor*JUCE_CALLTYPE

createPluginFilter() {
    return new PluginProcessor();
}
