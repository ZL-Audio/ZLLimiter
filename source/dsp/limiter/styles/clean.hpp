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
#include <span>
#include <vector>

#include "../limiter_definitions.hpp"
#include "../envelope/adaptive_recovery.hpp"
#include "../envelope/asymmetric_follower.hpp"
#include "../envelope/lookahead_envelope.hpp"
#include "../../vector/highway_import.hpp"

namespace zldsp::limiter {
    namespace clean_detail {
        namespace hn = hwy::HWY_NAMESPACE;

        template <typename FloatType>
        HWY_INLINE void calculateRequired(const FloatType* HWY_RESTRICT peaks,
                                          FloatType* HWY_RESTRICT required,
                                          const size_t num_samples,
                                          const FloatType ceiling_db) {
            static constexpr hn::ScalableTag<FloatType> d;
            static constexpr size_t lanes = hn::MaxLanes(d);
            static constexpr auto kLogMinimum = static_cast<FloatType>(1e-12);
            static constexpr auto kLogMultiplier = static_cast<FloatType>(8.6858896380650365530);

            const auto log_minimum = hn::Set(d, kLogMinimum);
            const auto log_multiplier = hn::Set(d, kLogMultiplier);
            const auto ceiling = hn::Set(d, ceiling_db);
            const auto zero = hn::Zero(d);

            size_t i = 0;
            for (; i + lanes <= num_samples; i += lanes) {
                const auto peak = hn::Abs(hn::LoadU(d, peaks + i));
                const auto peak_db = hn::Mul(hn::Log(d, hn::Max(peak, log_minimum)), log_multiplier);
                hn::StoreU(hn::Max(hn::Sub(peak_db, ceiling), zero), d, required + i);
            }
            for (; i < num_samples; ++i) {
                const auto peak = std::max(std::abs(peaks[i]), kLogMinimum);
                const auto peak_db = kLogMultiplier * std::log(peak);
                required[i] = std::max(FloatType(0), peak_db - ceiling_db);
            }
        }

        template <typename FloatType>
        HWY_INLINE FloatType smoothMaximum(const FloatType x, const FloatType y) {
            static constexpr FloatType kCrossingWidthDb{FloatType(0.1)};
            static constexpr FloatType kTwiceCrossingWidthDb{FloatType(2) * kCrossingWidthDb};
            static constexpr FloatType kInverseFourCrossingWidthDb{FloatType(0.25) / kCrossingWidthDb};

            const auto maximum = std::max(x, y);
            const auto limited_sum = std::min(x + y, kTwiceCrossingWidthDb);
            const auto transition_width = limited_sum -
                                          limited_sum * limited_sum * kInverseFourCrossingWidthDb;
            const auto transition = transition_width - std::abs(x - y);
            if (transition <= FloatType(0)) {
                return maximum;
            }
            return maximum + transition * transition / (FloatType(4) * transition_width);
        }
    }

    /**
     * the clean attenuation planner
     * @tparam FloatType
     */
    template <typename FloatType>
    class CleanStyle {
    public:
        void prepare(const double sample_rate,
                     const size_t /*maximum_block_size*/,
                     const size_t maximum_channels,
                     const double maximum_lookahead_seconds = kMaximumLookaheadSeconds) {
            sample_rate_ = sample_rate;
            const auto maximum_lookahead = std::max(maximum_lookahead_seconds, 0.0);
            maximum_lookahead_ms_ = maximum_lookahead * 1000.0;
            lookahead_.resize(maximum_channels);
            fast_state_.resize(maximum_channels);
            fast_.resize(maximum_channels);
            for (size_t channel = 0; channel < maximum_channels; ++channel) {
                lookahead_[channel].prepare(sample_rate_, maximum_lookahead);
            }
            recovery_.prepare(sample_rate_);
            common_support_.prepare(sample_rate_);
            reset();
            setLookaheadMilliseconds(lookahead_ms_);
            setAttackMilliseconds(attack_ms_);
            setReleaseMilliseconds(release_ms_);
            setChannelDeltaDecibels(channel_delta_db_);
        }

