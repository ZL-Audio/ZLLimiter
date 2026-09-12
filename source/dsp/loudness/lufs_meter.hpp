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
#include <bit>
#include <cmath>
#include <limits>
#include <span>

#include "../vector/vector.hpp"
#include "k_weighting_filter.hpp"

namespace zldsp::loudness {
    template <typename FloatType>
    class LUFSMeter {
    public:
        /**
         *
         * @param use_low_pass whether to use an extra lowpass filter at 22,000 Hz
         */
        explicit LUFSMeter(const bool use_low_pass = true) :
            k_weighting_filter_(use_low_pass) {
            histogram_.resize(701);
            histogram_sums_.resize(701);
            loudness_range_histogram_.resize(kLoudnessRangeBinCount);
            loudness_range_histogram_sums_.resize(kLoudnessRangeBinCount);
        }

        void prepare(const double sample_rate, const size_t num_channels) {
            k_weighting_filter_.prepare(sample_rate, num_channels);
            weights_.resize(num_channels);
            for (size_t i = 0; i < num_channels; ++i) {
                weights_[i] = (i == 4 || i == 5) ? FloatType(1.41) : FloatType(1);
            }

            max_idx_ = static_cast<int>(sample_rate * 0.1);
            mean_mul_ = static_cast<FloatType>(2.5 / sample_rate);
            short_term_mean_mul_ = 1.0 / (static_cast<double>(max_idx_) * kShortTermBlockCount);

            small_buffer_.resize(num_channels);
            small_buffer_ptrs_.resize(num_channels);
            for (size_t channel = 0; channel < num_channels; ++channel) {
                small_buffer_[channel].resize(static_cast<size_t>(max_idx_));
                small_buffer_ptrs_[channel] = small_buffer_[channel].data();
            }
            reset();
        }

        void reset() {
            k_weighting_filter_.reset();
            current_idx_ = 0;
            ready_count_ = 0;
            short_term_energy_tree_.fill(0.0);
            short_term_write_idx_ = 0;
            short_term_ready_count_ = 0;
            short_term_loudness_ = -std::numeric_limits<FloatType>::infinity();
            std::fill(loudness_range_histogram_.begin(), loudness_range_histogram_.end(), FloatType(0));
            std::fill(loudness_range_histogram_sums_.begin(), loudness_range_histogram_sums_.end(), FloatType(0));
            loudness_range_first_bin_ = kLoudnessRangeBinCount;
            loudness_range_last_bin_ = 0;
            loudness_range_warmup_blocks_ = 0;
            loudness_range_dirty_ = false;
            loudness_range_ = FloatType(0);
            std::fill(histogram_.begin(), histogram_.end(), FloatType(0));
            std::fill(histogram_sums_.begin(), histogram_sums_.end(), FloatType(0));
            for (auto& buffer : small_buffer_) {
                std::fill(buffer.begin(), buffer.end(), FloatType(0));
            }
        }

        void process(std::span<FloatType*> buffer, const size_t num_samples) {
            const auto num_total = static_cast<int>(num_samples);
            int start_idx = 0;
            while (num_total - start_idx >= max_idx_ - current_idx_) {
                // now we get a full 100 ms small block
                const auto remaining_num = max_idx_ - current_idx_;
                for (size_t channel = 0; channel < buffer.size(); ++channel) {
                    vector::copy(small_buffer_[channel].data() + static_cast<size_t>(current_idx_),
                                 buffer[channel] + static_cast<size_t>(start_idx),
                                 static_cast<size_t>(remaining_num));
                }
                start_idx += remaining_num;
                current_idx_ = 0;
                update();
            }
            if (num_total - start_idx > 0) {
                const auto remaining_num = num_total - start_idx;
                for (size_t channel = 0; channel < buffer.size(); ++channel) {
                    vector::copy(small_buffer_[channel].data() + static_cast<size_t>(current_idx_),
                                 buffer[channel] + static_cast<size_t>(start_idx),
                                 static_cast<size_t>(remaining_num));
                }
                current_idx_ += remaining_num;
            }
        }

        [[nodiscard]] FloatType getShortTermLoudness() const noexcept {
            return short_term_loudness_;
        }

        [[nodiscard]] bool isShortTermReady() const noexcept {
            return short_term_ready_count_ == kShortTermBlockCount;
        }

        [[nodiscard]] FloatType getLoudnessRange() const noexcept {
            if (loudness_range_dirty_) {
                loudness_range_ = calculateLoudnessRange();
                loudness_range_dirty_ = false;
            }
            return loudness_range_;
        }

