// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>
#include <vector>

#include "../../chore/rng/xoshiro256_star_star.hpp"
#include "ath_a.hpp"

namespace zldsp::dither {
    enum class Mode { kOff, kTPDF, kATHA };
    enum class BitDepth { k16, k24 };

    /**
     * PCM quantizer with TPDF dither and optional ATH A shaping
     * independent channel states and streams; shared coefficients
     * only prepare allocates; serialize configuration and processing
     */
    template <typename FloatType>
    class Dither {
    public:
        static_assert(std::is_same_v<FloatType, float> || std::is_same_v<FloatType, double>);

        void prepare(const double sample_rate, const size_t maximum_channels) {
            sample_rate_ = sample_rate;
            shaper_.prepare(sample_rate);
            channels_.resize(maximum_channels);
            setSeed(seed_);
        }

        void reset() noexcept {
            for (auto& channel : channels_) {
                channel.reset();
            }
        }

        void setSeed(const std::uint64_t seed) noexcept {
            seed_ = seed;
            zlchore::rng::Xoshiro256StarStar stream(seed_);
            for (auto& channel : channels_) {
                channel.random = stream;
                channel.reset();
                stream.jump();
            }
        }

        void setMode(const Mode mode) noexcept {
            if (mode_ != mode) {
                mode_ = mode;
                reset();
            }
        }

        [[nodiscard]] Mode getMode() const noexcept {
            return mode_;
        }

        [[nodiscard]] Mode getEffectiveMode() const noexcept {
            return mode_ == Mode::kATHA && !shaper_.isAvailable() ? Mode::kTPDF : mode_;
        }

        void setBitDepth(const BitDepth bit_depth) noexcept {
            if (bit_depth_ != bit_depth) {
                bit_depth_ = bit_depth;
                scale_ = bit_depth_ == BitDepth::k16 ? 0x1p15 : 0x1p23;
                inverse_scale_ = 1.0 / scale_;
                reset();
            }
        }

        [[nodiscard]] BitDepth getBitDepth() const noexcept {
            return bit_depth_;
        }

        [[nodiscard]] size_t getWarmUpSamples() const noexcept {
            return getEffectiveMode() == Mode::kATHA ?
                static_cast<size_t>(std::ceil(sample_rate_ * 0.001)) : 0;
        }

        void warmUp(const size_t num_samples) noexcept {
            if (getEffectiveMode() != Mode::kATHA) {
                return;
            }
            for (auto& channel : channels_) {
                for (size_t index = 0; index < num_samples; ++index) {
                    processSample<true>(FloatType(0), channel, 1.0);
                }
            }
        }

        void process(std::span<FloatType*> buffer, const size_t num_samples) noexcept {
            switch (getEffectiveMode()) {
                case Mode::kATHA:
                    processBlock<true>(buffer, num_samples);
                    break;
                case Mode::kTPDF:
                    processBlock<false>(buffer, num_samples);
                    break;
                case Mode::kOff:
                    break;
            }
        }

        void process(std::span<FloatType*> buffer, std::span<const double> envelope) noexcept {
            switch (getEffectiveMode()) {
                case Mode::kATHA:
                    processEnvelope<true>(buffer, envelope);
                    break;
                case Mode::kTPDF:
                    processEnvelope<false>(buffer, envelope);
                    break;
                case Mode::kOff:
                    break;
            }
        }

    private:
        struct Channel {
            zlchore::rng::Xoshiro256StarStar random{};
            ATHAShaper::State history{};
            bool has_history{false};

            void reset() noexcept {
                if (has_history) {
                    history.reset();
                    has_history = false;
                }
            }
        };

        std::vector<Channel> channels_;
        ATHAShaper shaper_;
        Mode mode_{Mode::kOff};
        BitDepth bit_depth_{BitDepth::k16};
        std::uint64_t seed_{0};
        double sample_rate_{48000.0};
        double scale_{0x1p15}, inverse_scale_{0x1p-15};

        template <bool shaped>
        void processBlock(std::span<FloatType*> buffer, const size_t num_samples) noexcept {
            for (size_t channel = 0; channel < buffer.size(); ++channel) {
                auto& state = channels_[channel];
                auto* samples = buffer[channel];
                for (size_t index = 0; index < num_samples; ++index) {
                    samples[index] = processSample<shaped>(samples[index], state, 1.0);
                }
            }
        }

        template <bool shaped>
        void processEnvelope(std::span<FloatType*> buffer, std::span<const double> envelope) noexcept {
            for (size_t index = 0; index < envelope.size(); ++index) {
                auto amount = std::isfinite(envelope[index]) ? std::clamp(envelope[index], 0.0, 1.0) : 1.0;
                if (amount < 1.0) {
                    for (const auto* samples : buffer) {
                        if (std::fpclassify(samples[index]) != FP_ZERO) {
                            // any audio forces full dither on all channels
                            amount = 1.0;
                            break;
                        }
                    }
                }
                for (size_t channel = 0; channel < buffer.size(); ++channel) {
                    buffer[channel][index] = processSample<shaped>(buffer[channel][index], channels_[channel], amount);
                }
            }
        }

        template <bool shaped>
        FloatType processSample(const FloatType sample, Channel& channel, const double amount) noexcept {
            if (amount == 0.0) {
                channel.reset();
                return FloatType(0);
            }

            const auto input = static_cast<double>(sample);
            if (!std::isfinite(input)) {
                channel.reset();
                if (std::isnan(input)) {
                    return FloatType(0);
                }
                return input < 0.0 ? FloatType(-1) : static_cast<FloatType>(1.0 - inverse_scale_);
            }
            // clamp before scaling; the positive PCM limit is one LSB below one
            if (input >= 1.0 || input < -1.0) {
                channel.reset();
                return input < 0.0 ? FloatType(-1) : static_cast<FloatType>(1.0 - inverse_scale_);
            }

            auto value = input * scale_;
            if constexpr (shaped) {
                value += amount * channel.history.correction;
            }
            // round to nearest, independent of the host rounding mode
            const auto quantized = std::round(value + amount * channel.random.nextTriangular());
            if (quantized < -scale_ || quantized > scale_ - 1.0) {
                // discard feedback after clipping to prevent windup
                channel.reset();
                return quantized < -scale_ ? FloatType(-1) : static_cast<FloatType>(1.0 - inverse_scale_);
            }
            if constexpr (shaped) {
                shaper_.processError(quantized - value, channel.history);
                channel.has_history = true;
            }
            return static_cast<FloatType>(quantized * inverse_scale_);
        }
    };
}
