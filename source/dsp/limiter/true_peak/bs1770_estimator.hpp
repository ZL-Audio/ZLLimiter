// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
// License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more
// details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see
// <https://www.gnu.org/licenses/>.

#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <vector>

#include "../../vector/vector.hpp"

namespace zldsp::limiter {
    /**
     * a streaming ITU-R BS.1770 Annex 2 true-peak estimator
     * @tparam FloatType the audio sample type
     */
    template <typename FloatType>
    class BS1770TruePeakEstimator {
    public:
        static constexpr size_t kNumPhases = 4;
        static constexpr size_t kTapsPerPhase = 12;

        void prepare(const size_t maximum_channels) {
            histories_.resize(std::max<size_t>(maximum_channels, 1));
            write_positions_.resize(histories_.size());
            for (auto& history : histories_) {
                history.resize(kTapsPerPhase * 2);
            }
            reset();
        }

        void reset() {
            for (auto& history : histories_) {
                std::fill(history.begin(), history.end(), FloatType(0));
            }
            std::fill(write_positions_.begin(), write_positions_.end(), size_t(0));
        }

        FloatType processSample(const size_t channel, const FloatType sample) {
            assert(channel < histories_.size());
            auto& history = histories_[channel];
            auto& write_position = write_positions_[channel];
            history[write_position] = sample;
            history[write_position + kTapsPerPhase] = sample;
            write_position += 1;
            if (write_position == kTapsPerPhase) {
                write_position = 0;
            }

            const auto* oldest_to_newest = history.data() + write_position;
            auto peak = std::abs(sample);
            for (const auto& phase : kCoefficients) {
                peak = std::max(peak, std::abs(vector::dot_product(oldest_to_newest, phase.data(), kTapsPerPhase)));
            }
            return peak;
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

        std::vector<vector::aligned_vector<FloatType>> histories_{};
        std::vector<size_t> write_positions_{};
    };
}