        [[nodiscard]] bool isLoudnessRangeReady() const noexcept {
            return loudness_range_first_bin_ < kLoudnessRangeBinCount;
        }

        [[nodiscard]] bool isLoudnessRangeProvisional() const noexcept {
            return !isLoudnessRangeReady() || loudness_range_warmup_blocks_ < kLoudnessRangeWarmupBlocks;
        }

        FloatType getIntegratedLoudness() const {
            const auto total_count = vector::sum(histogram_.data(), histogram_.size());
            if (total_count < FloatType(0.5)) {
                return FloatType(0);
            }
            const auto total_sum = vector::sum(histogram_sums_.data(), histogram_sums_.size());
            const auto total_mean_square = total_sum / total_count;
            const auto total_lufs = FloatType(-0.691) + FloatType(10) * std::log10(total_mean_square);
            if (total_lufs <= FloatType(-60) || total_lufs >= FloatType(9)) {
                return total_lufs;
            } else {
                const auto end_idx = static_cast<size_t>(std::round(-(total_lufs - FloatType(10)) * FloatType(10)));
                const auto sub_count = vector::sum(histogram_.data(), end_idx);
                const auto sub_sum = vector::sum(histogram_sums_.data(), end_idx);
                if (sub_count <= FloatType(0) || sub_sum <= FloatType(0)) {
                    return total_lufs;
                } else {
                    const auto sub_mean_square = sub_sum / sub_count;
                    const auto sub_lufs = FloatType(-0.691) + FloatType(10) * std::log10(sub_mean_square);
                    return sub_lufs;
                }
            }
        }

    private:
        KWeightingFilter<FloatType> k_weighting_filter_;
        std::vector<std::vector<FloatType>> small_buffer_;
        std::vector<FloatType*> small_buffer_ptrs_;
        int current_idx_{0}, max_idx_{0};
        int ready_count_{0};
        FloatType mean_mul_{1};
        std::array<FloatType, 4> sum_squares_{};

        static constexpr size_t kShortTermBlockCount = 30;
        static constexpr size_t kShortTermTreeLeaves = std::bit_ceil(kShortTermBlockCount);
        std::array<double, kShortTermTreeLeaves * 2> short_term_energy_tree_{};
        size_t short_term_write_idx_{0}, short_term_ready_count_{0};
        double short_term_mean_mul_{1};
        FloatType short_term_loudness_{-std::numeric_limits<FloatType>::infinity()};

        static constexpr FloatType kLoudnessRangeMaxLUFS = FloatType(10) * FloatType(
            std::numeric_limits<FloatType>::max_exponent10 + 1);
        static constexpr size_t kLoudnessRangeBinCount = static_cast<size_t>(
            (kLoudnessRangeMaxLUFS + FloatType(70)) * FloatType(10)) + 1;
        static constexpr size_t kLoudnessRangeWarmupBlocks = 600;
        vector::aligned_vector<FloatType> loudness_range_histogram_{};
        vector::aligned_vector<FloatType> loudness_range_histogram_sums_{};
        size_t loudness_range_first_bin_{kLoudnessRangeBinCount}, loudness_range_last_bin_{0};
        size_t loudness_range_warmup_blocks_{0};
        mutable bool loudness_range_dirty_{false};
        mutable FloatType loudness_range_{FloatType(0)};

        vector::aligned_vector<FloatType> histogram_{};
        vector::aligned_vector<FloatType> histogram_sums_{};
        std::vector<FloatType> weights_;

        void updateLoudnessRange(const FloatType mean_square, const FloatType loudness) {
            if (mean_square >= FloatType(1.1724653045822963e-7) && std::isfinite(mean_square)) {
                const auto hist_idx = static_cast<size_t>(std::clamp(
                    std::round((loudness + FloatType(70)) * FloatType(10)),
                    FloatType(0), static_cast<FloatType>(kLoudnessRangeBinCount - 1)));
                loudness_range_histogram_[hist_idx] += FloatType(1);
                loudness_range_histogram_sums_[hist_idx] += mean_square;
                loudness_range_first_bin_ = std::min(loudness_range_first_bin_, hist_idx);
                loudness_range_last_bin_ = std::max(loudness_range_last_bin_, hist_idx);
                loudness_range_dirty_ = true;
            }
        }