        void reset() {
            for (auto& envelope : lookahead_) {
                envelope.reset();
            }
            std::fill(fast_state_.begin(), fast_state_.end(), FloatType(0));
            recovery_.reset();
            common_support_.reset();
            std::fill(fast_.begin(), fast_.end(), FloatType(0));
        }

        void setLookaheadMilliseconds(const FloatType milliseconds) {
            lookahead_ms_ = static_cast<FloatType>(std::clamp(
                static_cast<double>(milliseconds), 0.0, maximum_lookahead_ms_));
            for (auto& envelope : lookahead_) {
                envelope.setLookaheadSeconds(static_cast<double>(lookahead_ms_) * 0.001);
            }
        }

        void setAttackMilliseconds(const FloatType milliseconds) {
            attack_ms_ = std::max(milliseconds, FloatType(0));
            common_support_.setAttackSeconds(static_cast<double>(attack_ms_) * 0.001);
        }

        void setReleaseMilliseconds(const FloatType milliseconds) {
            release_ms_ = std::max(milliseconds, FloatType(0));
            common_support_.setReleaseSeconds(static_cast<double>(release_ms_) * 0.001);
        }

        void setChannelDeltaDecibels(const FloatType decibels) {
            channel_delta_db_ = std::max(decibels, FloatType(0));
        }

        void setRecoverPercent(const FloatType percent) {
            recovery_.setRecoverPercent(percent);
        }

        void setCeilingDecibels(const FloatType ceiling_db) {
            ceiling_db_ = ceiling_db;
        }

        void process(std::span<const FloatType* const> peak_buffers,
                     std::span<FloatType* const> attenuation_buffers,
                     const size_t num_samples) {
            const auto num_channels = peak_buffers.size();
            for (size_t channel = 0; channel < num_channels; ++channel) {
                clean_detail::calculateRequired(
                    peak_buffers[channel], attenuation_buffers[channel], num_samples, ceiling_db_);
            }

            for (size_t i = 0; i < num_samples; ++i) {
                FloatType maximum_planned{0};
                for (size_t channel = 0; channel < num_channels; ++channel) {
                    fast_[channel] = lookahead_[channel].processSample(attenuation_buffers[channel][i]);
                    maximum_planned = std::max(maximum_planned, fast_[channel]);
                }
                const auto release_step = recovery_.processSample(maximum_planned);
                FloatType maximum_fast{0};
                for (size_t channel = 0; channel < num_channels; ++channel) {
                    const auto planned = fast_[channel];
                    auto& state = fast_state_[channel];
                    state = planned >= state ? planned :
                            std::max(planned, state + release_step * (planned - state));
                    fast_[channel] = state;
                    maximum_fast = std::max(maximum_fast, fast_[channel]);
                }

                const auto common = common_support_.processSample(maximum_fast);
                const auto minimum_fast = maximum_fast - channel_delta_db_;
                for (size_t channel = 0; channel < num_channels; ++channel) {
                    const auto bounded_fast = std::max(fast_[channel], minimum_fast);
                    attenuation_buffers[channel][i] = clean_detail::smoothMaximum(bounded_fast, common);
                }
            }
        }

        [[nodiscard]] size_t getMaximumDelaySamples() const {
            return lookahead_.empty() ? 0 : lookahead_.front().getMaximumDelaySamples();
        }

    private:
        double sample_rate_{48000.0};
        double maximum_lookahead_ms_{kMaximumLookaheadSeconds * 1000.0};
        FloatType ceiling_db_{FloatType(-1)};
        FloatType lookahead_ms_{FloatType(2)};
        FloatType attack_ms_{FloatType(100)};
        FloatType release_ms_{FloatType(500)};
        FloatType channel_delta_db_{FloatType(1.5)};
        std::vector<LookaheadEnvelope<FloatType>> lookahead_{};
        std::vector<FloatType> fast_state_{};
        AdaptiveRecovery<FloatType> recovery_{};
        AsymmetricFollower<FloatType> common_support_{};
        std::vector<FloatType> fast_{};
    };
}
