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
            history_.fill({});
        }

        void run(const zldsp::container::FIFORange range,
                 const std::vector<std::vector<float>>& gained_pre_samples,
                 const std::vector<std::vector<float>>& post_samples) {
            dbs_.fill(0.f);
            const auto num_channels = std::min({size_t(2), gained_pre_samples.size(), post_samples.size()});
            for (size_t channel = 0; channel < num_channels; ++channel) {
                const auto* pre = gained_pre_samples[channel].data();
                const auto* post = post_samples[channel].data();
                auto history = history_[channel];

                float min_gain = 1.f;
                const auto measure = [&](const int start, const int count) {
                    for (int i = 0; i < count; ++i) {
                        const auto pre_next = std::abs(pre[start + i]);
                        const auto post_next = std::abs(post[start + i]);

                        // evaluate the preceding sample only after its right neighbor arrives
                        const auto pre_val = history.pre_curr;
                        if (pre_val > kThresholdLinear && std::isfinite(pre_val)) {
                            if (pre_val >= history.pre_prev && pre_val >= pre_next) {
                                const auto post_val = std::max({history.post_prev, history.post_curr, post_next});

                                if (std::isfinite(post_val) && post_val < min_gain * pre_val) {
                                    min_gain = post_val / pre_val;
                                }
                            }
                        }

                        history.pre_prev = history.pre_curr;
                        history.pre_curr = pre_next;
                        history.post_prev = history.post_curr;
                        history.post_curr = post_next;
                    }
                };
                measure(range.start_index1, range.block_size1);
                measure(range.start_index2, range.block_size2);
                history_[channel] = history;
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
        struct History {
            float pre_prev{0.f};
            float pre_curr{0.f};
            float post_prev{0.f};
            float post_curr{0.f};
        };

        std::array<History, 2> history_{};
        std::array<float, 2> dbs_{0.f, 0.f};
    };
}
