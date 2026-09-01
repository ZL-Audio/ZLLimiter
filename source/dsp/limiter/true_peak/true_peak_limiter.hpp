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
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <span>
#include <vector>

#include "../../chore/decibels.hpp"
#include "../../container/circular_minmax_buffer.hpp"
#include "../../delay/integer_delay.hpp"
#include "../../vector/vector.hpp"
#include "../envelope/asymmetric_follower.hpp"
#include "bs1770_estimator.hpp"

namespace zldsp::limiter {
    namespace true_peak_detail {
        namespace hn = hwy::HWY_NAMESPACE;

        template <typename FloatType>
        HWY_INLINE void maximumInPlace(FloatType* HWY_RESTRICT output, const FloatType* HWY_RESTRICT input,
                                       const size_t num_samples) {
            static constexpr hn::ScalableTag<FloatType> d;
            static constexpr size_t lanes = hn::MaxLanes(d);

            size_t i = 0;
            for (; i + lanes <= num_samples; i += lanes) {
                hn::StoreU(hn::Max(hn::LoadU(d, output + i), hn::LoadU(d, input + i)), d, output + i);
            }
            for (; i < num_samples; ++i) {
                output[i] = std::max(output[i], input[i]);
            }
        }

        template <typename FloatType>
        HWY_INLINE void peaksToDemand(FloatType* HWY_RESTRICT peaks, const size_t num_samples,
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
                const auto peak = hn::Max(hn::LoadU(d, peaks + i), log_minimum);
                const auto peak_db = hn::Mul(hn::Log(d, peak), log_multiplier);
                hn::StoreU(hn::Max(hn::Sub(peak_db, ceiling), zero), d, peaks + i);
            }
            for (; i < num_samples; ++i) {
                const auto peak_db = kLogMultiplier * std::log(std::max(peaks[i], kLogMinimum));
                peaks[i] = std::max(FloatType(0), peak_db - ceiling_db);
            }
        }

