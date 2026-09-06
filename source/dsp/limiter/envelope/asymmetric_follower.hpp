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

namespace zldsp::limiter {
    /**
     * an asymmetric one-pole envelope follower
     * @tparam FloatType the audio sample type
     */
    template <typename FloatType>
    class AsymmetricFollower {
    public:
        void prepare(const double sample_rate) {
            sample_rate_ = std::max(sample_rate, 1.0);
            attack_step_ = stepForSeconds(attack_seconds_);
            release_step_ = stepForSeconds(release_seconds_);
        }

        void reset(const FloatType value = FloatType(0)) {
            state_ = static_cast<double>(value);
        }

        void setAttackSeconds(const double seconds) {
            attack_seconds_ = std::max(seconds, 0.0);
            attack_step_ = stepForSeconds(attack_seconds_);
        }

        void setReleaseSeconds(const double seconds) {
            release_seconds_ = std::max(seconds, 0.0);
            release_step_ = stepForSeconds(release_seconds_);
        }

        FloatType processSample(const FloatType target) {
            const auto step = static_cast<double>(target) >= state_ ? attack_step_ : release_step_;
            state_ += step * (static_cast<double>(target) - state_);
            return static_cast<FloatType>(state_);
        }

        [[nodiscard]] FloatType getCurrent() const {
            return static_cast<FloatType>(state_);
        }

    private:
        double sample_rate_{48000.0};
        double attack_seconds_{0.0};
        double release_seconds_{0.1};
        double attack_step_{1.0};
        double release_step_{1.0};
        double state_{0.0};

        [[nodiscard]] double stepForSeconds(const double seconds) const {
            if (seconds <= 0.0) {
                return 1.0;
            }
            const auto exponent = std::log(0.1) / (seconds * sample_rate_);
            return -std::expm1(exponent);
        }
    };
}
