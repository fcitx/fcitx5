/*
 * Copyright (C) 2015~2015 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING. If not,
 * see <http://www.gnu.org/licenses/>.
 */

#include <array>
#include <cstdio>
#include <climits>
#include <cmath>
#include "color.h"
#include "stringutils.h"
#include "charutils.h"

namespace fcitx {
static unsigned short roundColor(unsigned short c) {
    return c <= 255 ? c : 255;
}

static float roundColorF(float f) { return f < 0 ? 0.0f : (f > 1.0 ? 1.0 : f); }

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
    int dhi = 0, dlo = 0;
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

Color::Color(ushort r, ushort g, ushort b, ushort alpha)
    : m_red(extendColor(r)), m_green(extendColor(g)), m_blue(extendColor(b)),
      m_alpha(extendColor(alpha)) {}

Color::Color() : m_red(0), m_green(0), m_blue(0), m_alpha(USHRT_MAX) {}

bool Color::operator==(const Color &other) const {
    return m_red == other.m_red && m_green == other.m_green &&
           m_blue == other.m_blue && m_alpha == other.m_alpha;
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
        if (len != 8) {
            throw ColorParseException();
        }

        unsigned short r, g, b, a;
        r = to_hex_digit(digits[0], digits[1]);
        digits += 2;
        g = to_hex_digit(digits[0], digits[1]);
        digits += 2;
        b = to_hex_digit(digits[0], digits[1]);
        digits += 2;
        a = to_hex_digit(digits[0], digits[1]);

        m_red = extendColor(r);
        m_green = extendColor(g);
        m_blue = extendColor(b);
        m_alpha = extendColor(a);
    } else {
        unsigned short r, g, b;
        if (sscanf(str, "%hu %hu %hu", &r, &g, &b) != 3) {
            throw ColorParseException();
        }

        m_red = extendColor(r);
        m_green = extendColor(g);
        m_blue = extendColor(b);
        m_alpha = extendColor(255);
    }
}

std::string Color::toString() const {
    std::string result;
    result.push_back('#');
    std::array<unsigned short, 4> v = {
        (unsigned short)(m_red >> 8u), (unsigned short)(m_green >> 8u),
        (unsigned short)(m_blue >> 8u), (unsigned short)(m_alpha >> 8u),
    };

    for (auto value : v) {
        auto hi = value / 16;
        auto lo = value % 16;
        result.push_back(to_hex_char(hi));
        result.push_back(to_hex_char(lo));
    }

    return result;
}

unsigned short Color::red() const { return m_red >> 8; }
unsigned short Color::green() const { return m_green >> 8; }
unsigned short Color::blue() const { return m_blue >> 8; }
unsigned short Color::alpha() const { return m_alpha >> 8; }

float Color::redF() const { return m_red / float(USHRT_MAX); }
float Color::greenF() const { return m_green / float(USHRT_MAX); }
float Color::blueF() const { return m_blue / float(USHRT_MAX); }
float Color::alphaF() const { return m_alpha / float(USHRT_MAX); }

void Color::setRed(unsigned short red) { m_red = extendColor(red); }
void Color::setGreen(unsigned short green) { m_green = extendColor(green); }
void Color::setBlue(unsigned short blue) { m_blue = extendColor(blue); }
void Color::setAlpha(unsigned short alpha) { m_alpha = extendColor(alpha); }

void Color::setRedF(float red) {
    m_red = std::round(roundColorF(red) * USHRT_MAX);
}
void Color::setGreenF(float green) {
    m_green = std::round(roundColorF(green) * USHRT_MAX);
}
void Color::setBlueF(float blue) {
    m_blue = std::round(roundColorF(blue) * USHRT_MAX);
}
void Color::setAlphaF(float alpha) {
    m_alpha = std::round(roundColorF(alpha) * USHRT_MAX);
}
}