        FloatType calculateLoudnessRange() const noexcept {
            if (!isLoudnessRangeReady()) {
                return FloatType(0);
            }
            const auto histogram_size = loudness_range_last_bin_ - loudness_range_first_bin_ + 1;
            const auto total_count = vector::sum(
                loudness_range_histogram_.data() + loudness_range_first_bin_, histogram_size);
            if (total_count < FloatType(0.5)) {
                return FloatType(0);
            }
            const auto total_sum = vector::sum(
                loudness_range_histogram_sums_.data() + loudness_range_first_bin_, histogram_size);
            const auto total_mean_square = total_sum / total_count;
            if (!std::isfinite(total_mean_square)) {
                return FloatType(0);
            }
            const auto total_lufs = FloatType(-0.691) + FloatType(10) * std::log10(total_mean_square);
            const auto gate = std::max(total_lufs - FloatType(20), FloatType(-70));
            const auto start_idx = std::max(loudness_range_first_bin_, static_cast<size_t>(
                std::ceil((gate + FloatType(70)) * FloatType(10))));
            if (start_idx > loudness_range_last_bin_) {
                return FloatType(0);
            }
            const auto sub_count = vector::sum(
                loudness_range_histogram_.data() + start_idx, loudness_range_last_bin_ - start_idx + 1);
            if (sub_count < FloatType(0.5)) {
                return FloatType(0);
            }
            const auto low_rank = std::round((sub_count - FloatType(1)) * FloatType(0.10));
            const auto high_rank = std::round((sub_count - FloatType(1)) * FloatType(0.95));
            auto low_idx = start_idx;
            FloatType count = 0;
            for (size_t i = start_idx; i <= loudness_range_last_bin_; ++i) {
                const auto next_count = count + loudness_range_histogram_[i];
                if (count <= low_rank && low_rank < next_count) {
                    low_idx = i;
                }
                if (high_rank < next_count) {
                    return static_cast<FloatType>(i - low_idx) / FloatType(10);
                }
                count = next_count;
            }
            return FloatType(0);
        }

        void updateShortTerm(const FloatType sum_square) {
            loudness_range_warmup_blocks_ = std::min(loudness_range_warmup_blocks_ + 1, kLoudnessRangeWarmupBlocks);
            auto index = kShortTermTreeLeaves + short_term_write_idx_;
            short_term_energy_tree_[index] = static_cast<double>(sum_square);
            while (index > 1) {
                index /= 2;
                short_term_energy_tree_[index] = short_term_energy_tree_[index * 2]
                    + short_term_energy_tree_[index * 2 + 1];
            }
            short_term_write_idx_ = (short_term_write_idx_ + 1) % kShortTermBlockCount;
            short_term_ready_count_ = std::min(short_term_ready_count_ + 1, kShortTermBlockCount);
            if (isShortTermReady()) {
                const auto mean_square = short_term_energy_tree_[1] * short_term_mean_mul_;
                const auto loudness = mean_square > 0.0
                    ? -0.691 + 10.0 * std::log10(mean_square)
                    : -std::numeric_limits<double>::infinity();
                short_term_loudness_ = static_cast<FloatType>(loudness);
                updateLoudnessRange(static_cast<FloatType>(mean_square), short_term_loudness_);
            }
        }

        void update() {
            // perform K-weighting filtering
            k_weighting_filter_.process(std::span(small_buffer_ptrs_), static_cast<size_t>(max_idx_));
            // calculate the sum square of the small block
            FloatType sum_square = 0;
            for (size_t channel = 0; channel < small_buffer_.size(); ++channel) {
                const auto channel_sum_square = vector::sum_sqr(small_buffer_[channel].data(),
                                                                small_buffer_[channel].size());
                sum_square += channel_sum_square * weights_[channel];
            }
            updateShortTerm(sum_square);
            // shift circular sumSquares
            sum_squares_[0] = sum_squares_[1];
            sum_squares_[1] = sum_squares_[2];
            sum_squares_[2] = sum_squares_[3];
            sum_squares_[3] = sum_square;
            if (ready_count_ < 3) {
                ready_count_ += 1;
                return;
            }
            // calculate the mean square
            const auto mean_square = (
                sum_squares_[0] + sum_squares_[1] + sum_squares_[2] + sum_squares_[3]) * mean_mul_;
            // update histogram
            if (mean_square >= FloatType(1.1724653045822963e-7)) {
                // if greater than -70 LKFS
                const auto lkfs = std::min(-FloatType(0.691) + FloatType(10) * std::log10(mean_square), FloatType(0));
                const auto hist_idx = static_cast<size_t>(std::round(-lkfs * FloatType(10)));
                histogram_[hist_idx] += FloatType(1);
                histogram_sums_[hist_idx] += mean_square;
            }
        }
    };
}
