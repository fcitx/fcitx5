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

#ifndef _FCITX_UTILS_COLOR_H_
#define _FCITX_UTILS_COLOR_H_

#include "fcitxutils_export.h"
#include <string>
#include <type_traits>

namespace fcitx {
struct ColorParseException : public std::exception {
    virtual const char *what() const noexcept { return "Color parse error"; }
};

class FCITXUTILS_EXPORT Color {
public:
    Color();
    explicit Color(unsigned short r, unsigned short g, unsigned short b,
                   unsigned short alpha = 255);
    explicit inline Color(const char *s) { setFromString(s); }
    explicit inline Color(const std::string &s) : Color(s.c_str()) {}
    Color(const Color &other)
        : Color(other.red(), other.green(), other.blue(), other.alpha()) {}

    std::string toString() const;

    bool operator==(const Color &other) const;

    void setFromString(const char *s);
    inline void setFromString(const std::string &s) {
        setFromString(s.c_str());
    }

    void setRed(unsigned short);
    void setGreen(unsigned short);
    void setBlue(unsigned short);
    void setAlpha(unsigned short);

    void setRedF(float);
    void setGreenF(float);
    void setBlueF(float);
    void setAlphaF(float);

    unsigned short red() const;
    unsigned short green() const;
    unsigned short blue() const;
    unsigned short alpha() const;

    float redF() const;
    float greenF() const;
    float blueF() const;
    float alphaF() const;

private:
    unsigned short m_red;
    unsigned short m_green;
    unsigned short m_blue;
    unsigned short m_alpha;
};
}

#endif // _FCITX_UTILS_COLOR_H_
