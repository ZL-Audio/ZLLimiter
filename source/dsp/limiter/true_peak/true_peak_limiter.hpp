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
#include <cmath>
#include <cstddef>
#include <span>
#include <vector>

#include "../../chore/decibels.hpp"
#include "../../delay/integer_delay.hpp"
#include "../../vector/vector.hpp"
#include "../envelope/asymmetric_follower.hpp"
#include "../envelope/lookahead_envelope.hpp"
#include "true_peak_estimator.hpp"

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
        HWY_INLINE void absoluteCopy(FloatType* HWY_RESTRICT output, const FloatType* HWY_RESTRICT input,
                                     const size_t num_samples) {
            static constexpr hn::ScalableTag<FloatType> d;
            static constexpr size_t lanes = hn::MaxLanes(d);

            size_t i = 0;
            for (; i + lanes <= num_samples; i += lanes) {
                hn::StoreU(hn::Abs(hn::LoadU(d, input + i)), d, output + i);
            }
            for (; i < num_samples; ++i) {
                output[i] = std::abs(input[i]);
            }
        }

        template <typename FloatType>
        HWY_INLINE void absoluteMaximumInPlace(FloatType* HWY_RESTRICT output, const FloatType* HWY_RESTRICT input,
                                               const size_t num_samples) {
            static constexpr hn::ScalableTag<FloatType> d;
            static constexpr size_t lanes = hn::MaxLanes(d);

            size_t i = 0;
            for (; i + lanes <= num_samples; i += lanes) {
                hn::StoreU(hn::Max(hn::LoadU(d, output + i), hn::Abs(hn::LoadU(d, input + i))), d, output + i);
            }
            for (; i < num_samples; ++i) {
                output[i] = std::max(output[i], std::abs(input[i]));
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
     * one causal 16x true-peak correction pass
     * @tparam FloatType the audio sample type
     */
    template <typename FloatType>
    class TruePeakCorrectionStage {
    public:
        enum class StageMode {
            kTruePeak,
            kSamplePeak,
            kBypassed
        };

        static constexpr size_t kLookaheadSamples = TruePeakEstimator<FloatType>::kTapsPerPhase - 1;
        static constexpr size_t kDetectorWindowSamples = kLookaheadSamples + 1;
        static constexpr size_t kPrimeHistorySamples = kDetectorWindowSamples * 2 - 1;
        static constexpr FloatType kUnityAttenuationDb = FloatType(1e-5);
        static constexpr size_t kTruePeakHoldSamples = 32;
        static constexpr size_t kTruePeakAttackSamples = kLookaheadSamples - kTruePeakHoldSamples;

        void prepare(const double sample_rate, const size_t maximum_block_size, const size_t maximum_channels,
                     const double release_seconds) {
            sample_rate_ = std::max(sample_rate, 1.0);
            estimator_.prepare(maximum_channels, maximum_block_size);
            lookahead_envelope_.prepare(sample_rate_, static_cast<double>(kLookaheadSamples) / sample_rate_);
            release_.prepare(sample_rate_);
            release_.setAttackSeconds(0.0);
            release_.setReleaseSeconds(std::max(release_seconds, 0.0));
            gains_.resize(maximum_block_size);
            reconstructed_peaks_.resize(maximum_block_size);
            channel_peaks_.resize(maximum_block_size);
            input_histories_.resize(maximum_channels);

            const auto delay_seconds = static_cast<FloatType>(static_cast<double>(kLookaheadSamples) / sample_rate_);
            delay_.prepare(sample_rate_, maximum_block_size, maximum_channels, delay_seconds);
            delay_.setDelayInSamples(static_cast<int>(kLookaheadSamples));
            updateEnvelopeShape();
            reset();
        }

        void reset() {
            estimator_.reset();
            lookahead_envelope_.reset();
            release_.reset();
            delay_.reset();
            delay_.setDelayInSamples(static_cast<int>(kLookaheadSamples));
            for (auto& history : input_histories_) {
                history.fill(FloatType(0));
            }
            input_history_position_ = 0;
            input_history_size_ = 0;
            needs_prime_ = false;
            bypassed_ = (mode_ == StageMode::kBypassed);
        }

        void setMode(const StageMode mode) {
            if (mode == mode_) {
                return;
            }
            mode_ = mode;
            updateEnvelopeShape();
            if (mode_ == StageMode::kTruePeak) {
                needs_prime_ = true;
                bypassed_ = false;
            } else if (mode_ == StageMode::kSamplePeak) {
                bypassed_ = false;
                needs_prime_ = false;
            } else {
                if (release_.getCurrent() <= kUnityAttenuationDb) {
                    release_.reset();
                    bypassed_ = true;
                } else {
                    bypassed_ = false;
                }
                needs_prime_ = false;
            }
        }

        void setCeilingDecibels(const FloatType true_peak_ceiling_db, const FloatType sample_peak_ceiling_db) {
            ceiling_db_ = true_peak_ceiling_db;
            sample_ceiling_db_ = sample_peak_ceiling_db;
        }

        void process(std::span<FloatType*> buffer, const size_t num_samples) {
            if (buffer.empty() || num_samples == 0) {
                return;
            }
            switch (mode_) {
                case StageMode::kTruePeak:
                    if (needs_prime_) {
                        primeDetector();
                    }
                    processTruePeak(buffer, num_samples);
                    break;
                case StageMode::kSamplePeak:
                    processSamplePeak(buffer, num_samples);
                    break;
                case StageMode::kBypassed:
                    processBypassed(buffer, num_samples);
                    break;
            }
        }

        [[nodiscard]] size_t getLatencySamples() const {
            return kLookaheadSamples;
        }

    private:
        double sample_rate_{48000.0};
        FloatType ceiling_db_{FloatType(-1.1)};
        FloatType sample_ceiling_db_{FloatType(-1.01)};
        StageMode mode_{StageMode::kTruePeak};
        bool bypassed_{false};
        bool needs_prime_{false};
        TruePeakEstimator<FloatType> estimator_{};
        LookaheadEnvelope<FloatType> lookahead_envelope_{};
        AsymmetricFollower<FloatType> release_{};
        delay::IntegerDelay<FloatType> delay_{};
        vector::aligned_vector<FloatType> gains_{};
        vector::aligned_vector<FloatType> reconstructed_peaks_{};
        vector::aligned_vector<FloatType> channel_peaks_{};
        std::vector<std::array<FloatType, kPrimeHistorySamples>> input_histories_{};
        size_t input_history_position_{0};
        size_t input_history_size_{0};

        void updateEnvelopeShape() {
            if (mode_ == StageMode::kTruePeak) {
                lookahead_envelope_.setShape(kTruePeakAttackSamples, kTruePeakHoldSamples);
            } else {
                lookahead_envelope_.setShape(kLookaheadSamples, 0);
            }
        }

        void processTruePeak(std::span<FloatType*> buffer, const size_t num_samples) {
            bypassed_ = false;
            cacheInputBlock(buffer, num_samples);
            estimator_.processBlock(0, buffer[0], reconstructed_peaks_.data(), num_samples);
            for (size_t channel = 1; channel < buffer.size(); ++channel) {
                estimator_.processBlock(channel, buffer[channel], channel_peaks_.data(), num_samples);
                true_peak_detail::maximumInPlace(reconstructed_peaks_.data(), channel_peaks_.data(), num_samples);
            }
            true_peak_detail::peaksToDemand(reconstructed_peaks_.data(), num_samples, ceiling_db_);
            for (size_t i = 0; i < num_samples; ++i) {
                const auto future_maximum_db = lookahead_envelope_.processSample(reconstructed_peaks_[i]);
                gains_[i] = release_.processSample(future_maximum_db);
            }
            true_peak_detail::attenuationToGain(gains_.data(), num_samples);

            delay_.process(buffer, num_samples);
            for (auto* channel : buffer) {
                vector::multiply(channel, gains_.data(), num_samples);
            }
        }

        void processSamplePeak(std::span<FloatType*> buffer, const size_t num_samples) {
            bypassed_ = false;
            cacheInputBlock(buffer, num_samples);

            true_peak_detail::absoluteCopy(reconstructed_peaks_.data(), buffer[0], num_samples);
            for (size_t channel = 1; channel < buffer.size(); ++channel) {
                true_peak_detail::absoluteMaximumInPlace(reconstructed_peaks_.data(), buffer[channel], num_samples);
            }
            true_peak_detail::peaksToDemand(reconstructed_peaks_.data(), num_samples, sample_ceiling_db_);

            for (size_t i = 0; i < num_samples; ++i) {
                const auto future_maximum_db = lookahead_envelope_.processSample(reconstructed_peaks_[i]);
                gains_[i] = release_.processSample(future_maximum_db);
            }
            true_peak_detail::attenuationToGain(gains_.data(), num_samples);

            delay_.process(buffer, num_samples);
            for (auto* channel : buffer) {
                vector::multiply(channel, gains_.data(), num_samples);
            }
        }

        void processBypassed(std::span<FloatType*> buffer, const size_t num_samples) {
            cacheInputBlock(buffer, num_samples);
            if (bypassed_ || release_.getCurrent() <= kUnityAttenuationDb) {
                release_.reset();
                bypassed_ = true;
                delay_.process(buffer, num_samples);
                return;
            }

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
            if (num_samples >= kPrimeHistorySamples) {
                const auto next_position =
                    (input_history_position_ + num_samples % kPrimeHistorySamples) % kPrimeHistorySamples;
                const auto first_size = kPrimeHistorySamples - next_position;
                for (size_t channel = 0; channel < buffer.size(); ++channel) {
                    const auto* const suffix = buffer[channel] + num_samples - kPrimeHistorySamples;
                    auto* const history = input_histories_[channel].data();
                    vector::copy(history + next_position, suffix, first_size);
                    if (next_position != 0) {
                        vector::copy(history, suffix + first_size, next_position);
                    }
                }
                input_history_position_ = next_position;
                input_history_size_ = kPrimeHistorySamples;
                return;
            }

            size_t input_position = 0;
            while (input_position < num_samples) {
                const auto copy_size =
                    std::min(num_samples - input_position, kPrimeHistorySamples - input_history_position_);
                for (size_t channel = 0; channel < buffer.size(); ++channel) {
                    auto* const destination = input_histories_[channel].data() + input_history_position_;
                    vector::copy(destination, buffer[channel] + input_position, copy_size);
                }
                input_position += copy_size;
                input_history_position_ += copy_size;
                if (input_history_position_ == kPrimeHistorySamples) {
                    input_history_position_ = 0;
                }
            }
            input_history_size_ = std::min(input_history_size_ + num_samples, kPrimeHistorySamples);
        }

        void primeDetector() {
            estimator_.reset();
            lookahead_envelope_.reset();
            const auto first =
                (input_history_position_ + kPrimeHistorySamples - input_history_size_) % kPrimeHistorySamples;
            for (size_t i = 0; i < input_history_size_; ++i) {
                const auto position = (first + i) % kPrimeHistorySamples;
                FloatType reconstructed_peak{0};
                for (size_t channel = 0; channel < input_histories_.size(); ++channel) {
                    reconstructed_peak = std::max(
                        reconstructed_peak, estimator_.processSample(channel, input_histories_[channel][position]));
                }
                const auto demand_db = std::max(FloatType(0), chore::gainToDecibels(reconstructed_peak) - ceiling_db_);
                lookahead_envelope_.processSample(demand_db);
            }
            needs_prime_ = false;
        }
    };

    /**
     * a fixed-latency three-pass 16x true-peak limiter
     * @tparam FloatType the audio sample type
     */
    template <typename FloatType>
    class TruePeakLimiter {
    public:
        using StageMode = typename TruePeakCorrectionStage<FloatType>::StageMode;
        static constexpr double kDefaultReleaseSeconds = 0.01;
        static constexpr FloatType kDefaultSafetyMarginDb = FloatType(0.1);
        static constexpr FloatType kDefaultSampleMarginDb = FloatType(0.01);

        void prepare(const double sample_rate, const size_t maximum_block_size, const size_t maximum_channels,
                     const bool oversampling_enabled = true) {
            oversampling_enabled_ = oversampling_enabled;
            first_.prepare(sample_rate, maximum_block_size, maximum_channels, kDefaultReleaseSeconds);
            second_.prepare(sample_rate, maximum_block_size, maximum_channels, kDefaultReleaseSeconds);
            third_.prepare(sample_rate, maximum_block_size, maximum_channels, kDefaultReleaseSeconds);
            updateModes();
            setCeilingDecibels(ceiling_db_, safety_margin_db_);
            reset();
        }

        void reset() {
            first_.reset();
            second_.reset();
            third_.reset();
        }

        void setEnabled(const bool enabled) {
            if (enabled_ == enabled) {
                return;
            }
            enabled_ = enabled;
            updateModes();
        }

        void setOversamplingEnabled(const bool enabled) {
            if (oversampling_enabled_ == enabled) {
                return;
            }
            oversampling_enabled_ = enabled;
            updateModes();
        }

        void setCeilingDecibels(const FloatType ceiling_db, const FloatType safety_margin_db = kDefaultSafetyMarginDb) {
            ceiling_db_ = ceiling_db;
            safety_margin_db_ = std::max(safety_margin_db, FloatType(0));
            const auto tp_ceiling_db = ceiling_db_ - safety_margin_db_;
            const auto sp_ceiling_db = ceiling_db_ - kDefaultSampleMarginDb;
            first_.setCeilingDecibels(tp_ceiling_db, sp_ceiling_db);
            second_.setCeilingDecibels(tp_ceiling_db, sp_ceiling_db);
            third_.setCeilingDecibels(tp_ceiling_db, sp_ceiling_db);
        }

        void process(std::span<FloatType*> buffer, const size_t num_samples) {
            if (buffer.empty() || num_samples == 0) {
                return;
            }
            first_.process(buffer, num_samples);
            second_.process(buffer, num_samples);
            third_.process(buffer, num_samples);
        }

        [[nodiscard]] size_t getLatencySamples() const {
            return first_.getLatencySamples() + second_.getLatencySamples() + third_.getLatencySamples();
        }

    private:
        FloatType ceiling_db_{FloatType(-1)};
        FloatType safety_margin_db_{kDefaultSafetyMarginDb};
        bool enabled_{true};
        bool oversampling_enabled_{true};
        TruePeakCorrectionStage<FloatType> first_{};
        TruePeakCorrectionStage<FloatType> second_{};
        TruePeakCorrectionStage<FloatType> third_{};

        void updateModes() {
            if (enabled_) {
                first_.setMode(StageMode::kTruePeak);
                second_.setMode(StageMode::kTruePeak);
                third_.setMode(StageMode::kTruePeak);
            } else if (oversampling_enabled_) {
                first_.setMode(StageMode::kSamplePeak);
                second_.setMode(StageMode::kBypassed);
                third_.setMode(StageMode::kBypassed);
            } else {
                first_.setMode(StageMode::kBypassed);
                second_.setMode(StageMode::kBypassed);
                third_.setMode(StageMode::kBypassed);
            }
        }
    };
}
