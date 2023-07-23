/*
 * SPDX-FileCopyrightText: 2015-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_CHARUTILS_H_
#define _FCITX_UTILS_CHARUTILS_H_

#include <limits>

/// \addtogroup FcitxUtils
/// \{
/// \file
/// \brief Local independent API to detect character type.

namespace fcitx::charutils {
/// \brief ascii only is lower
static constexpr inline bool islower(char c) { return c >= 'a' && c <= 'z'; }

/// \brief ascii only is upper
static constexpr inline bool isupper(char c) { return c >= 'A' && c <= 'Z'; }

/// \brief ascii only to lower
static constexpr inline char tolower(char c) {
    return isupper(c) ? static_cast<char>(c - 'A' + 'a') : c;
}

/// \brief ascii only to upper
static constexpr inline char toupper(char c) {
    return islower(c) ? static_cast<char>(c - 'a' + 'A') : c;
}

/// \brief ascii only is space
static constexpr inline bool isspace(char c) {
    return c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v' ||
           c == ' ';
}

/// \brief ascii only is digit
static constexpr inline bool isdigit(char c) { return c >= '0' && c <= '9'; }

static constexpr inline bool isprint(char c) {
    constexpr char minPrint = 0x1f;
    return c >= minPrint && c < std::numeric_limits<signed char>::max();
}

static constexpr inline char toHex(int c) {
    constexpr char hex[] = "0123456789abcdef";
    return hex[c & 0xf];
}

/**
 * Return integer value for hex char
 *
 * @param c input char
 * @return return integer for hex, if not hex, return -1
 * @since 5.0.5
 */
static constexpr inline int fromHex(char c) {
    if (isdigit(c)) {
        return c - '0';
    }

    c = tolower(c);
    if ((c >= 'a') && (c <= 'f')) {
        return c - 'a' + 10;
    }
    return -1;
}

static constexpr inline bool isxdigit(char c) {
    return isdigit(c) || ((c >= 'a') && (c <= 'f')) ||
           ((c >= 'A') && (c <= 'F'));
}

} // namespace fcitx::charutils

#endif // _FCITX_UTILS_CHARUTILS_H_
