// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace zlp {
    inline constexpr int kVersionHint = 1;

    class PInputGain;
    class POutputCeiling;
    class PBypass;
    class PDelta;
    class PTruePeak;
    class POversampling;
    class PLookahead;
    class PAttack;
    class PRelease;
    class PChannelDelta;
    class PRecovery;

    template <typename FloatType>
    inline juce::NormalisableRange<FloatType> getLogMidRange(
        const FloatType x_min, const FloatType x_max, const FloatType x_mid, const FloatType x_interval) {
        const FloatType rng1{std::log(x_mid / x_min) * FloatType(2)};
        const FloatType rng2{std::log(x_max / x_mid) * FloatType(2)};
        auto result_range = juce::NormalisableRange<FloatType>{
            x_min, x_max,
            [=](FloatType, FloatType, const FloatType v) {
                return v < FloatType(.5) ? std::exp(v * rng1) * x_min : std::exp((v - FloatType(.5)) * rng2) * x_mid;
            },
            [=](FloatType, FloatType, const FloatType v) {
                return v < x_mid ? std::log(v / x_min) / rng1 : FloatType(.5) + std::log(v / x_mid) / rng2;
            },
            [=](FloatType, FloatType, const FloatType v) {
                const FloatType x = x_min + x_interval * std::round((v - x_min) / x_interval);
                return x <= x_min ? x_min : (x >= x_max ? x_max : x);
            }
        };
        result_range.interval = x_interval;
        return result_range;
    }

    template <typename FloatType>
    inline juce::NormalisableRange<FloatType> getLogMidRangeShift(
        const FloatType x_min, const FloatType x_max, const FloatType x_mid,
        const FloatType x_interval, const FloatType shift) {
        const auto range = getLogMidRange<FloatType>(x_min, x_max, x_mid, x_interval);
        auto result_range = juce::NormalisableRange<FloatType>{
            x_min + shift, x_max + shift,
            [=](FloatType, FloatType, const FloatType v) {
                return range.convertFrom0to1(v) + shift;
            },
            [=](FloatType, FloatType, const FloatType v) {
                return range.convertTo0to1(v - shift);
            },
            [=](FloatType, FloatType, const FloatType v) {
                return range.snapToLegalValue(v - shift) + shift;
            }
        };
        result_range.interval = x_interval;
        return result_range;
    }

    template <typename FloatType>
    inline juce::NormalisableRange<FloatType> getSymmetricLogMidRangeShift(
        const FloatType x_min, const FloatType x_max, const FloatType x_mid,
        const FloatType x_interval, const FloatType shift) {
        const auto range = getLogMidRangeShift<FloatType>(x_min, x_max, x_mid, x_interval, shift);
        auto result_range = juce::NormalisableRange<FloatType>{
            -(x_max + shift), x_max + shift,
            [=](FloatType, FloatType, const FloatType v) {
                if (v > FloatType(0.5)) {
                    return range.convertFrom0to1(v * FloatType(2) - FloatType(1));
                } else {
                    return -range.convertFrom0to1(FloatType(1) - v * FloatType(2));
                }
            },
            [=](FloatType, FloatType, const FloatType v) {
                if (v > FloatType(0)) {
                    return range.convertTo0to1(v) * FloatType(0.5) + FloatType(0.5);
                } else {
                    return FloatType(0.5) - range.convertTo0to1(-v) * FloatType(0.5);
                }
            },
            [=](FloatType, FloatType, const FloatType v) {
                if (v > FloatType(0)) {
                    return range.snapToLegalValue(v);
                } else {
                    return -range.snapToLegalValue(-v);
                }
            }
        };
        result_range.interval = x_interval;
        return result_range;
    }

    template <typename FloatType>
    inline juce::NormalisableRange<FloatType> getLinearMidRange(
        const FloatType x_min, const FloatType x_max, const FloatType x_mid, const FloatType x_interval) {
        auto result_range = juce::NormalisableRange<FloatType>{
            x_min, x_max,
            [=](FloatType, FloatType, const FloatType v) {
                return v < FloatType(.5)
                    ? FloatType(2) * v * (x_mid - x_min) + x_min
                    : FloatType(2) * (v - FloatType(0.5)) * (x_max - x_mid) + x_mid;
            },
            [=](FloatType, FloatType, const FloatType v) {
                return v < x_mid
                    ? FloatType(.5) * (v - x_min) / (x_mid - x_min)
                    : FloatType(.5) + FloatType(.5) * (v - x_mid) / (x_max - x_mid);
            },
            [=](FloatType, FloatType, const FloatType v) {
                const FloatType x = x_min + x_interval * std::round((v - x_min) / x_interval);
                return x <= x_min ? x_min : (x >= x_max ? x_max : x);
            }
        };
        result_range.interval = x_interval;
        return result_range;
    }

    // float
    template <class T>
    class FloatParameters {
    public:
        static std::unique_ptr<juce::AudioParameterFloat> get(const bool automate = true) {
            auto attributes = juce::AudioParameterFloatAttributes().withAutomatable(automate).withLabel(T::kName);
            return std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(T::kID, kVersionHint),
                                                               T::kName, T::kRange, T::kDefaultV, attributes);
        }

        static std::unique_ptr<juce::AudioParameterFloat> get(const std::string& suffix) {
            auto attributes = juce::AudioParameterFloatAttributes().withAutomatable(true).withLabel(T::kName);
            return std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(T::kID + suffix, kVersionHint),
                                                               T::kName + suffix, T::kRange, T::kDefaultV, attributes);
        }

        static std::unique_ptr<juce::AudioParameterFloat> get(const std::string& suffix, const bool meta,
                                                              const bool automate) {
            auto attributes = juce::AudioParameterFloatAttributes(
                ).withAutomatable(automate).withLabel(T::kName).withMeta(meta);
            return std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(T::kID + suffix, kVersionHint),
                                                               T::kName + suffix, T::kRange, T::kDefaultV, attributes);
        }

        inline static float convertTo01(const float x) {
            return T::kRange.convertTo0to1(x);
        }
    };

    // bool
    template <class T>
    class BoolParameters {
    public:
        static std::unique_ptr<juce::AudioParameterBool> get(const bool automate = true) {
            auto attributes = juce::AudioParameterBoolAttributes().withAutomatable(automate).withLabel(T::kName);
            return std::make_unique<juce::AudioParameterBool>(juce::ParameterID(T::kID, kVersionHint),
                                                              T::kName, T::kDefaultV, attributes);
        }

        static std::unique_ptr<juce::AudioParameterBool> get(const std::string& suffix) {
            auto attributes = juce::AudioParameterBoolAttributes().withAutomatable(true).withLabel(T::kName);
            return std::make_unique<juce::AudioParameterBool>(juce::ParameterID(T::kID + suffix, kVersionHint),
                                                              T::kName + suffix, T::kDefaultV, attributes);
        }

        static std::unique_ptr<juce::AudioParameterBool> get(const std::string& suffix, const bool meta,
                                                             const bool automate) {
            auto attributes = juce::AudioParameterBoolAttributes(
                ).withAutomatable(automate).withLabel(T::kName).withMeta(meta);
            return std::make_unique<juce::AudioParameterBool>(juce::ParameterID(T::kID + suffix, kVersionHint),
                                                              T::kName + suffix, T::kDefaultV, attributes);
        }

        inline static float convertTo01(const bool x) {
            return x ? 1.f : 0.f;
        }
    };

    // choice
    template <class T>
    class ChoiceParameters {
    public:
        static std::unique_ptr<juce::AudioParameterChoice> get(const bool automate = true) {
            auto attributes = juce::AudioParameterChoiceAttributes().withAutomatable(automate).withLabel(T::kName);
            return std::make_unique<juce::AudioParameterChoice>(juce::ParameterID(T::kID, kVersionHint),
                                                                T::kName, T::kChoices, T::kDefaultI, attributes);
        }

        static std::unique_ptr<juce::AudioParameterChoice> get(const std::string& suffix) {
            auto attributes = juce::AudioParameterChoiceAttributes().withAutomatable(true).withLabel(T::kName);
            return std::make_unique<juce::AudioParameterChoice>(juce::ParameterID(T::kID + suffix, kVersionHint),
                                                                T::kName + suffix, T::kChoices, T::kDefaultI,
                                                                attributes);
        }

        static std::unique_ptr<juce::AudioParameterChoice> get(const std::string& suffix, const bool meta,
                                                               const bool automate) {
            auto attributes = juce::AudioParameterChoiceAttributes(
                ).withAutomatable(automate).withLabel(T::kName).withMeta(meta);
            return std::make_unique<juce::AudioParameterChoice>(juce::ParameterID(T::kID + suffix, kVersionHint),
                                                                T::kName + suffix, T::kChoices, T::kDefaultI,
                                                                attributes);
        }

        inline static float convertTo01(const int x) {
            return static_cast<float>(x) / static_cast<float>(T::kChoices.size() - 1);
        }
    };

    class PInputGain : public FloatParameters<PInputGain> {
    public:
        static constexpr auto kID = "input_gain";
        static constexpr auto kName = "Input Gain";
        inline static const auto kRange = juce::NormalisableRange<float>(0.f, 24.f, 0.01f);
        static constexpr auto kDefaultV = 0.f;
    };

    class POutputCeiling : public FloatParameters<POutputCeiling> {
    public:
        static constexpr auto kID = "output_ceiling";
        static constexpr auto kName = "Output Ceiling";
        inline static const auto kRange = juce::NormalisableRange<float>(-12.f, 0.f, 0.01f);
        static constexpr auto kDefaultV = -0.01f;
    };

    class PBypass : public BoolParameters<PBypass> {
    public:
        static constexpr auto kID = "bypass";
        static constexpr auto kName = "Bypass";
        static constexpr auto kDefaultV = false;
    };

    class PDelta : public BoolParameters<PDelta> {
    public:
        static constexpr auto kID = "delta";
        static constexpr auto kName = "Delta";
        static constexpr auto kDefaultV = false;
    };

    class PTruePeak : public BoolParameters<PTruePeak> {
    public:
        static constexpr auto kID = "true_peak";
        static constexpr auto kName = "True Peak";
        static constexpr auto kDefaultV = true;
    };

    class POversampling : public ChoiceParameters<POversampling> {
    public:
        static constexpr auto kID = "oversampling";
        static constexpr auto kName = "Oversampling";
        inline static const auto kChoices = juce::StringArray{"Off", "2x", "4x", "8x", "16x", "32x"};
        static constexpr auto kDefaultI = 2;
    };

    class PLookahead : public FloatParameters<PLookahead> {
    public:
        static constexpr auto kID = "lookahead";
        static constexpr auto kName = "Lookahead";
        inline static const auto kRange = juce::NormalisableRange<float>(0.f, 5.f, 0.01f);
        static constexpr auto kDefaultV = 2.f;
    };

    class PAttack : public FloatParameters<PAttack> {
    public:
        static constexpr auto kID = "attack";
        static constexpr auto kName = "Attack";
        inline static const auto kRange = getLogMidRange(0.01f, 10000.f, 100.f, 0.01f);
        static constexpr auto kDefaultV = 100.f;
    };

    class PRelease : public FloatParameters<PRelease> {
    public:
        static constexpr auto kID = "release";
        static constexpr auto kName = "Release";
        inline static const auto kRange = getLogMidRange(1.f, 10000.f, 500.f, 0.01f);
        static constexpr auto kDefaultV = 500.f;
    };

    class PChannelDelta : public FloatParameters<PChannelDelta> {
    public:
        static constexpr auto kID = "channel_delta";
        static constexpr auto kName = "Channel Delta";
        inline static const auto kRange = juce::NormalisableRange<float>(0.f, 6.f, 0.01f);
        static constexpr auto kDefaultV = 1.5f;
    };

    class PRecovery : public FloatParameters<PRecovery> {
    public:
        static constexpr auto kID = "recovery";
        static constexpr auto kName = "Recovery";
        inline static const auto kRange = juce::NormalisableRange<float>(0.f, 100.f, 0.01f);
        static constexpr auto kDefaultV = 50.f;
    };

    inline juce::AudioProcessorValueTreeState::ParameterLayout getParameterLayout() {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;
        layout.add(PInputGain::get(), POutputCeiling::get(),
                   PTruePeak::get(), POversampling::get(),
                   PLookahead::get(), PRecovery::get(), PAttack::get(), PRelease::get(),
                   PChannelDelta::get(),
                   PBypass::get(), PDelta::get());
        return layout;
    }

    inline void updateParaNotifyHost(juce::RangedAudioParameter* para, const float value) {
        para->beginChangeGesture();
        para->setValueNotifyingHost(value);
        para->endChangeGesture();
    }
}
