// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <array>
#include <cmath>
#include <cstddef>

#include "../filter/iir_filter/coeff/ivantsov_coeff.hpp"

namespace zldsp::dither {
    /**
     * three-section fit of the normalized 48 kHz SSRC ATH A1 curve
     * flat reference above 24 kHz; not the native higher-rate SSRC presets
     * see licenses/SSRC_LICENSE.txt for the reference and license
     */
    class ATHAShaper {
    public:
        static constexpr size_t kNumSections = 3;

        struct State {
            std::array<std::array<double, 2>, kNumSections> sections{};
            double correction{0.0};

            void reset() noexcept {
                sections = {};
                correction = 0.0;
            }
        };

        /// Updates shared coefficients only; returns false outside 44.1-192 kHz.
        bool prepare(const double sample_rate) noexcept {
            available_ = false;
            coefficients_ = {};
            if (!std::isfinite(sample_rate) || sample_rate < 44100.0 || sample_rate > 192000.0) {
                return false;
            }

            const std::array<std::array<double, 5>, kNumSections> raw_coefficients{
                filter::IvantsovCoeff::get2HighShelf(
                    9740.985170950387 / sample_rate, 9.480161348593391, 1.1140091493975295),
                filter::IvantsovCoeff::get2Peak(
                    3510.42157116189 / sample_rate, -11.483988313091132, 1.1698082072199976),
                filter::IvantsovCoeff::get2Peak(
                    11441.222001232594 / sample_rate, -7.229777965673751, 2.3783500947462755)
            };
            for (size_t index = 0; index < kNumSections; ++index) {
                const auto& source = raw_coefficients[index];
                // normalize b0 to one for strictly causal feedback
                const Coefficients coefficient{
                    source[3] / source[2], source[4] / source[2], source[0], source[1]
                };
                if (!isStable(coefficient.a1, coefficient.a2) ||
                    !isStable(coefficient.b1, coefficient.b2)) {
                    coefficients_ = {};
                    return false;
                }
                coefficients_[index] = coefficient;
            }
            available_ = true;
            return true;
        }

        [[nodiscard]] bool isAvailable() const noexcept {
            return available_;
        }

        /// Filters q - v (including dither) into the next sample's correction.
        void processError(const double error, State& state) const noexcept {
            auto input = error;
            double next_correction{0.0};
            for (size_t index = 0; index < kNumSections; ++index) {
                const auto& coefficient = coefficients_[index];
                auto& memory = state.sections[index];
                const auto output = input + memory[0];
                const auto first = coefficient.b1 * input - coefficient.a1 * output + memory[1];
                const auto second = coefficient.b2 * input - coefficient.a2 * output;
                memory = {first, second};
                // with b0 = 1, the next state-only cascade output is this sum
                next_correction += first;
                input = output;
            }
            state.correction = next_correction;
        }

    private:
        struct Coefficients {
            double b1{0.0}, b2{0.0}, a1{0.0}, a2{0.0};
        };

        std::array<Coefficients, kNumSections> coefficients_{};
        bool available_{false};

        static bool isStable(const double first, const double second) noexcept {
            return std::isfinite(first) && std::isfinite(second) && std::abs(second) < 1.0 &&
                1.0 + first + second > 0.0 && 1.0 - first + second > 0.0;
        }
    };
}
