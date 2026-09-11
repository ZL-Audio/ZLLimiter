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
#include <algorithm>
#include <cmath>
#include <vector>

#include "../../container/fifo/fifo_base.hpp"

namespace zldsp::analyzer {
    class LimitReductionReceiver {
    public:
        static constexpr float kThresholdDB = -60.f;
        static constexpr float kThresholdLinear = 1e-3f;
        static constexpr float kKneeDB = -0.1f;

        static float applySoftKnee(const float raw_db) noexcept {
            if (raw_db >= 0.f) {
                return 0.f;
            }
            if (raw_db <= kKneeDB) {
                return raw_db;
            }
            const float t = raw_db / kKneeDB; // in [0, 1]
            return kKneeDB * t * t * (2.f - t);
        }

        LimitReductionReceiver() = default;

        void reset() {
            dbs_.fill(0.f);
        }

        void run(const zldsp::container::FIFORange range,
                 const std::vector<std::vector<float>>& gained_pre_samples,
                 const std::vector<std::vector<float>>& post_samples) {
            dbs_.fill(0.f);
            const auto num_channels = std::min({size_t(2), gained_pre_samples.size(), post_samples.size()});
            for (size_t channel = 0; channel < num_channels; ++channel) {
                const auto* pre = gained_pre_samples[channel].data();
                const auto* post = post_samples[channel].data();
                const auto fifo_size = static_cast<int>(gained_pre_samples[channel].size());
                if (fifo_size < 3) {
                    continue;
                }

                float min_gain = 1.f;
                const auto measure = [&](const int start, const int count) {
                    for (int i = 0; i < count; ++i) {
                        const int curr_idx = start + i;
                        const int prev_idx = curr_idx == 0 ? fifo_size - 1 : curr_idx - 1;
                        const int next_idx = curr_idx + 1 == fifo_size ? 0 : curr_idx + 1;

                        const auto pre_val = std::abs(pre[curr_idx]);
                        if (pre_val > kThresholdLinear && std::isfinite(pre_val)) {
                            const auto prev_val = std::abs(pre[prev_idx]);
                            const auto next_val = std::abs(pre[next_idx]);

                            if (pre_val >= prev_val && pre_val >= next_val) {
                                const auto post_prev = std::abs(post[prev_idx]);
                                const auto post_curr = std::abs(post[curr_idx]);
                                const auto post_next = std::abs(post[next_idx]);
                                const auto post_val = std::max({post_prev, post_curr, post_next});

                                if (std::isfinite(post_val) && post_val < min_gain * pre_val) {
                                    min_gain = post_val / pre_val;
                                }
                            }
                        }
                    }
                };
                measure(range.start_index1, range.block_size1);
                measure(range.start_index2, range.block_size2);
                if (min_gain < 1.f) {
                    const auto raw_db = 20.f * std::log10(std::max(min_gain, 1e-6f));
                    dbs_[channel] = applySoftKnee(raw_db);
                } else {
                    dbs_[channel] = 0.f;
                }
            }
        }

        [[nodiscard]] const std::array<float, 2>& getDBs() const noexcept {
            return dbs_;
        }

        [[nodiscard]] float getMinDB() const noexcept {
            return std::min(dbs_[0], dbs_[1]);
        }

    private:
        std::array<float, 2> dbs_{0.f, 0.f};
    };
}
