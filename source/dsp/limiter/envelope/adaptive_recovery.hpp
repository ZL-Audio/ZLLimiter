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
            persistence_.setTimesSeconds(0.1, 0.2);
            depth_.prepare(sample_rate_);
            depth_.setTimesSeconds(0.005, 0.1);
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

        void setRecoverPercent(const FloatType percent) {
            const auto recover = std::clamp(static_cast<double>(percent), 0.0, 100.0);
            if (std::abs(recover - recover_percent_) < 1e-10) {
                return;
            }
            recover_percent_ = recover;
            updateTargets();
            ramp_remaining_ = ramp_samples_;
            fast_increment_ = (target_fast_step_ - fast_step_) / static_cast<double>(ramp_samples_);
            slow_increment_ = (target_slow_step_ - slow_step_) / static_cast<double>(ramp_samples_);
        }

        FloatType processSample(const FloatType maximum_planned_db) {
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
            return static_cast<FloatType>(fast_step_ + static_cast<double>(severity) * (slow_step_ - fast_step_));
        }

    private:
        double sample_rate_{48000.0};
        double recover_percent_{50.0};
        double fast_step_{-std::expm1(std::log(0.1) / (48000.0 * 0.05))};
        double slow_step_{-std::expm1(std::log(0.1) / (48000.0 * 0.25))};
        double target_fast_step_{fast_step_}, target_slow_step_{slow_step_};
        double fast_increment_{0.0}, slow_increment_{0.0};
        size_t ramp_samples_{960}, ramp_remaining_{0};
        AsymmetricFollower<FloatType> persistence_{}, depth_{};

        void updateTargets() {
            const auto fast_seconds = 0.1 * std::exp2(-recover_percent_ * 0.02);
            target_fast_step_ = -std::expm1(std::log(0.1) / (sample_rate_ * fast_seconds));
            target_slow_step_ = -std::expm1(std::log(0.1) / (sample_rate_ * fast_seconds * 5.0));
        }
    };
}
