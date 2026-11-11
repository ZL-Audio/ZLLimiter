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
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace zlchore::rng {
    /**
     * allocation-free xoshiro256** with SplitMix seeding, after Blackman and Vigna
     * see licenses/xoshiro256starstar_LICENSE.txt for notices and sources
     */
    class Xoshiro256StarStar {
    public:
        using result_type = std::uint64_t;

        explicit constexpr Xoshiro256StarStar(const result_type seed_value = 0) noexcept {
            seed(seed_value);
        }

        constexpr void seed(const result_type seed_value = 0) noexcept {
            auto value = seed_value;
            // distinct SplitMix outputs prevent an all-zero state
            for (auto& word : state_) {
                word = splitMix(value);
            }
        }

        static constexpr result_type min() noexcept {
            return 0;
        }

        static constexpr result_type max() noexcept {
            return std::numeric_limits<result_type>::max();
        }

        constexpr result_type operator()() noexcept {
            const auto result = std::rotl(state_[1] * 5, 7) * 9;
            advance();
            return result;
        }

        /// Uniform in [0, 1), using the upper 53 bits.
        constexpr double nextDouble() noexcept {
            return static_cast<double>(operator()() >> 11) * 0x1p-53;
        }

        /// Discrete triangular in (-1, 1), with variance approximately 1/6.
        constexpr double nextTriangular() noexcept {
            const auto word = operator()();
            // jointly uniform halves; integer subtraction saves a conversion
            const auto first = static_cast<std::int64_t>(word >> 32);
            const auto second = static_cast<std::int64_t>(static_cast<std::uint32_t>(word));
            return static_cast<double>(first - second) * 0x1p-32;
        }

        /// Advances by 2^128 outputs to separate streams.
        constexpr void jump() noexcept {
            constexpr std::array<result_type, 4> jump_constants{
                0x180ec6d33cfd0aba, 0xd5a61266f0c9392c,
                0xa9582618e03fc9aa, 0x39abdc4529b1661c
            };
            std::array<result_type, 4> next_state{};
            for (const auto jump_constant : jump_constants) {
                for (int bit = 0; bit < 64; ++bit) {
                    if ((jump_constant & (result_type(1) << bit)) != 0) {
                        for (size_t index = 0; index < state_.size(); ++index) {
                            next_state[index] ^= state_[index];
                        }
                    }
                    advance();
                }
            }
            state_ = next_state;
        }

        friend constexpr bool operator==(const Xoshiro256StarStar&,
                                         const Xoshiro256StarStar&) noexcept = default;

    private:
        std::array<result_type, 4> state_{};

        static constexpr result_type splitMix(result_type& value) noexcept {
            value += 0x9e3779b97f4a7c15;
            auto result = value;
            result = (result ^ (result >> 30)) * 0xbf58476d1ce4e5b9;
            result = (result ^ (result >> 27)) * 0x94d049bb133111eb;
            return result ^ (result >> 31);
        }

        constexpr void advance() noexcept {
            const auto shifted = state_[1] << 17;
            state_[2] ^= state_[0];
            state_[3] ^= state_[1];
            state_[1] ^= state_[2];
            state_[0] ^= state_[3];
            state_[2] ^= shifted;
            state_[3] = std::rotl(state_[3], 45);
        }
    };
}
