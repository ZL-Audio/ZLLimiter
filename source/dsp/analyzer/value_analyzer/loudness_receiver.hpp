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
#include <span>
#include <vector>

#include "../../container/fifo/fifo_base.hpp"
#include "../../loudness/lufs_meter.hpp"

namespace zldsp::analyzer {
    class LoudnessReceiver {
    public:
        void prepare(const double sample_rate, const size_t num_channels) {
            assert(num_channels > 0 && num_channels <= 2);
            num_channels_ = num_channels;
            meter_.prepare(sample_rate, num_channels_);
            has_started_ = false;
        }

        void reset() {
            meter_.reset();
            has_started_ = false;
        }

        void run(const zldsp::container::FIFORange range,
                 const std::vector<std::vector<float>>& samples) {
            run(range, samples, [](const auto&) {
            });
        }

        template <typename Callback>
        void run(const zldsp::container::FIFORange range,
                 const std::vector<std::vector<float>>& samples, Callback&& on_update) {
            assert(num_channels_ > 0 && samples.size() >= num_channels_);
            const auto measure = [&](const int start, const int count) {
                for (size_t offset = 0; offset < static_cast<size_t>(count); offset += kChunkSize) {
                    const auto size = std::min(kChunkSize, static_cast<size_t>(count) - offset);
                    for (size_t channel = 0; channel < num_channels_; ++channel) {
                        const auto* input = samples[channel].data() + static_cast<size_t>(start) + offset;
                        for (size_t i = 0; i < size; ++i) {
                            buffer_[channel][i] = std::isfinite(input[i]) ? input[i] : 0.f;
                        }
                    }
                    size_t first_sample = 0;
                    if (!has_started_) {
                        first_sample = size;
                        for (size_t channel = 0; channel < num_channels_; ++channel) {
                            for (size_t i = 0; i < first_sample; ++i) {
                                if (std::fpclassify(buffer_[channel][i]) != FP_ZERO) {
                                    first_sample = i;
                                    break;
                                }
                            }
                        }
                        if (first_sample == size) {
                            continue;
                        }
                        has_started_ = true;
                    }
                    std::array<float*, 2> pointers{
                        buffer_[0].data() + first_sample, buffer_[1].data() + first_sample};
                    meter_.process(std::span(pointers.data(), num_channels_), size - first_sample,
                                   on_update);
                }
            };
            measure(range.start_index1, range.block_size1);
            measure(range.start_index2, range.block_size2);
        }

        [[nodiscard]] const auto& getMeter() const {
            return meter_;
        }

    private:
        static constexpr size_t kChunkSize = 512;
        size_t num_channels_{0};
        bool has_started_{false};
        std::array<std::array<float, kChunkSize>, 2> buffer_{};
        zldsp::loudness::LUFSMeter<float> meter_{false};
    };
}
