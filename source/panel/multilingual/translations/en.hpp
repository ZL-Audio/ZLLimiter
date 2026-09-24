// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

// This file is also dual licensed under the Apache License, Version 2.0. You may obtain a copy of the License at <http://www.apache.org/licenses/LICENSE-2.0>

#pragma once
#include <array>

namespace zlpanel::multilingual::en {
    inline constexpr std::array kTexts = {
        "Toggles the UI settings panel.",
        // preset related
        "Toggles the preset browser.",
        "Opens the presets folder in the system file manager.",
        "Deletes the selected preset group.",
        "Deletes the selected preset.",
        "Closes the preset browser.",
        // analyzer related
        "Toggles the analyzer settings panel.",
        "Selects peak detection mode for the analyzer.",
        "Selects the waveform scrolling mode.",
        "Toggles display of the input magnitude curve.",
        "Toggles display of the output magnitude curve.",
        "Toggles display of the limiter gain reduction curve.",
        "Selects the time duration displayed in the analyzer window.",
        "Selects the minimum decibel floor for the analyzer display.",
        "Toggles visibility of the level meter panel.",
        "Toggles visibility of the loudness and numeric readout panel.",
        // top controls
        "Adjusts the maximum output ceiling level.",
        "Selects the oversampling rate to reduce aliasing and inter-sample peaks.",
        "Toggles True Peak limiting to prevent inter-sample clipping.",
        "Auditions the delta signal (the difference between input and output, isolating what is being limited).",
        "Toggles plugin bypass.",
        // input gain controls
        "Selects gain linking mode.\nInput: adjusts input gain only.\nLinked: also attenuates output to match input gain.",
        "Adjusts the input gain driving into the limiter.",
        // bottom controls
        "Adjusts the lookahead time for the main limiter envelope to anticipate transients.",
        "Adjusts the adaptive recovery speed for the main envelope.",
        "Adjusts the attack time of the sustained support envelope.",
        "Adjusts the release time of the sustained support envelope.",
        "Adjusts the maximum allowed gain reduction difference between channels.",
        // values
        "Toggles the value readout settings panel.",
        "Displays the maximum held True Peak level.",
        "Displays current (left) and average (right) stereo phase correlation.",
        "Displays current (left) and maximum (right) momentary loudness.",
        "Displays current (left) and maximum (right) short-term loudness.",
        "Displays the loudness range. Brackets indicate a provisional measurement.",
        "Displays the overall integrated loudness.",
        ""
    };
}
