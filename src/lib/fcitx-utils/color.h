/*
 * SPDX-FileCopyrightText: 2015-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#ifndef _FCITX_UTILS_COLOR_H_
#define _FCITX_UTILS_COLOR_H_

#include <ostream>
#include <string>
#include <type_traits>
#include <fcitx-utils/macros.h>
#include "fcitxutils_export.h"

/// \addtogroup FcitxUtils
/// \{
/// \file
/// \brief Simple color class that represent a 64bit color.

namespace fcitx {
struct FCITXUTILS_EXPORT ColorParseException : public std::exception {
    virtual const char *what() const noexcept { return "Color parse error"; }
};

/// \brief Color class for handling color.
class FCITXUTILS_EXPORT Color {
public:
    Color();
    explicit Color(unsigned short r, unsigned short g, unsigned short b,
                   unsigned short alpha = 255);
    explicit inline Color(const char *s) { setFromString(s); }
    explicit inline Color(const std::string &s) : Color(s.c_str()) {}
    FCITX_INLINE_DEFINE_DEFAULT_DTOR_AND_COPY(Color);

    /// \brief Get color string in the format of "#rrggbbaa".
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
    unsigned short red_;
    unsigned short green_;
    unsigned short blue_;
    unsigned short alpha_;
};

FCITXUTILS_EXPORT std::ostream &operator<<(std::ostream &os, const Color &c);

} // namespace fcitx

#endif // _FCITX_UTILS_COLOR_H_
