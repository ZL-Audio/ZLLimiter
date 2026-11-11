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
#include <limits>

namespace zldsp::dither {
    /**
     * shared silence envelope; call once per frame with channel-linked activity
     * upcoming_active is upstream activity; output_active is pre-dither activity
     * output delay must stay within the prepared range
     */
    class SilenceGate {
    public:
        struct Settings {
            double hold_milliseconds{500.0};
            double fade_in_milliseconds{1.0};
            double warm_up_milliseconds{1.0};
            double fade_out_milliseconds{10.0};
        };

        void prepare(const double sample_rate, const size_t lookahead_samples) noexcept {
            prepare(sample_rate, lookahead_samples, lookahead_samples, Settings{});
        }

        void prepare(const double sample_rate, const size_t minimum_lookahead_samples,
                     const size_t maximum_lookahead_samples) noexcept {
            prepare(sample_rate, minimum_lookahead_samples, maximum_lookahead_samples, Settings{});
        }

        void prepare(const double sample_rate, const size_t minimum_lookahead_samples,
                     const size_t maximum_lookahead_samples, const Settings& settings) noexcept {
            const auto rate = std::isfinite(sample_rate) && sample_rate > 0.0 ? sample_rate : 48000.0;
            minimum_lookahead_samples_ = minimum_lookahead_samples;
            maximum_lookahead_samples_ = std::max(minimum_lookahead_samples, maximum_lookahead_samples);
            hold_samples_ = millisecondsToSamples(settings.hold_milliseconds, rate);
            fade_in_samples_ = millisecondsToSamples(settings.fade_in_milliseconds, rate);
            warm_up_samples_ = millisecondsToSamples(settings.warm_up_milliseconds, rate);
            fade_out_samples_ = millisecondsToSamples(settings.fade_out_milliseconds, rate);
            const auto required = getRequiredLookaheadSamples();
            opening_delay_samples_ = minimum_lookahead_samples_ > required
                ? minimum_lookahead_samples_ - required : 0;

            // prioritize warm-up when preview is short
            const auto available_fade = minimum_lookahead_samples_ > warm_up_samples_
                ? minimum_lookahead_samples_ - warm_up_samples_ : 0;
            opening_samples_ = std::min(fade_in_samples_, available_fade);
            prediction_hold_samples_ = addSaturated(maximum_lookahead_samples_, hold_samples_);
            reset();
        }

        void reset(const bool active = false) noexcept {
            gain_ = active ? 1.0 : 0.0;
            target_ = gain_;
            increment_ = 0.0;
            ramp_remaining_ = 0;
            opening_remaining_ = 0;
            opening_pending_ = false;
            activity_remaining_ = active ? hold_samples_ : 0;
            activity_pending_ = active;
        }

        /// Returns this frame's envelope; activity means exact nonzero program samples.
        /// Output activity forces full gain immediately.
        [[nodiscard]] double processSample(const bool upcoming_active, const bool output_active) noexcept {
            if (upcoming_active) {
                activity_remaining_ = std::max(activity_remaining_, prediction_hold_samples_);
                activity_pending_ = true;
                if (gain_ == 0.0 && !opening_pending_) {
                    opening_remaining_ = opening_delay_samples_;
                    opening_pending_ = true;
                }
            }

            if (output_active) {
                activity_remaining_ = std::max(activity_remaining_, hold_samples_);
                activity_pending_ = true;
                opening_pending_ = false;
                gain_ = 1.0;
                target_ = 1.0;
                increment_ = 0.0;
                ramp_remaining_ = 0;
            } else {
                const auto opening_due = opening_pending_ && opening_remaining_ == 0;
                if (opening_due || (activity_pending_ && gain_ > 0.0)) {
                    opening_pending_ = false;
                    startRamp(1.0, opening_samples_);
                } else if (!activity_pending_) {
                    startRamp(0.0, fade_out_samples_);
                }
                advanceRamp();
            }

            if (opening_pending_ && opening_remaining_ > 0) {
                --opening_remaining_;
            }
            if (activity_pending_) {
                if (activity_remaining_ > 0) {
                    --activity_remaining_;
                } else {
                    activity_pending_ = false;
                }
            }
            return gain_;
        }

        [[nodiscard]] double getGain() const noexcept {
            return gain_;
        }

        [[nodiscard]] bool isMuted() const noexcept {
            return gain_ == 0.0;
        }

        [[nodiscard]] size_t getWarmUpSamples() const noexcept {
            return warm_up_samples_;
        }

        [[nodiscard]] size_t getRequiredLookaheadSamples() const noexcept {
            return addSaturated(fade_in_samples_, warm_up_samples_);
        }

        // whether preview covers both fade-in and warm-up
        [[nodiscard]] bool hasFullPreroll() const noexcept {
            return minimum_lookahead_samples_ >= getRequiredLookaheadSamples();
        }

    private:
        static size_t addSaturated(const size_t first, const size_t second) noexcept {
            constexpr auto maximum = std::numeric_limits<size_t>::max();
            return first > maximum - second ? maximum : first + second;
        }

        static size_t millisecondsToSamples(const double milliseconds, const double sample_rate) noexcept {
            if (!std::isfinite(milliseconds) || milliseconds <= 0.0) {
                return 0;
            }
            constexpr auto maximum = std::numeric_limits<size_t>::max() / 4;
            const auto samples = std::ceil(milliseconds * (sample_rate * 0.001));
            return samples >= static_cast<double>(maximum) ? maximum : static_cast<size_t>(samples);
        }

        void startRamp(const double target, const size_t samples) noexcept {
            if (target_ == target) {
                return;
            }
            target_ = target;
            ramp_remaining_ = samples;
            if (samples == 0) {
                gain_ = target;
                increment_ = 0.0;
            } else {
                increment_ = (target - gain_) / static_cast<double>(samples);
            }
        }

        void advanceRamp() noexcept {
            if (ramp_remaining_ == 0) {
                return;
            }
            if (--ramp_remaining_ == 0) {
                gain_ = target_;
            } else {
                gain_ = std::clamp(gain_ + increment_, 0.0, 1.0);
            }
        }

        size_t minimum_lookahead_samples_{0}, maximum_lookahead_samples_{0};
        size_t hold_samples_{24000}, fade_in_samples_{48}, warm_up_samples_{48}, fade_out_samples_{480};
        size_t opening_samples_{0}, opening_delay_samples_{0}, prediction_hold_samples_{24000};
        size_t opening_remaining_{0}, activity_remaining_{0}, ramp_remaining_{0};
        double gain_{0.0}, target_{0.0}, increment_{0.0};
        bool opening_pending_{false}, activity_pending_{false};
    };
}
