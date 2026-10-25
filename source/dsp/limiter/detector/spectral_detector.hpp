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
#include <cstddef>
#include <numbers>
#include <span>

#include "../../filter/iir_filter/tdf/tdf.hpp"
#include "../envelope/asymmetric_follower.hpp"

namespace zldsp::limiter {
    /**
     * a spectral detector for signed base-rate audio
     * @tparam FloatType the audio sample type
     */
    template <typename FloatType>
    class SpectralDetector {
    public:
        void prepare(const double sample_rate, const size_t maximum_channels) {
            const auto rate = std::max(sample_rate, 1.0);
            low_pass_.prepare(rate, maximum_channels, 0);
            low_pass_.forceUpdate({filter::FilterType::kLowPass, 2, kCrossoverFrequency, 0.0, kCrossoverQ});
            high_pass_.prepare(rate, maximum_channels, 0);
            high_pass_.forceUpdate({filter::FilterType::kHighPass, 2, kCrossoverFrequency, 0.0, kCrossoverQ});

            // use 90% settling times, as in the recovery followers
            low_energy_.prepare(rate);
            low_energy_.setAttackMilliseconds(kEnergyAttackMilliseconds);
            low_energy_.setReleaseMilliseconds(kEnergyReleaseMilliseconds);
            high_energy_.prepare(rate);
            high_energy_.setAttackMilliseconds(kEnergyAttackMilliseconds);
            high_energy_.setReleaseMilliseconds(kEnergyReleaseMilliseconds);
            low_frequency_weight_.prepare(rate);
            low_frequency_weight_.setAttackMilliseconds(kWeightAttackMilliseconds);
            low_frequency_weight_.setReleaseMilliseconds(kWeightReleaseMilliseconds);
            reset();
        }

        void reset() {
            low_pass_.reset();
            high_pass_.reset();
            low_energy_.reset();
            high_energy_.reset();
            low_frequency_weight_.reset();
        }

        void setCeilingLinear(const FloatType ceiling_linear) {
            const auto ceiling = std::max(static_cast<double>(ceiling_linear), 1e-12);
            energy_floor_ = ceiling * ceiling * kRelativeEnergyFloor;
        }

        void process(std::span<const FloatType* const> input, std::span<FloatType> low_frequency_weights) {
            if (input.empty()) {
                std::fill(low_frequency_weights.begin(), low_frequency_weights.end(), FloatType(0));
                return;
            }
            const auto inverse_channels = 1.0 / static_cast<double>(input.size());
            for (size_t i = 0; i < low_frequency_weights.size(); ++i) {
                double low_power{0.0}, high_power{0.0};
                for (size_t channel = 0; channel < input.size(); ++channel) {
                    const auto sample = static_cast<double>(input[channel][i]);
                    const auto low = low_pass_.template processSample<2>(channel, sample);
                    const auto high = high_pass_.template processSample<2>(channel, sample);
                    // sum powers to avoid cancellation between channels
                    low_power += low * low;
                    high_power += high * high;
                }
                const auto low = low_energy_.processSample(low_power * inverse_channels);
                const auto high = high_energy_.processSample(high_power * inverse_channels);
                const auto ratio = low / (low + high + energy_floor_);
                low_frequency_weights[i] = static_cast<FloatType>(
                    std::clamp(low_frequency_weight_.processSample(ratio), 0.0, 1.0));
            }
        }

    private:
        static constexpr double kCrossoverFrequency{200.0};
        static constexpr double kCrossoverQ{std::numbers::sqrt2 * 0.5};
        static constexpr double kEnergyAttackMilliseconds{40.0};
        static constexpr double kEnergyReleaseMilliseconds{40.0};
        static constexpr double kWeightAttackMilliseconds{10.0};
        static constexpr double kWeightReleaseMilliseconds{10.0};
        // fade out the score around -60 dB relative to the ceiling
        static constexpr double kRelativeEnergyFloor{1e-6};
        double energy_floor_{kRelativeEnergyFloor};
        filter::TDF<double, 1> low_pass_{}, high_pass_{};
        AsymmetricFollower<double> low_energy_{}, high_energy_{}, low_frequency_weight_{};
    };
}
