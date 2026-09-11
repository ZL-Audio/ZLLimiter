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
#include "../../limiter/true_peak/true_peak_estimator.hpp"

namespace zldsp::analyzer {
    class MagnitudeReceiver {
    public:
        MagnitudeReceiver() {
            estimator_.prepare(2, kChunkSize);
        }

        void reset() {
            estimator_.reset();
            dbs_.fill(-240.f);
        }

        void run(const zldsp::container::FIFORange range,
                 const std::vector<std::vector<float>>& samples, const bool true_peak) {
            dbs_.fill(-240.f);
            for (size_t channel = 0; channel < 2; ++channel) {
                float peak = 0.f;
                const auto measure = [&](const int start, const int count) {
                    for (size_t offset = 0; offset < static_cast<size_t>(count); offset += kChunkSize) {
                        const auto size = std::min(kChunkSize, static_cast<size_t>(count) - offset);
                        const auto* input = samples[channel].data() + static_cast<size_t>(start) + offset;
                        for (size_t i = 0; i < size; ++i) {
                            input_[i] = std::isfinite(input[i]) ? input[i] : 0.f;
                        }
                        if (true_peak) {
                            estimator_.processBlock(channel, input_.data(), output_.data(), size);
                            peak = std::max(peak, zldsp::vector::max_abs_of(output_.data(), size));
                        } else {
                            estimator_.updateHistoryOnly(channel, input_.data(), size);
                            peak = std::max(peak, zldsp::vector::max_abs_of(input_.data(), size));
                        }
                    }
                };
                measure(range.start_index1, range.block_size1);
                measure(range.start_index2, range.block_size2);
                dbs_[channel] = 20.f * std::log10(std::max(peak, 1e-12f));
            }
        }

        [[nodiscard]] const auto& getDBs() const {
            return dbs_;
        }

        [[nodiscard]] float getMaxDB() const {
            return std::max(dbs_[0], dbs_[1]);
        }

    private:
        static constexpr size_t kChunkSize = 4096;
        zldsp::limiter::TruePeakEstimator<float> estimator_;
        std::array<float, kChunkSize> input_{}, output_{};
        std::array<float, 2> dbs_{-240.f, -240.f};
    };
}
