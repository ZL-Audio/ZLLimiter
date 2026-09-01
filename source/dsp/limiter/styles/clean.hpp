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

#include "../definitions.hpp"
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
            maximum_channels_ = maximum_channels;
            const auto maximum_lookahead = std::max(maximum_lookahead_seconds, 0.0);
            maximum_lookahead_ms_ = maximum_lookahead * 1000.0;
            lookahead_.resize(maximum_channels_);
            fast_release_.resize(maximum_channels_);
            fast_.resize(maximum_channels_);
            for (size_t channel = 0; channel < maximum_channels_; ++channel) {
                lookahead_[channel].prepare(sample_rate_, maximum_lookahead);
                fast_release_[channel].prepare(sample_rate_);
            }
            common_support_.prepare(sample_rate_);
            reset();
            setLookaheadMilliseconds(lookahead_ms_);
            setAttackMilliseconds(attack_ms_);
            setReleaseMilliseconds(release_ms_);
            setStereoDeltaDecibels(stereo_delta_db_);
            setMicroReleaseMilliseconds(micro_release_ms_);
        }

        void reset() {
            for (auto& envelope : lookahead_) {
                envelope.reset();
            }
            for (auto& follower : fast_release_) {
                follower.reset();
            }
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

        void setStereoDeltaDecibels(const FloatType decibels) {
            stereo_delta_db_ = std::max(decibels, FloatType(0));
        }

        void setMicroReleaseMilliseconds(const FloatType milliseconds) {
            micro_release_ms_ = std::max(milliseconds, FloatType(0));
            for (auto& follower : fast_release_) {
                follower.setReleaseSeconds(static_cast<double>(micro_release_ms_) * 0.001);
            }
        }

        void setCeilingDecibels(const FloatType ceiling_db) {
            ceiling_db_ = ceiling_db;
        }

        void process(std::span<const FloatType* const> peak_buffers,
                     std::span<FloatType* const> attenuation_buffers,
                     const size_t num_samples) {
            const auto num_channels = std::min({peak_buffers.size(), attenuation_buffers.size(), maximum_channels_});
            for (size_t channel = 0; channel < num_channels; ++channel) {
                clean_detail::calculateRequired(
                    peak_buffers[channel], attenuation_buffers[channel], num_samples, ceiling_db_);
            }

            for (size_t i = 0; i < num_samples; ++i) {
                FloatType maximum_fast{0};
                for (size_t channel = 0; channel < num_channels; ++channel) {
                    const auto planned = lookahead_[channel].processSample(attenuation_buffers[channel][i]);
                    fast_[channel] = fast_release_[channel].processSample(planned);
                    maximum_fast = std::max(maximum_fast, fast_[channel]);
                }

                const auto common = common_support_.processSample(maximum_fast);
                const auto minimum_fast = maximum_fast - stereo_delta_db_;
                for (size_t channel = 0; channel < num_channels; ++channel) {
                    const auto bounded_fast = std::max(fast_[channel], minimum_fast);
                    attenuation_buffers[channel][i] = std::max(bounded_fast, common);
                }
            }
        }

        [[nodiscard]] size_t getMaximumDelaySamples() const {
            return lookahead_.empty() ? 0 : lookahead_.front().getMaximumDelaySamples();
        }

    private:
        double sample_rate_{48000.0};
        double maximum_lookahead_ms_{kMaximumLookaheadSeconds * 1000.0};
        size_t maximum_channels_{0};
        FloatType ceiling_db_{FloatType(-1)};
        FloatType lookahead_ms_{FloatType(2)};
        FloatType attack_ms_{FloatType(100)};
        FloatType release_ms_{FloatType(500)};
        FloatType stereo_delta_db_{FloatType(1.5)};
        FloatType micro_release_ms_{FloatType(50)};
        std::vector<LookaheadEnvelope<FloatType>> lookahead_{};
        std::vector<AsymmetricFollower<FloatType>> fast_release_{};
        AsymmetricFollower<FloatType> common_support_{};
        std::vector<FloatType> fast_{};
    };
}
