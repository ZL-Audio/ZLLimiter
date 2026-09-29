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
#include <type_traits>
#include <variant>
#include <vector>

#include "../chore/decibels.hpp"
#include "../delay/integer_delay.hpp"
#include "../over_sample/over_sample.hpp"
#include "../vector/vector.hpp"

#include "limiter_definitions.hpp"
#include "detector/peak_detector.hpp"
#include "envelope/safety_guardian.hpp"
#include "gain/attenuation_interpolator.hpp"
#include "gain/attenuation_to_gain.hpp"
#include "styles/clean.hpp"
#include "true_peak/true_peak_limiter.hpp"

namespace zldsp::limiter {
    /**
     * a limiter with optional oversampling and true-peak correction
     * @tparam FloatType the audio sample type
     * @tparam NumOversamplingStages the number of cascaded 2x stages
     * @tparam Style the attenuation planner
     */
    template <typename FloatType, size_t NumOversamplingStages = 2, typename Style = CleanStyle<FloatType>>
    class Limiter {
    public:
        static_assert(NumOversamplingStages <= 5);
        static constexpr size_t kProcessingFactor = size_t(1) << NumOversamplingStages;
        static constexpr size_t kPlanningFactor = std::min(kProcessingFactor, size_t(4));
        static constexpr size_t kPoolingFactor = kProcessingFactor / kPlanningFactor;

        void prepare(const double sample_rate, const size_t maximum_block_size, const size_t maximum_channels) {
            sample_rate_ = std::max(sample_rate, 1.0);
            const auto processing_rate = sample_rate_ * static_cast<double>(kProcessingFactor);
            const auto processing_block_size = maximum_block_size * kProcessingFactor;
            const auto planning_rate = sample_rate_ * static_cast<double>(kPlanningFactor);
            const auto planning_block_size = maximum_block_size * kPlanningFactor;

            if constexpr (NumOversamplingStages > 0) {
                oversampler_.prepare(maximum_channels, maximum_block_size);
            }
            style_.prepare(planning_rate, planning_block_size, maximum_channels, kMaximumLookaheadSeconds);

            gain_buffers_.resize(maximum_channels);
            gain_pointers_.resize(maximum_channels);
            for (size_t channel = 0; channel < maximum_channels; ++channel) {
                gain_buffers_[channel].resize(processing_block_size);
                gain_pointers_[channel] = gain_buffers_[channel].data();
            }

            if constexpr (kPoolingFactor > 1) {
                peak_buffers_.resize(maximum_channels);
                attenuation_buffers_.resize(maximum_channels);
                peak_pointers_.resize(maximum_channels);
                attenuation_pointers_.resize(maximum_channels);
                attenuation_interpolators_.resize(maximum_channels);
                for (size_t channel = 0; channel < maximum_channels; ++channel) {
                    peak_buffers_[channel].resize(planning_block_size);
                    attenuation_buffers_[channel].resize(planning_block_size);
                    peak_pointers_[channel] = peak_buffers_[channel].data();
                    attenuation_pointers_[channel] = attenuation_buffers_[channel].data();
                }
            }

            // preserve the base-rate audio delay and align planning-rate attenuation to it
            main_delay_samples_ = static_cast<size_t>(std::round(kMaximumLookaheadSeconds * sample_rate_)) + 1;
            const auto main_delay_processing_samples = main_delay_samples_ * kProcessingFactor;
            const auto main_delay_seconds =
                static_cast<FloatType>(static_cast<double>(main_delay_processing_samples) / processing_rate);
            main_delay_.prepare(processing_rate, processing_block_size, maximum_channels, main_delay_seconds);
            main_delay_.setDelayInSamples(static_cast<int>(main_delay_processing_samples));

            // conservative interpolation contributes one planning sample of delay
            constexpr size_t interpolation_delay_samples = kPoolingFactor > 1 ? 1 : 0;
            attenuation_delay_samples_ = main_delay_samples_ * kPlanningFactor -
                style_.getMaximumDelaySamples() - interpolation_delay_samples;
            const auto attenuation_delay_seconds =
                static_cast<FloatType>(static_cast<double>(attenuation_delay_samples_) / planning_rate);
            attenuation_delay_.prepare(planning_rate, planning_block_size, maximum_channels,
                                       attenuation_delay_seconds);
            attenuation_delay_.setDelayInSamples(static_cast<int>(attenuation_delay_samples_));

            guardian_delay_base_samples_ =
                static_cast<size_t>(std::ceil(kDefaultGuardianLookaheadSeconds * sample_rate_));
            guardian_.prepare(processing_rate, processing_block_size, maximum_channels,
                              guardian_delay_base_samples_ * kProcessingFactor);
            true_peak_limiter_.prepare(sample_rate_, maximum_block_size, maximum_channels,
                                       NumOversamplingStages > 0);

            latency_samples_ =
                main_delay_samples_ + guardian_delay_base_samples_ + true_peak_limiter_.getLatencySamples();
            if constexpr (NumOversamplingStages > 0) {
                latency_samples_ += oversampler_.getLatency();
            }
            reset();
            setOutputCeilingDecibels(output_ceiling_db_);
            setTruePeakEnabled(true_peak_enabled_);
        }

        void reset() {
            if constexpr (NumOversamplingStages > 0) {
                oversampler_.reset();
            }
            style_.reset();
            main_delay_.reset();
            main_delay_.setDelayInSamples(static_cast<int>(main_delay_samples_ * kProcessingFactor));
            attenuation_delay_.reset();
            attenuation_delay_.setDelayInSamples(static_cast<int>(attenuation_delay_samples_));
            for (auto& interpolator : attenuation_interpolators_) {
                interpolator.reset();
            }
            guardian_.reset();
            true_peak_limiter_.reset();
        }

