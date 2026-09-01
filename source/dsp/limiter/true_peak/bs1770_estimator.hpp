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
#include <cstring>
#include <vector>

#include "../../vector/vector.hpp"

namespace zldsp::limiter {
    namespace hn = hwy::HWY_NAMESPACE;

    /**
     * a streaming ITU-R BS.1770 Annex 2 true-peak estimator
     * @tparam FloatType the audio sample type
     */
    template <typename FloatType>
    class BS1770TruePeakEstimator {
    public:
        static constexpr size_t kNumPhases = 4;
        static constexpr size_t kTapsPerPhase = 12;
        static constexpr size_t kHistorySamples = kTapsPerPhase - 1;

        void prepare(const size_t maximum_channels, const size_t maximum_block_size) {
            maximum_block_size_ = std::max<size_t>(maximum_block_size, 1);
            histories_.resize(std::max<size_t>(maximum_channels, 1));
            for (auto& history : histories_) {
                history.resize(kHistorySamples + maximum_block_size_);
            }
            reset();
        }

        void reset() {
            for (auto& history : histories_) {
                std::fill(history.begin(), history.end(), FloatType(0));
            }
        }

        FloatType processSample(const size_t channel, const FloatType sample) {
            assert(channel < histories_.size());
            auto& history = histories_[channel];
            history[kHistorySamples] = sample;
            auto peak = std::abs(sample);
            for (const auto& phase : kCoefficients) {
                peak = std::max(peak, std::abs(vector::dot_product(history.data(), phase.data(), kTapsPerPhase)));
            }
            std::memmove(history.data(), history.data() + 1, kHistorySamples * sizeof(FloatType));
            return peak;
        }

        void processBlock(const size_t channel, const FloatType* HWY_RESTRICT input, FloatType* HWY_RESTRICT output,
                          const size_t num_samples) {
            assert(channel < histories_.size());
            assert(num_samples <= maximum_block_size_);
            if (num_samples == 0) {
                return;
            }

            static constexpr hn::ScalableTag<FloatType> d;
            static constexpr size_t lanes = hn::MaxLanes(d);
            auto& history = histories_[channel];
            vector::copy(history.data() + kHistorySamples, input, num_samples);

            size_t i = 0;
            for (; i + lanes <= num_samples; i += lanes) {
                auto phase0 = hn::Zero(d);
                auto phase1 = hn::Zero(d);
                auto phase2 = hn::Zero(d);
                auto phase3 = hn::Zero(d);
                for (size_t tap = 0; tap < kTapsPerPhase; ++tap) {
                    const auto samples = hn::LoadU(d, history.data() + i + tap);
                    phase0 = hn::MulAdd(samples, hn::Set(d, kCoefficients[0][tap]), phase0);
                    phase1 = hn::MulAdd(samples, hn::Set(d, kCoefficients[1][tap]), phase1);
                    phase2 = hn::MulAdd(samples, hn::Set(d, kCoefficients[2][tap]), phase2);
                    phase3 = hn::MulAdd(samples, hn::Set(d, kCoefficients[3][tap]), phase3);
                }
                const auto reconstructed =
                    hn::Max(hn::Max(hn::Abs(phase0), hn::Abs(phase1)), hn::Max(hn::Abs(phase2), hn::Abs(phase3)));
                const auto stored = hn::Abs(hn::LoadU(d, input + i));
                hn::StoreU(hn::Max(stored, reconstructed), d, output + i);
            }
            for (; i < num_samples; ++i) {
                auto peak = std::abs(input[i]);
                for (const auto& phase : kCoefficients) {
                    peak =
                        std::max(peak, std::abs(vector::dot_product(history.data() + i, phase.data(), kTapsPerPhase)));
                }
                output[i] = peak;
            }

            std::memmove(history.data(), history.data() + num_samples, kHistorySamples * sizeof(FloatType));
        }

    private:
        inline static constexpr std::array<std::array<FloatType, kTapsPerPhase>, kNumPhases> kCoefficients{
            {{{FloatType(-0.0083007812500), FloatType(0.0148925781250), FloatType(-0.0266113281250),
               FloatType(0.0476074218750), FloatType(-0.1022949218750), FloatType(0.9721679687500),
               FloatType(0.1373291015625), FloatType(-0.0594482421875), FloatType(0.0332031250000),
               FloatType(-0.0196533203125), FloatType(0.0109863281250), FloatType(0.0017089843750)}},
             {{FloatType(-0.0189208984375), FloatType(0.0330810546875), FloatType(-0.0582275390625),
               FloatType(0.1015625000000), FloatType(-0.2003173828125), FloatType(0.7797851562500),
               FloatType(0.4650878906250), FloatType(-0.1665039062500), FloatType(0.0891113281250),
               FloatType(-0.0517578125000), FloatType(0.0292968750000), FloatType(-0.0291748046875)}},
             {{FloatType(-0.0291748046875), FloatType(0.0292968750000), FloatType(-0.0517578125000),
               FloatType(0.0891113281250), FloatType(-0.1665039062500), FloatType(0.4650878906250),
               FloatType(0.7797851562500), FloatType(-0.2003173828125), FloatType(0.1015625000000),
               FloatType(-0.0582275390625), FloatType(0.0330810546875), FloatType(-0.0189208984375)}},
             {{FloatType(0.0017089843750), FloatType(0.0109863281250), FloatType(-0.0196533203125),
               FloatType(0.0332031250000), FloatType(-0.0594482421875), FloatType(0.1373291015625),
               FloatType(0.9721679687500), FloatType(-0.1022949218750), FloatType(0.0476074218750),
               FloatType(-0.0266113281250), FloatType(0.0148925781250), FloatType(-0.0083007812500)}}}};

        size_t maximum_block_size_{0};
        std::vector<vector::aligned_vector<FloatType>> histories_{};
    };
}
