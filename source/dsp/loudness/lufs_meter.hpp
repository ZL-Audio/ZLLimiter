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
#include <bit>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <deque>
#include <limits>
#include <span>
#include <vector>

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
        }

        void prepare(const double sample_rate, const size_t num_channels) {
            std::array<FloatType, 6> weights{FloatType(1), FloatType(1), FloatType(1),
                                             FloatType(1), FloatType(1), FloatType(1)};
            if (num_channels >= 4) {
                weights[num_channels - 2] = FloatType(1.41);
                weights[num_channels - 1] = FloatType(1.41);
            }
            if (num_channels == 6) {
                weights[3] = FloatType(0);
            }
            prepare(sample_rate, std::span<const FloatType>(weights.data(), num_channels));
        }

        void prepare(const double sample_rate, const std::span<const FloatType> weights) {
            const auto block_size = sample_rate / 10.0;

            k_weighting_filter_.prepare(sample_rate, weights.size());
            weights_.assign(weights.begin(), weights.end());
            base_block_size_ = static_cast<size_t>(block_size);
            block_size_remainder_ = sample_rate - static_cast<double>(base_block_size_) * 10.0;

            small_buffer_.resize(weights.size());
            small_buffer_ptrs_.resize(weights.size());
            for (size_t channel = 0; channel < weights.size(); ++channel) {
                small_buffer_[channel].resize(kScratchSize);
                small_buffer_ptrs_[channel] = small_buffer_[channel].data();
            }
            reset();
        }

        void reset() {
            k_weighting_filter_.reset();
            current_idx_ = 0;
            block_processed_ = 0;
            block_remainder_ = 0.0;
            nextBlock();
            block_sum_square_ = 0.0;
            ready_count_ = 0;
            sum_squares_.fill(0.0);
            block_sizes_.fill(0);
            short_term_energy_tree_.fill(0.0);
            short_term_block_sizes_.fill(0);
            short_term_num_samples_ = 0;
            short_term_write_idx_ = 0;
            short_term_ready_count_ = 0;
            short_term_mean_square_ = 0.0;
            short_term_dirty_ = false;
            short_term_loudness_ = -std::numeric_limits<FloatType>::infinity();
            loudness_range_history_.reset();
            loudness_range_warmup_blocks_ = 0;
            loudness_range_dirty_ = false;
            loudness_range_ = FloatType(0);
            integrated_history_.reset();
            integrated_dirty_ = false;
            integrated_loudness_ = -std::numeric_limits<FloatType>::infinity();
        }

        void process(std::span<FloatType*> buffer, const size_t num_samples) {
            assert(base_block_size_ > 0 && buffer.size() == small_buffer_.size());
            if (base_block_size_ == 0 || buffer.size() != small_buffer_.size()) {
                return;
            }
            size_t start_idx = 0;
            while (start_idx < num_samples) {
                const auto scratch_size = std::min(kScratchSize, block_size_ - block_processed_);
                const auto remaining_num = std::min(num_samples - start_idx, scratch_size - current_idx_);
                for (size_t channel = 0; channel < buffer.size(); ++channel) {
                    vector::copy(small_buffer_[channel].data() + current_idx_,
                                 buffer[channel] + start_idx, remaining_num);
                }
                start_idx += remaining_num;
                current_idx_ += remaining_num;
                if (current_idx_ == scratch_size) {
                    processScratch(scratch_size);
                    current_idx_ = 0;
                    block_processed_ += scratch_size;
                    if (block_processed_ == block_size_) {
                        update(block_sum_square_);
                        block_processed_ = 0;
                        block_sum_square_ = 0.0;
                        nextBlock();
                    }
                }
            }
        }

        [[nodiscard]] FloatType getShortTermLoudness() const noexcept {
            if (short_term_dirty_) {
                short_term_loudness_ = toLoudness(short_term_mean_square_);
                short_term_dirty_ = false;
            }
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
            return loudness_range_history_.getTotal().count > 0;
        }

        [[nodiscard]] bool isLoudnessRangeProvisional() const noexcept {
            return !isLoudnessRangeReady() || loudness_range_warmup_blocks_ < kLoudnessRangeWarmupBlocks;
        }

        [[nodiscard]] FloatType getIntegratedLoudness() const noexcept {
            if (integrated_dirty_) {
                const auto total = integrated_history_.getTotal();
                const auto gate = std::max(kAbsoluteGate, total.sum / static_cast<double>(total.count) * 0.1);
                const auto gated = integrated_history_.getGated(gate, false);
                integrated_loudness_ = toLoudness(gated.sum / static_cast<double>(gated.count));
                integrated_dirty_ = false;
            }
            return integrated_loudness_;
        }

        [[nodiscard]] bool isIntegratedReady() const noexcept {
            return integrated_history_.getTotal().count > 0;
        }

    private:
        class EnergyHistory {
        public:
            struct Statistics {
                double sum{0.0};
                uint64_t count{0};
            };

            void reset() noexcept {
                root_ = kEmpty;
                used_nodes_ = 0;
            }

            void insert(const double energy) {
                root_ = insert(root_, energy);
            }

            [[nodiscard]] Statistics getTotal() const noexcept {
                return getTotal(root_);
            }

            [[nodiscard]] Statistics getGated(const double gate, const bool inclusive) const noexcept {
                Statistics result;
                auto index = root_;
                while (index != kEmpty) {
                    const auto& node = nodes_[index];
                    if (inclusive ? node.energy >= gate : node.energy > gate) {
                        const auto right = getTotal(node.right);
                        result.sum += node.energy * static_cast<double>(node.count) + right.sum;
                        result.count += node.count + right.count;
                        index = node.left;
                    } else {
                        index = node.right;
                    }
                }
                return result;
            }

            [[nodiscard]] double getEnergyAtRank(uint64_t rank) const noexcept {
                assert(rank < getTotal().count);
                auto index = root_;
                while (index != kEmpty) {
                    const auto& node = nodes_[index];
                    const auto left_count = getTotal(node.left).count;
                    if (rank < left_count) {
                        index = node.left;
                    } else if (rank - left_count < node.count) {
                        return node.energy;
                    } else {
                        rank -= left_count + node.count;
                        index = node.right;
                    }
                }
                return 0.0;
            }

        private:
            static constexpr size_t kEmpty = std::numeric_limits<size_t>::max();

            struct Node {
                double energy{0.0}, sum{0.0};
                uint64_t count{1}, total_count{1};
                size_t left{kEmpty}, right{kEmpty};
                int height{1};
            };

            std::deque<Node> nodes_;
            size_t root_{kEmpty}, used_nodes_{0};

            [[nodiscard]] Statistics getTotal(const size_t index) const noexcept {
                return index == kEmpty ? Statistics{} : Statistics{nodes_[index].sum, nodes_[index].total_count};
            }

            [[nodiscard]] int getHeight(const size_t index) const noexcept {
                return index == kEmpty ? 0 : nodes_[index].height;
            }

            void updateNode(const size_t index) noexcept {
                auto& node = nodes_[index];
                const auto left = getTotal(node.left);
                const auto right = getTotal(node.right);
                node.sum = left.sum + right.sum + node.energy * static_cast<double>(node.count);
                node.total_count = left.count + right.count + node.count;
                node.height = 1 + std::max(getHeight(node.left), getHeight(node.right));
            }

            size_t rotateLeft(const size_t index) noexcept {
                const auto right = nodes_[index].right;
                nodes_[index].right = nodes_[right].left;
                nodes_[right].left = index;
                updateNode(index);
                updateNode(right);
                return right;
            }

            size_t rotateRight(const size_t index) noexcept {
                const auto left = nodes_[index].left;
                nodes_[index].left = nodes_[left].right;
                nodes_[left].right = index;
                updateNode(index);
                updateNode(left);
                return left;
            }

            size_t insert(const size_t index, const double energy) {
                if (index == kEmpty) {
                    if (used_nodes_ == nodes_.size()) {
                        nodes_.emplace_back();
                    }
                    nodes_[used_nodes_] = Node{.energy = energy, .sum = energy};
                    return used_nodes_++;
                }
                auto& node = nodes_[index];
                if (energy < node.energy) {
                    node.left = insert(node.left, energy);
                } else if (energy > node.energy) {
                    node.right = insert(node.right, energy);
                } else {
                    node.count += 1;
                }
                updateNode(index);
                const auto balance = getHeight(node.left) - getHeight(node.right);
                if (balance > 1) {
                    if (energy > nodes_[node.left].energy) {
                        node.left = rotateLeft(node.left);
                    }
                    return rotateRight(index);
                }
                if (balance < -1) {
                    if (energy < nodes_[node.right].energy) {
                        node.right = rotateRight(node.right);
                    }
                    return rotateLeft(index);
                }
                return index;
            }
        };

        static constexpr double kAbsoluteGate = 1.1724653045822963e-7;
        static constexpr size_t kScratchSize = 512;
        KWeightingFilter<FloatType> k_weighting_filter_;
        std::vector<std::vector<FloatType>> small_buffer_;
        std::vector<FloatType*> small_buffer_ptrs_;
        std::vector<FloatType> weights_;
        size_t current_idx_{0}, base_block_size_{0}, block_size_{0}, block_processed_{0};
        double block_size_remainder_{0.0}, block_remainder_{0.0}, block_sum_square_{0.0};
        size_t ready_count_{0};
        std::array<double, 4> sum_squares_{};
        std::array<size_t, 4> block_sizes_{};

        static constexpr size_t kShortTermBlockCount = 30;
        static constexpr size_t kShortTermTreeLeaves = std::bit_ceil(kShortTermBlockCount);
        std::array<double, kShortTermTreeLeaves * 2> short_term_energy_tree_{};
        std::array<size_t, kShortTermBlockCount> short_term_block_sizes_{};
        size_t short_term_num_samples_{0};
        size_t short_term_write_idx_{0}, short_term_ready_count_{0};
        double short_term_mean_square_{0.0};
        mutable bool short_term_dirty_{false};
        mutable FloatType short_term_loudness_{-std::numeric_limits<FloatType>::infinity()};

        static constexpr size_t kLoudnessRangeWarmupBlocks = 600;
        EnergyHistory loudness_range_history_;
        size_t loudness_range_warmup_blocks_{0};
        mutable bool loudness_range_dirty_{false};
        mutable FloatType loudness_range_{FloatType(0)};

        EnergyHistory integrated_history_;
        mutable bool integrated_dirty_{false};
        mutable FloatType integrated_loudness_{-std::numeric_limits<FloatType>::infinity()};

        static FloatType toLoudness(const double mean_square) noexcept {
            return mean_square > 0.0
                ? static_cast<FloatType>(-0.691 + 10.0 * std::log10(mean_square))
                : -std::numeric_limits<FloatType>::infinity();
        }

        static uint64_t percentileRank(const uint64_t count, const uint64_t numerator,
                                       const uint64_t denominator) noexcept {
            const auto last_rank = count - 1;
            return (last_rank / denominator) * numerator
                + ((last_rank % denominator) * numerator + denominator / 2) / denominator;
        }

        FloatType calculateLoudnessRange() const noexcept {
            const auto total = loudness_range_history_.getTotal();
            const auto gate = std::max(kAbsoluteGate, total.sum / static_cast<double>(total.count) * 0.01);
            const auto gated = loudness_range_history_.getGated(gate, true);
            const auto first_rank = total.count - gated.count;
            const auto low = loudness_range_history_.getEnergyAtRank(
                first_rank + percentileRank(gated.count, 1, 10));
            const auto high = loudness_range_history_.getEnergyAtRank(
                first_rank + percentileRank(gated.count, 19, 20));
            return static_cast<FloatType>(10.0 * std::log10(high / low));
        }

        void updateShortTerm(const double sum_square) {
            loudness_range_warmup_blocks_ = std::min(loudness_range_warmup_blocks_ + 1, kLoudnessRangeWarmupBlocks);
            auto index = kShortTermTreeLeaves + short_term_write_idx_;
            short_term_energy_tree_[index] = sum_square;
            while (index > 1) {
                index /= 2;
                short_term_energy_tree_[index] = short_term_energy_tree_[index * 2]
                    + short_term_energy_tree_[index * 2 + 1];
            }
            short_term_num_samples_ -= short_term_block_sizes_[short_term_write_idx_];
            short_term_block_sizes_[short_term_write_idx_] = block_size_;
            short_term_num_samples_ += block_size_;
            short_term_write_idx_ = (short_term_write_idx_ + 1) % kShortTermBlockCount;
            short_term_ready_count_ = std::min(short_term_ready_count_ + 1, kShortTermBlockCount);
            if (isShortTermReady()) {
                short_term_mean_square_ = short_term_energy_tree_[1] / static_cast<double>(short_term_num_samples_);
                short_term_dirty_ = true;
                if (short_term_mean_square_ >= kAbsoluteGate && std::isfinite(short_term_mean_square_)) {
                    loudness_range_history_.insert(short_term_mean_square_);
                    loudness_range_dirty_ = true;
                }
            }
        }

        void nextBlock() noexcept {
            block_remainder_ += block_size_remainder_;
            const auto extra_sample = block_remainder_ >= 10.0;
            block_size_ = base_block_size_ + static_cast<size_t>(extra_sample);
            block_remainder_ -= extra_sample ? 10.0 : 0.0;
        }

        void processScratch(const size_t num_samples) {
            k_weighting_filter_.process(std::span(small_buffer_ptrs_), num_samples);
            for (size_t channel = 0; channel < small_buffer_.size(); ++channel) {
                if (std::fpclassify(weights_[channel]) == FP_ZERO) {
                    continue;
                }
                const auto channel_sum_square = vector::sum_sqr(small_buffer_[channel].data(),
                                                                num_samples);
                block_sum_square_ += static_cast<double>(channel_sum_square) * static_cast<double>(weights_[channel]);
            }
        }

        void update(const double sum_square) {
            updateShortTerm(sum_square);
            sum_squares_[0] = sum_squares_[1];
            sum_squares_[1] = sum_squares_[2];
            sum_squares_[2] = sum_squares_[3];
            sum_squares_[3] = sum_square;
            block_sizes_[0] = block_sizes_[1];
            block_sizes_[1] = block_sizes_[2];
            block_sizes_[2] = block_sizes_[3];
            block_sizes_[3] = block_size_;
            if (ready_count_ < 3) {
                ready_count_ += 1;
                return;
            }
            const auto mean_square = (sum_squares_[0] + sum_squares_[1] + sum_squares_[2] + sum_squares_[3])
                / static_cast<double>(block_sizes_[0] + block_sizes_[1]
                    + block_sizes_[2] + block_sizes_[3]);
            if (mean_square > kAbsoluteGate && std::isfinite(mean_square)) {
                integrated_history_.insert(mean_square);
                integrated_dirty_ = true;
            }
        }
    };
}
