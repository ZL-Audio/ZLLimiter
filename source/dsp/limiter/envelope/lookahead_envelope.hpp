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
#include <numbers>

#include "../../vector/vector.hpp"

namespace zldsp::limiter {
    namespace hn = hwy::HWY_NAMESPACE;

    /**
     * a fixed-latency, future-weighted maximum envelope
     * @tparam FloatType the audio sample type
     */
    template <typename FloatType>
    class LookaheadEnvelope {
    public:
        void prepare(const double sample_rate, const double maximum_lookahead_seconds) {
            sample_rate_ = std::max(sample_rate, 1.0);
            maximum_delay_ = static_cast<size_t>(std::ceil(maximum_lookahead_seconds * sample_rate_));
            capacity_ = maximum_delay_ + 1;
            history_.resize(capacity_ * 2);
            weights_.resize(capacity_);
            weights_ready_ = false;
            reset();
            setLookaheadSeconds(std::min(current_lookahead_seconds_, maximum_lookahead_seconds));
        }

        void reset() {
            std::fill(history_.begin(), history_.end(), FloatType(0));
            write_position_ = 0;
        }

        void setLookaheadSeconds(const double seconds) {
            current_lookahead_seconds_ = std::clamp(seconds, 0.0, static_cast<double>(maximum_delay_) / sample_rate_);
            const auto lookahead =
                std::min(maximum_delay_, static_cast<size_t>(std::round(current_lookahead_seconds_ * sample_rate_)));
            if (weights_ready_ && lookahead == lookahead_) {
                return;
            }
            lookahead_ = lookahead;
            if (lookahead_ == 0) {
                weights_[0] = FloatType(1);
                weights_ready_ = true;
                return;
            }
            const auto inverse_length = 1.0 / static_cast<double>(lookahead_);
            for (size_t i = 0; i <= lookahead_; ++i) {
                const auto phase = static_cast<double>(i) * inverse_length;
                weights_[i] = static_cast<FloatType>(0.5 * (1.0 + std::cos(std::numbers::pi * phase)));
            }
            weights_ready_ = true;
        }

        FloatType processSample(const FloatType demand) {
            history_[write_position_] = demand;
            history_[write_position_ + capacity_] = demand;
            write_position_ += 1;
            if (write_position_ == capacity_) {
                write_position_ = 0;
            }
            return weightedMaximum(history_.data() + write_position_, weights_.data(), lookahead_ + 1);
        }

        [[nodiscard]] size_t getMaximumDelaySamples() const {
            return maximum_delay_;
        }

        [[nodiscard]] size_t getLookaheadSamples() const {
            return lookahead_;
        }

    private:
        double sample_rate_{48000.0};
        double current_lookahead_seconds_{0.002};
        size_t maximum_delay_{0};
        size_t lookahead_{0};
        size_t capacity_{1};
        size_t write_position_{0};
        bool weights_ready_{false};
        vector::aligned_vector<FloatType> history_{};
        vector::aligned_vector<FloatType> weights_{};

        static FloatType weightedMaximum(const FloatType* values, const FloatType* weights, const size_t size) {
            static constexpr hn::ScalableTag<FloatType> d;
            static constexpr size_t lanes = hn::MaxLanes(d);
            static constexpr size_t block = lanes << 2;

            auto maximum0 = hn::Zero(d);
            auto maximum1 = hn::Zero(d);
            auto maximum2 = hn::Zero(d);
            auto maximum3 = hn::Zero(d);
            size_t i = 0;
            for (; i + block <= size; i += block) {
                maximum0 = hn::Max(maximum0, hn::Mul(hn::LoadU(d, values + i),
                                                     hn::LoadU(d, weights + i)));
                maximum1 = hn::Max(maximum1, hn::Mul(hn::LoadU(d, values + i + lanes),
                                                     hn::LoadU(d, weights + i + lanes)));
                maximum2 = hn::Max(maximum2, hn::Mul(hn::LoadU(d, values + i + lanes * 2),
                                                     hn::LoadU(d, weights + i + lanes * 2)));
                maximum3 = hn::Max(maximum3, hn::Mul(hn::LoadU(d, values + i + lanes * 3),
                                                     hn::LoadU(d, weights + i + lanes * 3)));
            }
            auto vector_maximum = hn::Max(hn::Max(maximum0, maximum1), hn::Max(maximum2, maximum3));
            for (; i + lanes <= size; i += lanes) {
                const auto value = hn::LoadU(d, values + i);
                const auto weight = hn::LoadU(d, weights + i);
                vector_maximum = hn::Max(vector_maximum, hn::Mul(value, weight));
            }
            auto result = hn::ReduceMax(d, vector_maximum);
            for (; i < size; ++i) {
                result = std::max(result, values[i] * weights[i]);
            }
            return result;
        }
    };
}
