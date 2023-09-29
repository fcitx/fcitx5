#include "colorhelper.h"
#include <cmath>
#include <fcitx-utils/color.h>

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

Color accentForeground(const Color &accent) {
    auto c = Color(255, 255, 255);
    // light bg
    if (luma(accent) > 0.5) {
        c = Color(0, 0, 0);
    }
    return c;
}

} // namespace fcitx::classicui