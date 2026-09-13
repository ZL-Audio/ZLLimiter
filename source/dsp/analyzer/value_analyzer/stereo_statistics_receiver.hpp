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
#include <array>
#include <cassert>
#include <cmath>
#include <limits>
#include <vector>

#include "../../container/fifo/fifo_base.hpp"
#include "../../vector/vector.hpp"

namespace zldsp::analyzer {
    class StereoStatisticsReceiver {
    public:
        void prepare(const double sample_rate, const size_t num_channels) {
            num_channels_ = num_channels;
            base_block_size_ = static_cast<size_t>(sample_rate / 10.0);
            block_size_remainder_ = sample_rate - static_cast<double>(base_block_size_) * 10.0;
            reset();
        }

        void reset() {
            block_ = {};
            history_.fill({});
            write_idx_ = 0;
            ready_count_ = 0;
            block_remainder_ = 0.0;
            nextBlock();
            correlation_ = std::numeric_limits<float>::quiet_NaN();
        }

        void run(const zldsp::container::FIFORange range,
                 const std::vector<std::vector<float>>& samples) {
            run(range, samples, [](float, double) {});
        }

        /** Reports correlation and its unweighted energy weight for every complete 400 ms window. */
        template <typename Callback>
        void run(const zldsp::container::FIFORange range,
                 const std::vector<std::vector<float>>& samples, Callback&& on_update) {
            assert(base_block_size_ > 0 && samples.size() >= num_channels_);
            const auto measure = [&](const int start, const int count) {
                size_t offset = 0;
                while (offset < static_cast<size_t>(count)) {
                    const auto size = std::min({kChunkSize, static_cast<size_t>(count) - offset,
                                                block_size_ - block_.num_samples});
                    for (size_t channel = 0; channel < num_channels_; ++channel) {
                        const auto* input = samples[channel].data() + static_cast<size_t>(start) + offset;
                        for (size_t i = 0; i < size; ++i) {
                            buffer_[channel][i] = std::isfinite(input[i]) ? input[i] : 0.f;
                        }
                    }
                    block_.left_energy += static_cast<double>(vector::sum_sqr(buffer_[0].data(), size));
                    if (num_channels_ == 2) {
                        block_.right_energy += static_cast<double>(vector::sum_sqr(buffer_[1].data(), size));
                        block_.cross_product += static_cast<double>(
                            vector::dot_product(buffer_[0].data(), buffer_[1].data(), size));
                    }
                    block_.num_samples += size;
                    offset += size;
                    if (block_.num_samples == block_size_) {
                        update(on_update);
                        block_ = {};
                        nextBlock();
                    }
                }
            };
            measure(range.start_index1, range.block_size1);
            measure(range.start_index2, range.block_size2);
        }

        [[nodiscard]] float getCorrelation() const noexcept {
            return correlation_;
        }

        [[nodiscard]] bool isReady() const noexcept {
            return ready_count_ == kBlockCount;
        }

    private:
        struct Statistics {
            double left_energy{0.0}, right_energy{0.0}, cross_product{0.0};
            size_t num_samples{0};
        };

        static constexpr size_t kChunkSize = 512;
        static constexpr size_t kBlockCount = 4;
        size_t num_channels_{0}, base_block_size_{0}, block_size_{0};
        double block_size_remainder_{0.0}, block_remainder_{0.0};
        Statistics block_;
        std::array<Statistics, kBlockCount> history_{};
        size_t write_idx_{0}, ready_count_{0};
        std::array<std::array<float, kChunkSize>, 2> buffer_{};
        float correlation_{std::numeric_limits<float>::quiet_NaN()};

        void nextBlock() noexcept {
            block_remainder_ += block_size_remainder_;
            const auto extra_sample = block_remainder_ >= 10.0;
            block_size_ = base_block_size_ + static_cast<size_t>(extra_sample);
            block_remainder_ -= extra_sample ? 10.0 : 0.0;
        }

        template <typename Callback>
        void update(Callback& on_update) {
            history_[write_idx_] = block_;
            write_idx_ = (write_idx_ + 1) % kBlockCount;
            ready_count_ = std::min(ready_count_ + 1, kBlockCount);
            if (!isReady()) {
                return;
            }
            Statistics total;
            for (const auto& block : history_) {
                total.left_energy += block.left_energy;
                total.right_energy += block.right_energy;
                total.cross_product += block.cross_product;
                total.num_samples += block.num_samples;
            }
            const auto mean_square = (total.left_energy + total.right_energy)
                / (static_cast<double>(total.num_samples) * static_cast<double>(num_channels_));
            correlation_ = num_channels_ == 2 && total.left_energy > 0.0 && total.right_energy > 0.0
                ? static_cast<float>(std::clamp(total.cross_product
                                                / (std::sqrt(total.left_energy)
                                                    * std::sqrt(total.right_energy)), -1.0, 1.0))
                : std::numeric_limits<float>::quiet_NaN();
            // Power times hop length is proportional to RMS^2 * elapsed time.
            // The common 1/sample_rate factor cancels in the weighted mean between resets.
            on_update(correlation_, mean_square * static_cast<double>(block_.num_samples));
        }
    };
}
