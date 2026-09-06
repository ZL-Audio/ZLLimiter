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
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
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
                for (size_t pair = 0; pair < kNumPairs; pair += 2) {
                    auto e0 = hn::Zero(d);
                    auto o0 = hn::Zero(d);
                    auto e1 = hn::Zero(d);
                    auto o1 = hn::Zero(d);
                    for (size_t tap = 0; tap < kHalfTaps; ++tap) {
                        const auto left = hn::LoadU(d, history.data() + i + tap);
                        const auto right = hn::LoadU(d, history.data() + i + kHistorySamples - tap);
                        const auto sum = hn::Add(left, right);
                        const auto difference = hn::Sub(left, right);
                        e0 = hn::MulAdd(sum, hn::Set(d, kPairedCoefficients[pair].even[tap]), e0);
                        o0 = hn::MulAdd(difference, hn::Set(d, kPairedCoefficients[pair].odd[tap]), o0);
                        e1 = hn::MulAdd(sum, hn::Set(d, kPairedCoefficients[pair + 1].even[tap]), e1);
                        o1 = hn::MulAdd(difference, hn::Set(d, kPairedCoefficients[pair + 1].odd[tap]), o1);
                    }
                    peak = hn::Max(peak, hn::Max(hn::Add(hn::Abs(e0), hn::Abs(o0)),
                                                hn::Add(hn::Abs(e1), hn::Abs(o1))));
                }
                hn::StoreU(peak, d, output + i);
            }
            for (; i < num_samples; ++i) {
                output[i] = evaluateSample(history.data() + i, input[i]);
            }
            std::memmove(history.data(), history.data() + num_samples, kHistorySamples * sizeof(FloatType));
        }

    private:
        static constexpr size_t kNumPairs = kNumPhases / 2;
        static constexpr size_t kHalfTaps = kTapsPerPhase / 2;
        static_assert(kNumPhases % 4 == 0 && kTapsPerPhase % 2 == 0);
        static_assert([] {
            for (size_t phase = 0; phase < kNumPairs; ++phase) {
                for (size_t tap = 0; tap < kTapsPerPhase; ++tap) {
                    if (std::bit_cast<uint64_t>(true_peak_coefficients::kTable[phase][tap]) !=
                        std::bit_cast<uint64_t>(true_peak_coefficients::kTable[kNumPhases - 1 - phase][kHistorySamples - tap])) {
                        return false;
                    }
                }
            }
            return true;
        }(), "Paired FIR evaluation requires exactly reversed phase coefficients");

        struct PairedCoefficients {
            std::array<FloatType, kHalfTaps> even{};
            std::array<FloatType, kHalfTaps> odd{};
        };

        inline static constexpr auto kPairedCoefficients = [] {
            std::array<PairedCoefficients, kNumPairs> result{};
            for (size_t pair = 0; pair < kNumPairs; ++pair) {
                for (size_t tap = 0; tap < kHalfTaps; ++tap) {
                    const auto left = true_peak_coefficients::kTable[pair][tap];
                    const auto right = true_peak_coefficients::kTable[pair][kHistorySamples - tap];
                    result[pair].even[tap] = static_cast<FloatType>((left + right) * 0.5);
                    result[pair].odd[tap] = static_cast<FloatType>((left - right) * 0.5);
                }
            }
            return result;
        }();

        static FloatType evaluateSample(const FloatType* history, const FloatType sample) {
            std::array<FloatType, kHalfTaps> sums{}, differences{};
            for (size_t tap = 0; tap < kHalfTaps; ++tap) {
                sums[tap] = history[tap] + history[kHistorySamples - tap];
                differences[tap] = history[tap] - history[kHistorySamples - tap];
            }
            auto peak = std::abs(sample);
            for (const auto& pair : kPairedCoefficients) {
                const auto even = vector::dot_product(sums.data(), pair.even.data(), kHalfTaps);
                const auto odd = vector::dot_product(differences.data(), pair.odd.data(), kHalfTaps);
                peak = std::max(peak, std::abs(even) + std::abs(odd));
            }
            return peak;
        }

        std::vector<vector::aligned_vector<FloatType>> histories_{};
    };
}
