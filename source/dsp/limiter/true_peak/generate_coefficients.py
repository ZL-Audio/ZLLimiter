#  Copyright (C) 2026 - zsliu98
#  This file is part of ZLLimiter
#
#  ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
#
#  ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
#
#  You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

import argparse
from pathlib import Path

import numpy as np


def coefficients(taps=64, phases=16, beta=8.6):
    centers = taps / 2 - 1 + (np.arange(phases) + 0.5) / phases
    distance = np.arange(taps)[None, :] - centers[:, None]
    window = np.i0(beta * np.sqrt(np.maximum(0, 1 - (distance / (taps / 2)) ** 2))) / np.i0(beta)
    table = np.sinc(distance) * window
    table /= table.sum(axis=1)[:, None]
    table[phases // 2:] = table[:phases // 2][::-1, ::-1]
    return table


def render():
    table = coefficients()
    lines = [
        "#pragma once", "", "#include <array>", "#include <cstddef>", "",
        "namespace zldsp::limiter::true_peak_coefficients {",
        "    inline constexpr size_t kNumPhases = 16;",
        "    inline constexpr size_t kTapsPerPhase = 64;",
        "    inline constexpr std::array<std::array<double, kTapsPerPhase>, kNumPhases> kTable{{",
    ]
    for phase in table:
        lines.append("        {{")
        for start in range(0, len(phase), 4):
            lines.append("            " + ", ".join(f"{v:.17e}" for v in phase[start:start + 4]) + ",")
        lines.append("        }},")
    lines += ["    }};", "}", ""]
    return "\n".join(lines)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    path = Path(__file__).with_name("true_peak_coefficients.hpp")
    expected = render()
    if args.check:
        if path.read_text() != expected:
            raise SystemExit("true-peak coefficients differ; regenerate the table")
        print("true-peak coefficients match")
    else:
        path.write_text(expected)
