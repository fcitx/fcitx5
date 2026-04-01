/*
 * SPDX-FileCopyrightText: 2023-2023 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "colorhelper.h"
#include <cmath>
#include <utility>
#include "fcitx-utils/color.h"

namespace fcitx::classicui {

inline float normalize(float v) {
    return (v < 1.0 ? (v > 0.0 ? v : 0.0) : 1.0);
}

float gamma(float n) { return std::pow(normalize(n), 2.2); }

float lumag(float r, float g, float b) {
    return r * 0.2126 + g * 0.7152 + b * 0.0722;
}

float luma(const Color &c) {
    return lumag(gamma(c.redF()), gamma(c.greenF()), gamma(c.blueF()));
}

std::pair<Color, Color> accentForeground(const Color &accent) {
    // light bg
    if (luma(accent) > 0.5) {
        return {Color(0, 0, 0), Color(51, 51, 51)};
    }
    return {Color(255, 255, 255), Color(204, 204, 204)};
}

} // namespace fcitx::classicui
