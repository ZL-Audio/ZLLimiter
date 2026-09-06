// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <atomic>
#include <span>
#include <tuple>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

#include "../chore/thread/notifier.hpp"
#include "../dsp/delay/integer_delay.hpp"
#include "../dsp/limiter/limiter.hpp"
#include "../dsp/vector/vector.hpp"
#include "zlp_definitions.hpp"

namespace zlp {
    class Controller final : private juce::AsyncUpdater {
    public:
        explicit Controller(juce::AudioProcessor& processor);

        ~Controller() override;

        void prepare(double sample_rate, size_t max_num_samples);

        void process(std::span<float*> buffer, size_t num_samples, bool host_bypassed);

        void setInputGain(const float db) {
            input_gain_db_.store(db, std::memory_order_relaxed);
            to_update_input_gain_.signal();
            to_update_.signal();
        }

        void setOutputCeiling(const float db) {
            output_ceiling_db_.store(db, std::memory_order_relaxed);
            to_update_output_ceiling_.signal();
            to_update_.signal();
        }

        void setBypassEnabled(const bool enabled) {
            bypass_parameter_.store(enabled, std::memory_order_relaxed);
            to_update_output_mode_.signal();
            to_update_.signal();
        }

        void setDeltaEnabled(const bool enabled) {
            delta_parameter_.store(enabled, std::memory_order_relaxed);
            to_update_output_mode_.signal();
            to_update_.signal();
        }

        void setTruePeakEnabled(const bool enabled) {
            true_peak_enabled_.store(enabled, std::memory_order_relaxed);
            to_update_true_peak_.signal();
            to_update_.signal();
        }

        void setOversamplingIndex(const int index) {
            oversampling_index_parameter_.store(index, std::memory_order_relaxed);
            to_update_oversampling_.signal();
            to_update_.signal();
        }

        void setLookahead(const float milliseconds) {
            lookahead_ms_.store(milliseconds, std::memory_order_relaxed);
            to_update_lookahead_.signal();
            to_update_.signal();
        }

        void setAttack(const float milliseconds) {
            attack_ms_.store(milliseconds, std::memory_order_relaxed);
            to_update_attack_.signal();
            to_update_.signal();
        }

        void setRelease(const float milliseconds) {
            release_ms_.store(milliseconds, std::memory_order_relaxed);
            to_update_release_.signal();
            to_update_.signal();
        }

        void setChannelDelta(const float db) {
            channel_delta_db_.store(db, std::memory_order_relaxed);
            to_update_channel_delta_.signal();
            to_update_.signal();
        }

        void setRecover(const float percent) {
            recover_percent_.store(percent, std::memory_order_relaxed);
            to_update_recover_.signal();
            to_update_.signal();
        }

    private:
        juce::AudioProcessor& p_ref_;

        using Limiter1x = zldsp::limiter::Limiter<float, 0>;
        using Limiter2x = zldsp::limiter::Limiter<float, 1>;
        using Limiter4x = zldsp::limiter::Limiter<float, 2>;
        using Limiter8x = zldsp::limiter::Limiter<float, 3>;
        using Limiter16x = zldsp::limiter::Limiter<float, 4>;
        using Limiter32x = zldsp::limiter::Limiter<float, 5>;
        using LimiterTuple = std::tuple<Limiter1x, Limiter2x, Limiter4x, Limiter8x, Limiter16x, Limiter32x>;
        static constexpr size_t kOversamplingModeCount = std::tuple_size_v<LimiterTuple>;

        LimiterTuple limiters_{};
        zldsp::delay::IntegerDelay<float> dry_delay_{};
        std::vector<zldsp::vector::aligned_vector<float>> dry_buffers_{};
        std::vector<float*> dry_pointers_{};

        zlchore::thread::Notifier to_update_{true};
        zlchore::thread::Notifier to_update_input_gain_{true};
        zlchore::thread::Notifier to_update_output_ceiling_{true};
        zlchore::thread::Notifier to_update_output_mode_{true};
        zlchore::thread::Notifier to_update_true_peak_{true};
        zlchore::thread::Notifier to_update_oversampling_{true};
        zlchore::thread::Notifier to_update_lookahead_{true};
        zlchore::thread::Notifier to_update_attack_{true};
        zlchore::thread::Notifier to_update_release_{true};
        zlchore::thread::Notifier to_update_channel_delta_{true};
        zlchore::thread::Notifier to_update_recover_{true};

        std::atomic<float> input_gain_db_{PInputGain::kDefaultV};
        std::atomic<float> output_ceiling_db_{POutputCeiling::kDefaultV};
        std::atomic<bool> bypass_parameter_{PBypass::kDefaultV};
        std::atomic<bool> delta_parameter_{PDelta::kDefaultV};
        std::atomic<bool> true_peak_enabled_{PTruePeak::kDefaultV};
        std::atomic<int> oversampling_index_parameter_{POversampling::kDefaultI};
        std::atomic<float> lookahead_ms_{PLookahead::kDefaultV};
        std::atomic<float> attack_ms_{PAttack::kDefaultV};
        std::atomic<float> release_ms_{PRelease::kDefaultV};
        std::atomic<float> channel_delta_db_{PChannelDelta::kDefaultV};
        std::atomic<float> recover_percent_{PRecover::kDefaultV};

        bool is_prepared_{false};
        bool bypass_enabled_{PBypass::kDefaultV};
        bool delta_enabled_{PDelta::kDefaultV};
        size_t oversampling_index_{kOversamplingModeCount};
        std::atomic<int> pending_latency_samples_{0};

        void signalParameterUpdates();

        void prepareBuffer();

        void prepareLimiters(double sample_rate, size_t maximum_block_size, size_t maximum_channels);

        void selectOversampling(size_t oversampling_index);

        void resetActiveLimiter();

        void processActiveLimiter(std::span<float*> buffer, size_t num_samples);

        [[nodiscard]] size_t getActiveLatencySamples() const;

        [[nodiscard]] size_t getMaximumLatencySamples() const;

        void handleAsyncUpdate() override;
    };
}
