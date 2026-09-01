// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#include "controller.hpp"

#include <algorithm>
#include <array>

namespace zlp {
    Controller::Controller(juce::AudioProcessor& processor) :
        p_ref_(processor) {
        jassert(POversampling::kChoices.size() == static_cast<int>(kOversamplingModeCount));
    }

    Controller::~Controller() {
        cancelPendingUpdate();
    }

    void Controller::prepare(const double sample_rate, const size_t maximum_block_size) {
        cancelPendingUpdate();
        is_prepared_ = false;
        maximum_block_size_ = std::max<size_t>(maximum_block_size, 1);
        maximum_channels_ = std::clamp<size_t>(
            static_cast<size_t>(std::max(p_ref_.getMainBusNumInputChannels(), 1)), 1, 2);
        prepareLimiters(sample_rate, maximum_block_size_, maximum_channels_);

        bypass_buffers_.resize(maximum_channels_);
        bypass_pointers_.resize(maximum_channels_);
        for (size_t channel = 0; channel < maximum_channels_; ++channel) {
            bypass_buffers_[channel].resize(maximum_block_size_);
            bypass_pointers_[channel] = bypass_buffers_[channel].data();
        }
        const auto maximum_latency = getMaximumLatencySamples();
        const auto maximum_delay_seconds = static_cast<float>(
            static_cast<double>(maximum_latency) / std::max(sample_rate, 1.0));
        bypass_delay_.prepare(sample_rate, maximum_block_size_, maximum_channels_, maximum_delay_seconds);

        oversampling_index_ = kOversamplingModeCount;
        is_prepared_ = true;
        signalParameterUpdates();
        prepareBuffer();
        cancelPendingUpdate();
        const auto latency = getActiveLatencySamples();
        pending_latency_samples_.store(static_cast<int>(latency), std::memory_order_release);
        p_ref_.setLatencySamples(static_cast<int>(latency));
    }

    void Controller::prepareBuffer() {
        if (!is_prepared_ || !to_update_.check()) {
            return;
        }

        if (to_update_input_gain_.check()) {
            const auto db = input_gain_db_.load(std::memory_order_relaxed);
            std::apply([&](auto&... limiter) {
                (limiter.setInputGainDecibels(db), ...);
            }, limiters_);
        }

        if (to_update_output_ceiling_.check()) {
            const auto db = output_ceiling_db_.load(std::memory_order_relaxed);
            std::apply([&](auto&... limiter) {
                (limiter.setOutputCeilingDecibels(db), ...);
            }, limiters_);
        }

        if (to_update_true_peak_.check()) {
            const auto enabled = true_peak_enabled_.load(std::memory_order_relaxed);
            std::apply([&](auto&... limiter) {
                (limiter.setTruePeakEnabled(enabled), ...);
            }, limiters_);
        }

        if (to_update_lookahead_.check()) {
            const auto milliseconds = lookahead_ms_.load(std::memory_order_relaxed);
            std::apply([&](auto&... limiter) {
                (limiter.setLookaheadMilliseconds(milliseconds), ...);
            }, limiters_);
        }
        if (to_update_attack_.check()) {
            const auto milliseconds = attack_ms_.load(std::memory_order_relaxed);
            std::apply([&](auto&... limiter) {
                (limiter.setAttackMilliseconds(milliseconds), ...);
            }, limiters_);
        }
        if (to_update_release_.check()) {
            const auto milliseconds = release_ms_.load(std::memory_order_relaxed);
            std::apply([&](auto&... limiter) {
                (limiter.setReleaseMilliseconds(milliseconds), ...);
            }, limiters_);
        }
        if (to_update_stereo_delta_.check()) {
            const auto decibels = stereo_delta_db_.load(std::memory_order_relaxed);
            std::apply([&](auto&... limiter) {
                (limiter.setStereoDeltaDecibels(decibels), ...);
            }, limiters_);
        }

        if (to_update_oversampling_.check()) {
            const auto index = std::clamp(
                oversampling_index_parameter_.load(std::memory_order_relaxed),
                0, static_cast<int>(kOversamplingModeCount - 1));
            selectOversampling(static_cast<size_t>(index));
        }
    }

