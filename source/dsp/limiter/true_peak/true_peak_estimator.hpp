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
                if constexpr (kPairsPerPass == 8) {
                    peak = processPass8(d, history.data() + i, peak);
                } else if constexpr (kPairsPerPass == 4) {
                    peak = processPass4<0>(d, history.data() + i, peak);
                    peak = processPass4<4>(d, history.data() + i, peak);
                } else {
                    peak = processPass2<0>(d, history.data() + i, peak);
                    peak = processPass2<2>(d, history.data() + i, peak);
                    peak = processPass2<4>(d, history.data() + i, peak);
                    peak = processPass2<6>(d, history.data() + i, peak);
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

#if defined(HWY_REGISTERS) && (HWY_REGISTERS >= 32)
        static constexpr size_t kPairsPerPass = 8;
#elif defined(HWY_REGISTERS) && (HWY_REGISTERS >= 16) && (HWY_ARCH_X86_64 || !HWY_ARCH_X86)
        static constexpr size_t kPairsPerPass = 4;
#else
        static constexpr size_t kPairsPerPass = 2;
#endif
        static_assert(kNumPairs % kPairsPerPass == 0);

        template <typename D, typename Vec>
        HWY_INLINE static Vec processPass8(const D d, const FloatType* HWY_RESTRICT history, Vec peak) {
            auto e0 = hn::Zero(d), o0 = hn::Zero(d);
            auto e1 = hn::Zero(d), o1 = hn::Zero(d);
            auto e2 = hn::Zero(d), o2 = hn::Zero(d);
            auto e3 = hn::Zero(d), o3 = hn::Zero(d);
            auto e4 = hn::Zero(d), o4 = hn::Zero(d);
            auto e5 = hn::Zero(d), o5 = hn::Zero(d);
            auto e6 = hn::Zero(d), o6 = hn::Zero(d);
            auto e7 = hn::Zero(d), o7 = hn::Zero(d);

            for (size_t tap = 0; tap < kHalfTaps; ++tap) {
                const auto left = hn::LoadU(d, history + tap);
                const auto right = hn::LoadU(d, history + kHistorySamples - tap);
                const auto sum = hn::Add(left, right);
                const auto difference = hn::Sub(left, right);

                e0 = hn::MulAdd(sum, hn::Set(d, kPairedCoefficients[0].even[tap]), e0);
                o0 = hn::MulAdd(difference, hn::Set(d, kPairedCoefficients[0].odd[tap]), o0);
                e1 = hn::MulAdd(sum, hn::Set(d, kPairedCoefficients[1].even[tap]), e1);
                o1 = hn::MulAdd(difference, hn::Set(d, kPairedCoefficients[1].odd[tap]), o1);
                e2 = hn::MulAdd(sum, hn::Set(d, kPairedCoefficients[2].even[tap]), e2);
                o2 = hn::MulAdd(difference, hn::Set(d, kPairedCoefficients[2].odd[tap]), o2);
                e3 = hn::MulAdd(sum, hn::Set(d, kPairedCoefficients[3].even[tap]), e3);
                o3 = hn::MulAdd(difference, hn::Set(d, kPairedCoefficients[3].odd[tap]), o3);
                e4 = hn::MulAdd(sum, hn::Set(d, kPairedCoefficients[4].even[tap]), e4);
                o4 = hn::MulAdd(difference, hn::Set(d, kPairedCoefficients[4].odd[tap]), o4);
                e5 = hn::MulAdd(sum, hn::Set(d, kPairedCoefficients[5].even[tap]), e5);
                o5 = hn::MulAdd(difference, hn::Set(d, kPairedCoefficients[5].odd[tap]), o5);
                e6 = hn::MulAdd(sum, hn::Set(d, kPairedCoefficients[6].even[tap]), e6);
                o6 = hn::MulAdd(difference, hn::Set(d, kPairedCoefficients[6].odd[tap]), o6);
                e7 = hn::MulAdd(sum, hn::Set(d, kPairedCoefficients[7].even[tap]), e7);
                o7 = hn::MulAdd(difference, hn::Set(d, kPairedCoefficients[7].odd[tap]), o7);
            }

            const auto p01 = hn::Max(hn::Add(hn::Abs(e0), hn::Abs(o0)),
                                     hn::Add(hn::Abs(e1), hn::Abs(o1)));
            const auto p23 = hn::Max(hn::Add(hn::Abs(e2), hn::Abs(o2)),
                                     hn::Add(hn::Abs(e3), hn::Abs(o3)));
            const auto p45 = hn::Max(hn::Add(hn::Abs(e4), hn::Abs(o4)),
                                     hn::Add(hn::Abs(e5), hn::Abs(o5)));
            const auto p67 = hn::Max(hn::Add(hn::Abs(e6), hn::Abs(o6)),
                                     hn::Add(hn::Abs(e7), hn::Abs(o7)));

            return hn::Max(peak, hn::Max(hn::Max(p01, p23), hn::Max(p45, p67)));
        }

        template <size_t BasePair, typename D, typename Vec>
        HWY_INLINE static Vec processPass4(const D d, const FloatType* HWY_RESTRICT history, Vec peak) {
            auto e0 = hn::Zero(d), o0 = hn::Zero(d);
            auto e1 = hn::Zero(d), o1 = hn::Zero(d);
            auto e2 = hn::Zero(d), o2 = hn::Zero(d);
            auto e3 = hn::Zero(d), o3 = hn::Zero(d);

            for (size_t tap = 0; tap < kHalfTaps; ++tap) {
                const auto left = hn::LoadU(d, history + tap);
                const auto right = hn::LoadU(d, history + kHistorySamples - tap);
                const auto sum = hn::Add(left, right);
                const auto difference = hn::Sub(left, right);

                e0 = hn::MulAdd(sum, hn::Set(d, kPairedCoefficients[BasePair + 0].even[tap]), e0);
                o0 = hn::MulAdd(difference, hn::Set(d, kPairedCoefficients[BasePair + 0].odd[tap]), o0);
                e1 = hn::MulAdd(sum, hn::Set(d, kPairedCoefficients[BasePair + 1].even[tap]), e1);
                o1 = hn::MulAdd(difference, hn::Set(d, kPairedCoefficients[BasePair + 1].odd[tap]), o1);
                e2 = hn::MulAdd(sum, hn::Set(d, kPairedCoefficients[BasePair + 2].even[tap]), e2);
                o2 = hn::MulAdd(difference, hn::Set(d, kPairedCoefficients[BasePair + 2].odd[tap]), o2);
                e3 = hn::MulAdd(sum, hn::Set(d, kPairedCoefficients[BasePair + 3].even[tap]), e3);
                o3 = hn::MulAdd(difference, hn::Set(d, kPairedCoefficients[BasePair + 3].odd[tap]), o3);
            }

            const auto p01 = hn::Max(hn::Add(hn::Abs(e0), hn::Abs(o0)),
                                     hn::Add(hn::Abs(e1), hn::Abs(o1)));
            const auto p23 = hn::Max(hn::Add(hn::Abs(e2), hn::Abs(o2)),
                                     hn::Add(hn::Abs(e3), hn::Abs(o3)));

            return hn::Max(peak, hn::Max(p01, p23));
        }

        template <size_t BasePair, typename D, typename Vec>
        HWY_INLINE static Vec processPass2(const D d, const FloatType* HWY_RESTRICT history, Vec peak) {
            auto e0 = hn::Zero(d), o0 = hn::Zero(d);
            auto e1 = hn::Zero(d), o1 = hn::Zero(d);

            for (size_t tap = 0; tap < kHalfTaps; ++tap) {
                const auto left = hn::LoadU(d, history + tap);
                const auto right = hn::LoadU(d, history + kHistorySamples - tap);
                const auto sum = hn::Add(left, right);
                const auto difference = hn::Sub(left, right);

                e0 = hn::MulAdd(sum, hn::Set(d, kPairedCoefficients[BasePair + 0].even[tap]), e0);
                o0 = hn::MulAdd(difference, hn::Set(d, kPairedCoefficients[BasePair + 0].odd[tap]), o0);
                e1 = hn::MulAdd(sum, hn::Set(d, kPairedCoefficients[BasePair + 1].even[tap]), e1);
                o1 = hn::MulAdd(difference, hn::Set(d, kPairedCoefficients[BasePair + 1].odd[tap]), o1);
            }

            const auto p01 = hn::Max(hn::Add(hn::Abs(e0), hn::Abs(o0)),
                                     hn::Add(hn::Abs(e1), hn::Abs(o1)));

            return hn::Max(peak, p01);
        }

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
