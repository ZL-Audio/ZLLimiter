// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include "../highway_import.hpp"

namespace zldsp::vector {
    namespace hn = hwy::HWY_NAMESPACE;

    template <typename F>
    HWY_INLINE F max_abs_of(const F* __restrict in, const size_t size) {
        static constexpr hn::ScalableTag<F> d;
        static constexpr size_t lanes = hn::MaxLanes(d);
        static constexpr size_t block = lanes << 2;

        auto maximum0 = hn::Zero(d);
        auto maximum1 = hn::Zero(d);
        auto maximum2 = hn::Zero(d);
        auto maximum3 = hn::Zero(d);
        size_t i = 0;
        for (; i + block <= size; i += block) {
            maximum0 = hn::Max(maximum0, hn::Abs(hn::LoadU(d, in + i)));
            maximum1 = hn::Max(maximum1, hn::Abs(hn::LoadU(d, in + i + lanes)));
            maximum2 = hn::Max(maximum2, hn::Abs(hn::LoadU(d, in + i + lanes * 2)));
            maximum3 = hn::Max(maximum3, hn::Abs(hn::LoadU(d, in + i + lanes * 3)));
        }
        auto vector_maximum = hn::Max(hn::Max(maximum0, maximum1), hn::Max(maximum2, maximum3));
        for (; i + lanes <= size; i += lanes) {
            auto v_in = hn::LoadU(d, in + i);
            auto v_abs = hn::Abs(v_in);
            vector_maximum = hn::Max(vector_maximum, v_abs);
        }
        F scalar_max_abs = hn::ReduceMax(d, vector_maximum);
        for (; i < size; ++i) {
            scalar_max_abs = std::max(scalar_max_abs, std::abs(in[i]));
        }
        return scalar_max_abs;
    }
}
