// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <cmath>
#include <cstddef>

#include "../../vector/highway_import.hpp"

namespace zldsp::limiter {
    /** Converts positive dB attenuation to linear gain in place. */
    template <typename FloatType>
    HWY_INLINE void attenuationToGain(FloatType* HWY_RESTRICT attenuation, const size_t num_samples) {
        namespace hn = hwy::HWY_NAMESPACE;
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
