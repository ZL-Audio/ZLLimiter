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

#include <bit>
#include <cmath>
#include <cstddef>

#include "../../vector/highway_import.hpp"

namespace zldsp::limiter {
    namespace hn = hwy::HWY_NAMESPACE;

    namespace detail {
        template <size_t FirstVector, size_t NumVectors, typename FloatType>
        HWY_INLINE auto maxAbsVectors(const FloatType* input) {
            static_assert(NumVectors > 0);

            static constexpr hn::ScalableTag<FloatType> d;
            static constexpr size_t lanes = hn::MaxLanes(d);

            if constexpr (NumVectors == 1) {
                return hn::Abs(hn::LoadU(d, input + FirstVector * lanes));
            } else {
                static constexpr size_t left_vectors = NumVectors / 2;
                return hn::Max(maxAbsVectors<FirstVector, left_vectors>(input),
                               maxAbsVectors<FirstVector + left_vectors, NumVectors - left_vectors>(input));
            }
        }

        template <size_t ProcessingFactor, typename FloatType>
        HWY_INLINE FloatType maxAbsGroup(const FloatType* input) {
            if constexpr (ProcessingFactor == 1) {
                return std::abs(input[0]);
            } else {
                static constexpr hn::ScalableTag<FloatType> d;
                static constexpr size_t lanes = hn::MaxLanes(d);

                if constexpr (ProcessingFactor < lanes) {
                    static constexpr hn::CappedTag<FloatType, ProcessingFactor> capped_d;
                    return hn::ReduceMax(capped_d, hn::Abs(hn::LoadU(capped_d, input)));
                } else {
                    static_assert(ProcessingFactor % lanes == 0);
                    static constexpr size_t num_vectors = ProcessingFactor / lanes;
                    return hn::ReduceMax(d, maxAbsVectors<0, num_vectors>(input));
                }
            }
        }
    }

    /**
     * conservatively pool processing-rate magnitudes into control intervals
     */
    template <size_t ProcessingFactor, typename FloatType>
    HWY_INLINE void poolPeaks(const FloatType* input, FloatType* output, const size_t control_samples) {
        static_assert(ProcessingFactor > 0);
        static_assert(std::has_single_bit(ProcessingFactor));

        static constexpr hn::ScalableTag<FloatType> d;
        static constexpr size_t lanes = hn::MaxLanes(d);

        size_t i = 0;
        if constexpr (ProcessingFactor == 1) {
            for (; i + lanes <= control_samples; i += lanes) {
                hn::StoreU(hn::Abs(hn::LoadU(d, input + i)), d, output + i);
            }
        }

        for (; i < control_samples; ++i) {
            output[i] = detail::maxAbsGroup<ProcessingFactor>(input + i * ProcessingFactor);
        }
    }
}
