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
#include <cstring>
#include <vector>

#include "../../vector/vector.hpp"
#include "true_peak_coefficients.hpp"

namespace zldsp::limiter {
    namespace hn = hwy::HWY_NAMESPACE;

    /**
     * 16x windowed-sinc peak estimation
     */
    template <typename FloatType>
    class TruePeakEstimator {
    public:
        static constexpr size_t kNumPhases = true_peak_coefficients::kNumPhases;
        static constexpr size_t kTapsPerPhase = true_peak_coefficients::kTapsPerPhase;
        static constexpr size_t kHistorySamples = kTapsPerPhase - 1;

        void prepare(const size_t maximum_channels, const size_t maximum_block_size) {
            histories_.resize(maximum_channels);
            for (auto& history : histories_) {
                history.resize(kHistorySamples + maximum_block_size);
            }
            reset();
        }

        void reset() {
            for (auto& history : histories_) {
                std::fill(history.begin(), history.end(), FloatType(0));
            }
        }

        FloatType processSample(const size_t channel, const FloatType sample) {
            auto& history = histories_[channel];
            history[kHistorySamples] = sample;
            const auto peak = evaluateSample(history.data(), sample);
            std::memmove(history.data(), history.data() + 1, kHistorySamples * sizeof(FloatType));
            return peak;
        }

        void processBlock(const size_t channel, const FloatType* HWY_RESTRICT input,
                          FloatType* HWY_RESTRICT output, const size_t num_samples) {
            if (num_samples == 0) {
                return;
            }
            static constexpr hn::ScalableTag<FloatType> d;
            static constexpr size_t lanes = hn::MaxLanes(d);
            auto& history = histories_[channel];
            vector::copy(history.data() + kHistorySamples, input, num_samples);

            size_t i = 0;
            for (; i + lanes <= num_samples; i += lanes) {
                auto peak = hn::Abs(hn::LoadU(d, input + i));
                // Four phase accumulators share each history load without keeping
                // all sixteen accumulators live on SSE2/NEON.
                for (size_t phase = 0; phase < kNumPhases; phase += 4) {
                    auto p0 = hn::Zero(d);
                    auto p1 = hn::Zero(d);
                    auto p2 = hn::Zero(d);
                    auto p3 = hn::Zero(d);
                    for (size_t tap = 0; tap < kTapsPerPhase; ++tap) {
                        const auto samples = hn::LoadU(d, history.data() + i + tap);
                        p0 = hn::MulAdd(samples, hn::Set(d, kCoefficients[phase][tap]), p0);
                        p1 = hn::MulAdd(samples, hn::Set(d, kCoefficients[phase + 1][tap]), p1);
                        p2 = hn::MulAdd(samples, hn::Set(d, kCoefficients[phase + 2][tap]), p2);
                        p3 = hn::MulAdd(samples, hn::Set(d, kCoefficients[phase + 3][tap]), p3);
                    }
                    peak = hn::Max(peak, hn::Max(hn::Max(hn::Abs(p0), hn::Abs(p1)),
                                                hn::Max(hn::Abs(p2), hn::Abs(p3))));
                }
                hn::StoreU(peak, d, output + i);
            }
            for (; i < num_samples; ++i) {
                output[i] = evaluateSample(history.data() + i, input[i]);
            }
            std::memmove(history.data(), history.data() + num_samples, kHistorySamples * sizeof(FloatType));
        }

    private:
        inline static constexpr auto kCoefficients = [] {
            std::array<std::array<FloatType, kTapsPerPhase>, kNumPhases> result{};
            for (size_t phase = 0; phase < kNumPhases; ++phase) {
                for (size_t tap = 0; tap < kTapsPerPhase; ++tap) {
                    result[phase][tap] = static_cast<FloatType>(true_peak_coefficients::kTable[phase][tap]);
                }
            }
            return result;
        }();

        static FloatType evaluateSample(const FloatType* history, const FloatType sample) {
            auto peak = std::abs(sample);
            for (const auto& phase : kCoefficients) {
                peak = std::max(peak, std::abs(vector::dot_product(history, phase.data(), kTapsPerPhase)));
            }
            return peak;
        }

        std::vector<vector::aligned_vector<FloatType>> histories_{};
    };
}