    void Controller::process(std::span<float*> buffer, const size_t num_samples, const bool is_bypass) {
        if (!is_prepared_ || buffer.empty() || num_samples == 0) {
            return;
        }
        prepareBuffer();
        const auto num_channels = std::min(buffer.size(), maximum_channels_);
        auto dry_buffer = std::span<float*>{bypass_pointers_.data(), num_channels};
        std::array<float*, 2> wet_pointers{};
        size_t position = 0;
        while (position < num_samples) {
            const auto block_size = std::min(maximum_block_size_, num_samples - position);
            for (size_t channel = 0; channel < num_channels; ++channel) {
                wet_pointers[channel] = buffer[channel] + position;
                zldsp::vector::copy(dry_buffer[channel], wet_pointers[channel], block_size);
            }
            auto wet_buffer = std::span<float*>{wet_pointers.data(), num_channels};
            bypass_delay_.process(dry_buffer, block_size);
            processActiveLimiter(wet_buffer, block_size);
            if (is_bypass) {
                for (size_t channel = 0; channel < num_channels; ++channel) {
                    zldsp::vector::copy(wet_buffer[channel], dry_buffer[channel], block_size);
                }
            }
            position += block_size;
        }
    }

    void Controller::signalParameterUpdates() {
        to_update_input_gain_.signal();
        to_update_output_ceiling_.signal();
        to_update_true_peak_.signal();
        to_update_oversampling_.signal();
        to_update_lookahead_.signal();
        to_update_attack_.signal();
        to_update_release_.signal();
        to_update_stereo_delta_.signal();
        to_update_.signal();
    }

    void Controller::prepareLimiters(const double sample_rate,
                                     const size_t maximum_block_size,
                                     const size_t maximum_channels) {
        std::apply([&](auto&... limiter) {
            (limiter.prepare(sample_rate, maximum_block_size, maximum_channels), ...);
        }, limiters_);
    }

    void Controller::selectOversampling(const size_t oversampling_index) {
        const auto clamped_index = std::min(oversampling_index, kOversamplingModeCount - 1);
        if (clamped_index == oversampling_index_) {
            return;
        }
        oversampling_index_ = clamped_index;
        resetActiveLimiter();

        const auto latency = getActiveLatencySamples();
        bypass_delay_.setDelayInSamples(static_cast<int>(latency));
        bypass_delay_.reset();
        pending_latency_samples_.store(static_cast<int>(latency), std::memory_order_release);
        triggerAsyncUpdate();
    }

    void Controller::resetActiveLimiter() {
        switch (oversampling_index_) {
        case 0:
            std::get<0>(limiters_).reset();
            break;
        case 1:
            std::get<1>(limiters_).reset();
            break;
        case 2:
            std::get<2>(limiters_).reset();
            break;
        case 3:
            std::get<3>(limiters_).reset();
            break;
        case 4:
            std::get<4>(limiters_).reset();
            break;
        case 5:
            std::get<5>(limiters_).reset();
            break;
        default:
            jassertfalse;
            break;
        }
    }

    void Controller::processActiveLimiter(std::span<float*> buffer, const size_t num_samples) {
        switch (oversampling_index_) {
        case 0:
            std::get<0>(limiters_).process(buffer, num_samples);
            break;
        case 1:
            std::get<1>(limiters_).process(buffer, num_samples);
            break;
        case 2:
            std::get<2>(limiters_).process(buffer, num_samples);
            break;
        case 3:
            std::get<3>(limiters_).process(buffer, num_samples);
            break;
        case 4:
            std::get<4>(limiters_).process(buffer, num_samples);
            break;
        case 5:
            std::get<5>(limiters_).process(buffer, num_samples);
            break;
        default:
            jassertfalse;
            break;
        }
    }

    size_t Controller::getActiveLatencySamples() const {
        switch (oversampling_index_) {
        case 0:
            return std::get<0>(limiters_).getLatencySamples();
        case 1:
            return std::get<1>(limiters_).getLatencySamples();
        case 2:
            return std::get<2>(limiters_).getLatencySamples();
        case 3:
            return std::get<3>(limiters_).getLatencySamples();
        case 4:
            return std::get<4>(limiters_).getLatencySamples();
        case 5:
            return std::get<5>(limiters_).getLatencySamples();
        default:
            jassertfalse;
            return std::get<2>(limiters_).getLatencySamples();
        }
    }

    size_t Controller::getMaximumLatencySamples() const {
        size_t maximum_latency = 0;
        const auto update = [&](const auto& limiter) {
            maximum_latency = std::max(maximum_latency, limiter.getLatencySamples());
        };
        std::apply([&](const auto&... limiter) {
            (update(limiter), ...);
        }, limiters_);
        return maximum_latency;
    }

    void Controller::handleAsyncUpdate() {
        p_ref_.setLatencySamples(pending_latency_samples_.load(std::memory_order_acquire));
    }
}
