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
#include <cmath>
#include <limits>
#include <span>
#include <vector>

#include "../../container/circular_minmax_buffer.hpp"
#include "../../delay/integer_delay.hpp"
#include "../../vector/vector.hpp"

namespace zldsp::limiter {
    /**
     * a common-channel residual clipping-control limiter
     * @tparam FloatType the audio sample type
     */
    template <typename FloatType>
    class SafetyGuardian {
    public:
        void prepare(const double sample_rate,
                     const size_t maximum_block_size,
                     const size_t maximum_channels,
                     const size_t lookahead_samples,
                     const FloatType aim_factor = FloatType(1.01)) {
            sample_rate_ = std::max(sample_rate, 1.0);
            lookahead_samples_ = lookahead_samples;
            const auto delay_seconds = static_cast<FloatType>(
                static_cast<double>(lookahead_samples_) / sample_rate_);
            delay_.prepare(sample_rate_, maximum_block_size, maximum_channels, delay_seconds);
            delay_.setDelayInSamples(static_cast<int>(lookahead_samples_));
            gains_.resize(maximum_block_size);
            maximum_.setCapacity(lookahead_samples_ + 1);
            maximum_.setSize(lookahead_samples_ + 1);

            const auto eta = std::max(static_cast<double>(aim_factor), 1.000001);
            beta_ = static_cast<FloatType>(1.0 - 1.0 / eta);
            inverse_one_minus_beta_ = std::nextafter(
                FloatType(1) / (FloatType(1) - beta_), std::numeric_limits<FloatType>::infinity());
            attack_step_ = static_cast<FloatType>(
                1.0 - std::pow(static_cast<double>(beta_), 1.0 / static_cast<double>(lookahead_samples_ + 1)));
            reset();
        }

        void reset() {
            envelope_ = FloatType(0);
            maximum_.clear();
            delay_.reset();
            delay_.setDelayInSamples(static_cast<int>(lookahead_samples_));
        }

        void setCeilingLinear(const FloatType ceiling) {
            ceiling_ = std::max(ceiling, FloatType(1e-12));
        }

        void process(std::span<FloatType*> buffer, const size_t num_samples) {
            for (size_t i = 0; i < num_samples; ++i) {
                FloatType magnitude{0};
                for (const auto* channel : buffer) {
                    magnitude = std::max(magnitude, std::abs(channel[i]));
                }
                const auto aimed = std::max(
                    magnitude,
                    (magnitude - beta_ * envelope_) * inverse_one_minus_beta_);
                const auto running_maximum = maximum_.push(aimed);
                envelope_ += attack_step_ * (running_maximum - envelope_);
                gains_[i] = envelope_ > ceiling_ ? ceiling_ / envelope_ : FloatType(1);
            }

            delay_.process(buffer, num_samples);
            for (auto* channel : buffer) {
                vector::multiply(channel, gains_.data(), num_samples);
            }
        }

        [[nodiscard]] size_t getLatencySamples() const { return lookahead_samples_; }

    private:
        double sample_rate_{48000.0};
        size_t lookahead_samples_{0};
        FloatType ceiling_{FloatType(1)};
        FloatType beta_{FloatType(0)};
        FloatType inverse_one_minus_beta_{FloatType(1)};
        FloatType attack_step_{FloatType(1)};
        FloatType envelope_{FloatType(0)};
        container::CircularMinMaxBuffer<FloatType, container::MinMaxBufferType::kFindMax> maximum_{1};
        delay::IntegerDelay<FloatType> delay_{};
        vector::aligned_vector<FloatType> gains_{};
    };
}
