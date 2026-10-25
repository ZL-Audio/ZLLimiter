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

#include "asymmetric_follower.hpp"

namespace zldsp::limiter {
    template <typename FloatType>
    class AdaptiveRecovery {
    public:
        void prepare(const double sample_rate) {
            sample_rate_ = std::max(sample_rate, 1.0);
            ramp_samples_ = std::max(static_cast<size_t>(1), static_cast<size_t>(sample_rate_ * 0.02));
            persistence_.prepare(sample_rate_);
            persistence_.setAttackMilliseconds(100.0);
            persistence_.setReleaseMilliseconds(200.0);
            depth_.prepare(sample_rate_);
            depth_.setAttackMilliseconds(5.0);
            depth_.setReleaseMilliseconds(100.0);
            updateTargets();
            reset();
        }

        void reset() {
            persistence_.reset();
            depth_.reset();
            fast_step_ = target_fast_step_;
            slow_step_ = target_slow_step_;
            ramp_remaining_ = 0;
        }

        void setRecoveryMilliseconds(const FloatType milliseconds) {
            if (std::abs(milliseconds - recovery_ms_) < 1e-10) {
                return;
            }
            recovery_ms_ = milliseconds;
            updateTargets();
            ramp_remaining_ = ramp_samples_;
            fast_increment_ = (target_fast_step_ - fast_step_) / static_cast<double>(ramp_samples_);
            slow_increment_ = (target_slow_step_ - slow_step_) / static_cast<double>(ramp_samples_);
        }

        FloatType processSample(const FloatType maximum_planned_db,
                                const FloatType low_frequency_weight = FloatType(0)) {
            if (ramp_remaining_ > 0) {
                fast_step_ += fast_increment_;
                slow_step_ += slow_increment_;
                if (--ramp_remaining_ == 0) {
                    fast_step_ = target_fast_step_;
                    slow_step_ = target_slow_step_;
                }
            }
            const auto demand = std::max(maximum_planned_db, FloatType(0));
            const auto activity = std::min(demand * FloatType(4), FloatType(1));
            const auto persistence = persistence_.processSample(activity);
            const auto depth = std::min(depth_.processSample(demand) / FloatType(12), FloatType(1));
            const auto severity = persistence + FloatType(0.25) * (FloatType(1) - persistence) * depth;
            const auto weight = std::clamp(low_frequency_weight, FloatType(0), FloatType(1));
            // bias toward the slow endpoint within the existing recovery range
            const auto spectral_severity = severity + kSpectralAmount * weight * (FloatType(1) - severity);
            return static_cast<FloatType>(
                fast_step_ + static_cast<double>(spectral_severity) * (slow_step_ - fast_step_));
        }

    private:
        static constexpr FloatType kSpectralAmount{FloatType(0.5)};
        double sample_rate_{48000.0};
        double recovery_ms_{50.0};
        double fast_step_{-std::expm1(std::log(0.1) / (sample_rate_ * recovery_ms_ * 0.001))};
        double slow_step_{-std::expm1(std::log(0.1) / (sample_rate_ * recovery_ms_ * 0.001 * 5.0))};
        double target_fast_step_{fast_step_}, target_slow_step_{slow_step_};
        double fast_increment_{0.0}, slow_increment_{0.0};
        size_t ramp_samples_{960}, ramp_remaining_{0};
        AsymmetricFollower<FloatType> persistence_{}, depth_{};

        void updateTargets() {
            if (recovery_ms_ <= 1e-3) {
                target_fast_step_ = 1.0;
                target_slow_step_ = 1.0;
                return;
            }
            // sustained limiting can extend it to 5x
            const auto fast_samples = sample_rate_ * recovery_ms_ * 0.001;
            target_fast_step_ = -std::expm1(std::log(0.1) / fast_samples);
            target_slow_step_ = -std::expm1(std::log(0.1) / (fast_samples * 5.0));
        }
    };
}
