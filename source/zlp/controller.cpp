// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
// License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more
// details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see
// <https://www.gnu.org/licenses/>.

#include "controller.hpp"

namespace zlp {
    Controller::Controller(juce::AudioProcessor& p) : p_ref_(p) {}

    void Controller::prepare(const double sample_rate, const size_t) {}

    void Controller::prepareBuffer() {}

    void Controller::process(const std::array<float*, 2>& buffer, const size_t num_samples, const bool is_bypass) {}

    void Controller::handleAsyncUpdate() {}
}