        template <typename FloatType>
        HWY_INLINE void attenuationToGain(FloatType* HWY_RESTRICT attenuation, const size_t num_samples) {
            static constexpr hn::ScalableTag<FloatType> d;
            static constexpr size_t lanes = hn::MaxLanes(d);
            static constexpr auto kDbToLogGain = static_cast<FloatType>(-0.1151292546497022842);
            const auto scale = hn::Set(d, kDbToLogGain);

            size_t i = 0;
            for (; i + lanes <= num_samples; i += lanes) {
                const auto value = hn::LoadU(d, attenuation + i);
                hn::StoreU(hn::Exp(d, hn::Mul(value, scale)), d, attenuation + i);
            }
            for (; i < num_samples; ++i) {
                attenuation[i] = static_cast<FloatType>(
                    std::exp(static_cast<double>(attenuation[i]) * static_cast<double>(kDbToLogGain)));
            }
        }
    }

    /**
     * one causal BS.1770 true-peak correction pass
     * @tparam FloatType the audio sample type
     */
    template <typename FloatType>
    class TruePeakCorrectionStage {
    public:
        static constexpr size_t kLookaheadSamples = BS1770TruePeakEstimator<FloatType>::kTapsPerPhase - 1;
        static constexpr size_t kDetectorWindowSamples = kLookaheadSamples + 1;
        static constexpr size_t kPrimeHistorySamples = kDetectorWindowSamples * 2 - 1;
        static constexpr FloatType kUnityAttenuationDb = FloatType(1e-5);

        void prepare(const double sample_rate, const size_t maximum_block_size, const size_t maximum_channels,
                     const double release_seconds) {
            sample_rate_ = std::max(sample_rate, 1.0);
            maximum_block_size_ = std::max<size_t>(maximum_block_size, 1);
            maximum_channels_ = std::max<size_t>(maximum_channels, 1);
            estimator_.prepare(maximum_channels_, maximum_block_size_);
            maximum_.setCapacity(kDetectorWindowSamples);
            maximum_.setSize(kDetectorWindowSamples);
            release_.prepare(sample_rate_);
            release_.setTimesSeconds(0.0, std::max(release_seconds, 0.0));
            gains_.resize(maximum_block_size_);
            reconstructed_peaks_.resize(maximum_block_size_);
            channel_peaks_.resize(maximum_block_size_);
            input_histories_.resize(maximum_channels_);

            const auto delay_seconds = static_cast<FloatType>(static_cast<double>(kLookaheadSamples) / sample_rate_);
            delay_.prepare(sample_rate_, maximum_block_size_, maximum_channels_, delay_seconds);
            delay_.setDelayInSamples(static_cast<int>(kLookaheadSamples));
            reset();
        }

        void reset() {
            estimator_.reset();
            maximum_.clear();
            release_.reset();
            delay_.reset();
            delay_.setDelayInSamples(static_cast<int>(kLookaheadSamples));
            for (auto& history : input_histories_) {
                history.fill(FloatType(0));
            }
            input_history_position_ = 0;
            input_history_size_ = 0;
            needs_prime_ = false;
            bypassed_ = !enabled_;
        }

        void setEnabled(const bool enabled) {
            if (enabled == enabled_) {
                return;
            }
            enabled_ = enabled;
            if (enabled_) {
                needs_prime_ = true;
                bypassed_ = false;
            } else if (release_.getCurrent() <= kUnityAttenuationDb) {
                release_.reset();
                bypassed_ = true;
            } else {
                bypassed_ = false;
            }
        }

        void setCeilingDecibels(const FloatType ceiling_db) {
            ceiling_db_ = ceiling_db;
        }

        void process(std::span<FloatType*> buffer, const size_t num_samples) {
            assert(buffer.size() <= maximum_channels_);
            assert(num_samples <= maximum_block_size_);
            if (enabled_) {
                if (needs_prime_) {
                    primeDetector(buffer.size());
                }
                processEnabled(buffer, num_samples);
            } else {
                processDisabled(buffer, num_samples);
            }
        }

        [[nodiscard]] size_t getLatencySamples() const {
            return kLookaheadSamples;
        }

    private:
        double sample_rate_{48000.0};
        size_t maximum_block_size_{0};
        size_t maximum_channels_{0};
        FloatType ceiling_db_{FloatType(-1.1)};
        bool enabled_{true};
        bool bypassed_{false};
        bool needs_prime_{false};
        BS1770TruePeakEstimator<FloatType> estimator_{};
        container::CircularMinMaxBuffer<FloatType, container::MinMaxBufferType::kFindMax> maximum_{1};
        AsymmetricFollower<FloatType> release_{};
        delay::IntegerDelay<FloatType> delay_{};
        vector::aligned_vector<FloatType> gains_{};
        vector::aligned_vector<FloatType> reconstructed_peaks_{};
        vector::aligned_vector<FloatType> channel_peaks_{};
        std::vector<std::array<FloatType, kPrimeHistorySamples>> input_histories_{};
        size_t input_history_position_{0};
        size_t input_history_size_{0};

        void processEnabled(std::span<FloatType*> buffer, const size_t num_samples) {
            bypassed_ = false;
            cacheInputBlock(buffer, num_samples);
            estimator_.processBlock(0, buffer[0], reconstructed_peaks_.data(), num_samples);
            for (size_t channel = 1; channel < buffer.size(); ++channel) {
                estimator_.processBlock(channel, buffer[channel], channel_peaks_.data(), num_samples);
                true_peak_detail::maximumInPlace(reconstructed_peaks_.data(), channel_peaks_.data(), num_samples);
            }
            true_peak_detail::peaksToDemand(reconstructed_peaks_.data(), num_samples, ceiling_db_);
            for (size_t i = 0; i < num_samples; ++i) {
                const auto future_maximum_db = maximum_.push(reconstructed_peaks_[i]);
                gains_[i] = release_.processSample(future_maximum_db);
            }
            true_peak_detail::attenuationToGain(gains_.data(), num_samples);

            delay_.process(buffer, num_samples);
            for (auto* channel : buffer) {
                vector::multiply(channel, gains_.data(), num_samples);
            }
        }

        void processDisabled(std::span<FloatType*> buffer, const size_t num_samples) {
            if (bypassed_ || release_.getCurrent() <= kUnityAttenuationDb) {
                release_.reset();
                bypassed_ = true;
                cacheInputBlock(buffer, num_samples);
                delay_.process(buffer, num_samples);
                return;
            }

            cacheInputBlock(buffer, num_samples);
            for (size_t i = 0; i < num_samples; ++i) {
                auto attenuation_db = release_.processSample(FloatType(0));
                if (attenuation_db <= kUnityAttenuationDb) {
                    release_.reset();
                    attenuation_db = FloatType(0);
                    bypassed_ = true;
                }
                gains_[i] = attenuation_db;
            }
            true_peak_detail::attenuationToGain(gains_.data(), num_samples);

            delay_.process(buffer, num_samples);
            for (auto* channel : buffer) {
                vector::multiply(channel, gains_.data(), num_samples);
            }
        }

        void cacheInputBlock(const std::span<FloatType*> buffer, const size_t num_samples) {
            size_t input_position = 0;
            while (input_position < num_samples) {
                const auto copy_size =
                    std::min(num_samples - input_position, kPrimeHistorySamples - input_history_position_);
                for (size_t channel = 0; channel < maximum_channels_; ++channel) {
                    auto* const destination = input_histories_[channel].data() + input_history_position_;
                    if (channel < buffer.size()) {
                        vector::copy(destination, buffer[channel] + input_position, copy_size);
                    } else {
                        std::fill_n(destination, copy_size, FloatType(0));
                    }
                }
                input_position += copy_size;
                input_history_position_ += copy_size;
                if (input_history_position_ == kPrimeHistorySamples) {
                    input_history_position_ = 0;
                }
            }
            input_history_size_ = std::min(input_history_size_ + num_samples, kPrimeHistorySamples);
        }

        void primeDetector(const size_t num_channels) {
            estimator_.reset();
            maximum_.clear();
            const auto first =
                (input_history_position_ + kPrimeHistorySamples - input_history_size_) % kPrimeHistorySamples;
            for (size_t i = 0; i < input_history_size_; ++i) {
                const auto position = (first + i) % kPrimeHistorySamples;
                FloatType reconstructed_peak{0};
                for (size_t channel = 0; channel < num_channels; ++channel) {
                    reconstructed_peak = std::max(
                        reconstructed_peak, estimator_.processSample(channel, input_histories_[channel][position]));
                }
                const auto demand_db = std::max(FloatType(0), chore::gainToDecibels(reconstructed_peak) - ceiling_db_);
                maximum_.push(demand_db);
            }
            needs_prime_ = false;
        }
    };

    /**
     * a fixed-latency two-pass BS.1770 true-peak limiter
     * @tparam FloatType the audio sample type
     */
    template <typename FloatType>
    class TruePeakLimiter {
    public:
        static constexpr double kDefaultReleaseSeconds = 0.01;
        static constexpr FloatType kDefaultSafetyMarginDb = FloatType(0.01);

        void prepare(const double sample_rate, const size_t maximum_block_size, const size_t maximum_channels) {
            maximum_block_size_ = std::max<size_t>(maximum_block_size, 1);
            maximum_channels_ = std::max<size_t>(maximum_channels, 1);
            first_.prepare(sample_rate, maximum_block_size_, maximum_channels_, kDefaultReleaseSeconds);
            second_.prepare(sample_rate, maximum_block_size_, maximum_channels_, kDefaultReleaseSeconds);
            setCeilingDecibels(ceiling_db_, safety_margin_db_);
            setEnabled(enabled_);
            reset();
        }

        void reset() {
            first_.reset();
            second_.reset();
        }

        void setEnabled(const bool enabled) {
            enabled_ = enabled;
            first_.setEnabled(enabled_);
            second_.setEnabled(enabled_);
        }

        void setCeilingDecibels(const FloatType ceiling_db, const FloatType safety_margin_db = kDefaultSafetyMarginDb) {
            ceiling_db_ = ceiling_db;
            safety_margin_db_ = std::max(safety_margin_db, FloatType(0));
            const auto correction_ceiling_db = ceiling_db_ - safety_margin_db_;
            first_.setCeilingDecibels(correction_ceiling_db);
            second_.setCeilingDecibels(correction_ceiling_db);
        }

        void process(std::span<FloatType*> buffer, const size_t num_samples) {
            if (buffer.empty() || num_samples == 0) {
                return;
            }
            assert(buffer.size() <= maximum_channels_);
            assert(num_samples <= maximum_block_size_);
            first_.process(buffer, num_samples);
            second_.process(buffer, num_samples);
        }

        [[nodiscard]] size_t getLatencySamples() const {
            return first_.getLatencySamples() + second_.getLatencySamples();
        }

    private:
        size_t maximum_block_size_{0};
        size_t maximum_channels_{0};
        FloatType ceiling_db_{FloatType(-1)};
        FloatType safety_margin_db_{kDefaultSafetyMarginDb};
        bool enabled_{true};
        TruePeakCorrectionStage<FloatType> first_{};
        TruePeakCorrectionStage<FloatType> second_{};
    };
}
