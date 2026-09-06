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
#include <cstddef>

#include "../../vector/highway_import.hpp"

namespace zldsp::limiter {
    namespace hn = hwy::HWY_NAMESPACE;

    /**
     * a conservative interpolator from control-rate dB attenuation to linear gain
     * @tparam FloatType the audio sample type
     */
    template <typename FloatType>
    class AttenuationInterpolator {
    public:
        void reset(const FloatType attenuation_db = FloatType(0)) {
            previous_attenuation_db_ = attenuation_db;
            previous_endpoint_db_ = attenuation_db;
        }

        template <size_t Factor>
        void process(const FloatType* HWY_RESTRICT attenuation_db, FloatType* HWY_RESTRICT output_gain,
                     const size_t num_samples) {
            static_assert(Factor > 0);

            if (num_samples == 0) {
                return;
            }
            if constexpr (Factor == 1) {
                processUnity(attenuation_db, output_gain, num_samples);
            } else {
                processInterpolated<Factor>(attenuation_db, output_gain, num_samples);
            }
            const auto previous = num_samples == 1 ? previous_attenuation_db_ : attenuation_db[num_samples - 2];
            previous_endpoint_db_ = std::max(previous, attenuation_db[num_samples - 1]);
            previous_attenuation_db_ = attenuation_db[num_samples - 1];
        }

    private:
        static constexpr double kDbToLogGain{-0.1151292546497022842};

        void processUnity(const FloatType* HWY_RESTRICT attenuation_db, FloatType* HWY_RESTRICT output_gain,
                          const size_t num_samples) const {
            static constexpr hn::ScalableTag<FloatType> d;
            static constexpr size_t lanes = hn::MaxLanes(d);
            const auto scale = hn::Set(d, static_cast<FloatType>(kDbToLogGain));

            size_t i = 0;
            for (; i + lanes <= num_samples; i += lanes) {
                const auto previous = i == 0
                    ? hn::InsertLane(hn::Slide1Up(d, hn::LoadU(d, attenuation_db)), 0, previous_attenuation_db_)
                    : hn::LoadU(d, attenuation_db + i - 1);
                hn::StoreU(hn::Exp(d, hn::Mul(previous, scale)), d, output_gain + i);
            }
            for (; i < num_samples; ++i) {
                const auto previous = i == 0 ? previous_attenuation_db_ : attenuation_db[i - 1];
                output_gain[i] = static_cast<FloatType>(std::exp(static_cast<double>(previous) * kDbToLogGain));
            }
        }

        template <size_t Factor>
        void processInterpolated(const FloatType* HWY_RESTRICT attenuation_db, FloatType* HWY_RESTRICT output_gain,
                                 const size_t num_samples) const {
            static constexpr hn::ScalableTag<FloatType> d;
            static constexpr size_t lanes = hn::MaxLanes(d);
            static constexpr auto step_db_to_log_gain = kDbToLogGain / static_cast<double>(Factor);
            const auto scale = hn::Set(d, static_cast<FloatType>(kDbToLogGain));
            const auto step_scale = hn::Set(d, static_cast<FloatType>(step_db_to_log_gain));
            HWY_ALIGN FloatType gains[lanes];
            HWY_ALIGN FloatType ratios[lanes];
            auto previous_endpoint = previous_endpoint_db_;

            size_t i = 0;
            for (; i + lanes <= num_samples; i += lanes) {
                const auto current = hn::LoadU(d, attenuation_db + i);
                const auto previous = i == 0 ? hn::InsertLane(hn::Slide1Up(d, current), 0, previous_attenuation_db_)
                                             : hn::LoadU(d, attenuation_db + i - 1);
                const auto end = hn::Max(previous, current);
                const auto start = hn::InsertLane(hn::Slide1Up(d, end), 0, previous_endpoint);
                previous_endpoint = hn::ExtractLane(end, lanes - 1);
                const auto delta = hn::Sub(end, start);
                hn::StoreU(hn::Exp(d, hn::Mul(start, scale)), d, gains);
                hn::StoreU(hn::Exp(d, hn::Mul(delta, step_scale)), d, ratios);

                for (size_t lane = 0; lane < lanes; ++lane) {
                    auto gain = gains[lane];
                    auto* const output = output_gain + (i + lane) * Factor;
                    for (size_t phase = 0; phase < Factor; ++phase) {
                        output[phase] = gain;
                        gain *= ratios[lane];
                    }
                }
            }
            for (; i < num_samples; ++i) {
                const auto previous = i == 0 ? previous_attenuation_db_ : attenuation_db[i - 1];
                const auto end = std::max(previous, attenuation_db[i]);
                processScalar<Factor>(previous_endpoint, end, output_gain + i * Factor);
                previous_endpoint = end;
            }
        }

        template <size_t Factor>
        static void processScalar(const FloatType start_attenuation_db, const FloatType end_attenuation_db,
                                  FloatType* output_gain) {
            static constexpr auto step_db_to_log_gain = kDbToLogGain / static_cast<double>(Factor);
            auto gain = static_cast<FloatType>(std::exp(static_cast<double>(start_attenuation_db) * kDbToLogGain));
            const auto delta = static_cast<double>(end_attenuation_db - start_attenuation_db);
            const auto ratio = static_cast<FloatType>(std::exp(delta * step_db_to_log_gain));
            for (size_t phase = 0; phase < Factor; ++phase) {
                output_gain[phase] = gain;
                gain *= ratio;
            }
        }

        FloatType previous_attenuation_db_{FloatType(0)};
        FloatType previous_endpoint_db_{FloatType(0)};
    };
}