        void setOutputCeilingDecibels(const FloatType ceiling_db) {
            output_ceiling_db_ = ceiling_db;
            final_ceiling_linear_ = chore::decibelsToGain(output_ceiling_db_);
            style_.setCeilingDecibels(output_ceiling_db_);
            guardian_.setCeilingLinear(chore::decibelsToGain(output_ceiling_db_));
            true_peak_limiter_.setCeilingDecibels(output_ceiling_db_);
        }

        void setTruePeakEnabled(const bool enabled) {
            true_peak_enabled_ = enabled;
            true_peak_limiter_.setEnabled(true_peak_enabled_);
        }

        void setLookaheadMilliseconds(const FloatType milliseconds) {
            style_.setLookaheadMilliseconds(milliseconds);
        }

        void setAttackMilliseconds(const FloatType milliseconds) {
            style_.setAttackMilliseconds(milliseconds);
        }

        void setReleaseMilliseconds(const FloatType milliseconds) {
            style_.setReleaseMilliseconds(milliseconds);
        }

        void setChannelDeltaDecibels(const FloatType decibels) {
            style_.setChannelDeltaDecibels(decibels);
        }

        void setRecoveryPercent(const FloatType percent) {
            style_.setRecoveryPercent(percent);
        }

        void process(std::span<FloatType*> buffer, const size_t num_samples) {
            if (buffer.empty() || num_samples == 0) {
                return;
            }
            const auto num_channels = buffer.size();
            const auto processing_samples = num_samples * kProcessingFactor;

            auto processing_buffer = buffer;
            // up-sample
            if constexpr (NumOversamplingStages > 0) {
                oversampler_.upsample(processing_buffer, num_samples);
                auto& processing_pointers = oversampler_.getOSPointer();
                processing_buffer = std::span<FloatType*>{processing_pointers.data(), num_channels};
            }
            const auto gains = std::span<FloatType*>{gain_pointers_.data(), num_channels};
            if constexpr (kPoolingFactor > 1) {
                // pool to 4x, plan attenuation, then interpolate back to the processing rate
                const auto planning_samples = num_samples * kPlanningFactor;
                for (size_t channel = 0; channel < num_channels; ++channel) {
                    poolPeaks<kPoolingFactor>(processing_buffer[channel], peak_buffers_[channel].data(),
                                             planning_samples);
                }
                const auto attenuation = std::span<FloatType*>{attenuation_pointers_.data(), num_channels};
                style_.process(std::span<const FloatType* const>{peak_pointers_.data(), num_channels},
                               attenuation, planning_samples);
                attenuation_delay_.process(attenuation, planning_samples);
                for (size_t channel = 0; channel < num_channels; ++channel) {
                    attenuation_interpolators_[channel].template process<kPoolingFactor>(
                        attenuation[channel], gains[channel], planning_samples);
                }
            } else {
                // plan attenuation directly from processing-rate samples through 4x
                style_.process(std::span<const FloatType* const>{processing_buffer.data(), num_channels},
                               gains, processing_samples);
                attenuation_delay_.process(gains, processing_samples);
                for (size_t channel = 0; channel < num_channels; ++channel) {
                    attenuationToGain(gains[channel], processing_samples);
                }
            }
            // main signal delay
            main_delay_.process(processing_buffer, processing_samples);
            for (size_t channel = 0; channel < num_channels; ++channel) {
                vector::multiply(processing_buffer[channel], gain_buffers_[channel].data(), processing_samples);
            }
            guardian_.process(processing_buffer, processing_samples);
            // down-sample
            if constexpr (NumOversamplingStages > 0) {
                oversampler_.downsample(buffer, num_samples);
            }
            true_peak_limiter_.process(buffer, num_samples);
            applyOutputClamp(buffer, num_samples);
        }

        [[nodiscard]] size_t getLatencySamples() const {
            return latency_samples_;
        }

        [[nodiscard]] Style& getStyle() {
            return style_;
        }

        [[nodiscard]] const Style& getStyle() const {
            return style_;
        }

    private:
        double sample_rate_{48000.0};
        size_t main_delay_samples_{0};
        size_t attenuation_delay_samples_{0};
        size_t guardian_delay_base_samples_{0};
        size_t latency_samples_{0};

        FloatType output_ceiling_db_{FloatType(-1)};
        FloatType final_ceiling_linear_{chore::decibelsToGain(FloatType(-1))};
        bool true_peak_enabled_{true};

        using OverSamplerType = std::conditional_t<NumOversamplingStages == 0, std::monostate,
                                                   oversample::OverSampler<FloatType, NumOversamplingStages>>;
        [[no_unique_address]] OverSamplerType oversampler_{};
        Style style_{};
        delay::IntegerDelay<FloatType> main_delay_{};
        delay::IntegerDelay<FloatType> attenuation_delay_{};
        SafetyGuardian<FloatType> guardian_{};
        TruePeakLimiter<FloatType> true_peak_limiter_{};

        std::vector<vector::aligned_vector<FloatType>> gain_buffers_{};
        std::vector<FloatType*> gain_pointers_{};
        std::vector<vector::aligned_vector<FloatType>> peak_buffers_{};
        std::vector<vector::aligned_vector<FloatType>> attenuation_buffers_{};
        std::vector<const FloatType*> peak_pointers_{};
        std::vector<FloatType*> attenuation_pointers_{};
        std::vector<AttenuationInterpolator<FloatType>> attenuation_interpolators_{};

        void applyOutputClamp(std::span<FloatType*> buffer, const size_t num_samples) {
            for (auto* channel : buffer) {
                vector::clamp(channel, -final_ceiling_linear_, final_ceiling_linear_, num_samples);
            }
        }
    };
}
