/*
 * SPDX-FileCopyrightText: 2015-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "color.h"
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <ostream>
#include <string>
#include "charutils.h"
#include "stringutils.h"

namespace fcitx {
static unsigned short roundColor(unsigned short c) {
    return c <= 255 ? c : 255;
}

static float roundColorF(float f) {
    return f < 0 ? 0.0F : (f > 1.0 ? 1.0F : f);
}

static unsigned short extendColor(unsigned short c) {
    c = roundColor(c);
    return c << 8 | c;
}

static inline char to_hex_char(int v) {
    return (char)(((v) >= 0 && (v) < 10) ? (v + '0') : (v - 10 + 'a'));
}

static inline unsigned short to_hex_digit(char hi, char lo) {
    hi = charutils::tolower(hi);
    lo = charutils::tolower(lo);
    int dhi = 0;
    int dlo = 0;
    if (hi >= '0' && hi <= '9') {
        dhi = hi - '0';
    } else {
        dhi = hi - 'a' + 10;
    }
    if (lo >= '0' && lo <= '9') {
        dlo = lo - '0';
    } else {
        dlo = lo - 'a' + 10;
    }

    return dhi * 16 + dlo;
}

Color::Color(unsigned short r, unsigned short g, unsigned short b,
             unsigned short alpha)
    : red_(extendColor(r)), green_(extendColor(g)), blue_(extendColor(b)),
      alpha_(extendColor(alpha)) {}

Color::Color() : red_(0), green_(0), blue_(0), alpha_(USHRT_MAX) {}

bool Color::operator==(const Color &other) const {
    return red_ == other.red_ && green_ == other.green_ &&
           blue_ == other.blue_ && alpha_ == other.alpha_;
}

void Color::setFromString(const char *str) {
    size_t idx = 0;

    // skip space
    while (str[idx] && charutils::isspace(str[idx])) {
        idx++;
    }

    if (str[idx] == '#') {
        // count the digit length
        size_t len = 0;
        const char *digits = &str[idx + 1];
        while (digits[len] &&
               (charutils::isdigit(digits[len]) ||
                ('A' <= digits[len] && digits[len] <= 'F') |
                    ('a' <= digits[len] && digits[len] <= 'f'))) {
            len++;
        }
        if (len != 8 && len != 6) {
            throw ColorParseException();
        }

        uint16_t r;
        uint16_t g;
        uint16_t b;
        uint16_t a;
        r = to_hex_digit(digits[0], digits[1]);
        digits += 2;
        g = to_hex_digit(digits[0], digits[1]);
        digits += 2;
        b = to_hex_digit(digits[0], digits[1]);
        if (len == 8) {
            digits += 2;
            a = to_hex_digit(digits[0], digits[1]);
        } else {
            a = 255;
        }

        red_ = extendColor(r);
        green_ = extendColor(g);
        blue_ = extendColor(b);
        alpha_ = extendColor(a);
    } else {
        uint16_t r;
        uint16_t g;
        uint16_t b;
        if (sscanf(str, "%hu %hu %hu", &r, &g, &b) != 3) {
            throw ColorParseException();
        }

        red_ = extendColor(r);
        green_ = extendColor(g);
        blue_ = extendColor(b);
        alpha_ = extendColor(255);
    }
}

std::string Color::toString() const {
    std::string result;
    result.push_back('#');
    unsigned short v[] = {
        static_cast<unsigned short>(red_ >> 8U),
        static_cast<unsigned short>(green_ >> 8U),
        static_cast<unsigned short>(blue_ >> 8U),
        static_cast<unsigned short>(alpha_ >> 8U),
    };

    for (auto value : v) {
        auto hi = value / 16;
        auto lo = value % 16;
        result.push_back(to_hex_char(hi));
        result.push_back(to_hex_char(lo));
    }
    if (stringutils::endsWith(result, "ff")) {
        result.erase(result.size() - 2, 2);
    }

    return result;
}

unsigned short Color::red() const { return red_ >> 8; }
unsigned short Color::green() const { return green_ >> 8; }
unsigned short Color::blue() const { return blue_ >> 8; }
unsigned short Color::alpha() const { return alpha_ >> 8; }

float Color::redF() const { return red_ / float(USHRT_MAX); }
float Color::greenF() const { return green_ / float(USHRT_MAX); }
float Color::blueF() const { return blue_ / float(USHRT_MAX); }
float Color::alphaF() const { return alpha_ / float(USHRT_MAX); }

void Color::setRed(unsigned short red) { red_ = extendColor(red); }
void Color::setGreen(unsigned short green) { green_ = extendColor(green); }
void Color::setBlue(unsigned short blue) { blue_ = extendColor(blue); }
void Color::setAlpha(unsigned short alpha) { alpha_ = extendColor(alpha); }

void Color::setRedF(float red) {
    red_ = std::round(roundColorF(red) * USHRT_MAX);
}
void Color::setGreenF(float green) {
    green_ = std::round(roundColorF(green) * USHRT_MAX);
}
void Color::setBlueF(float blue) {
    blue_ = std::round(roundColorF(blue) * USHRT_MAX);
}
void Color::setAlphaF(float alpha) {
    alpha_ = std::round(roundColorF(alpha) * USHRT_MAX);
}

std::ostream &operator<<(std::ostream &os, const Color &c) {
    os << "Color(" << c.toString() << ")";
    return os;
}
} // namespace fcitx
