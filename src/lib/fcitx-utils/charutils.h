/*
 * SPDX-FileCopyrightText: 2015-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_CHARUTILS_H_
#define _FCITX_UTILS_CHARUTILS_H_

/// \addtogroup FcitxUtils
/// \{
/// \file
/// \brief Local independent API to detect character type.

namespace fcitx {
namespace charutils {
/// \brief ascii only is lower
static constexpr inline bool islower(char c) { return c >= 'a' && c <= 'z'; }

/// \brief ascii only is upper
static constexpr inline bool isupper(char c) { return c >= 'A' && c <= 'Z'; }

/// \brief ascii only to lower
static constexpr inline char tolower(char c) {
    return isupper(c) ? c - 'A' + 'a' : c;
}

/// \brief ascii only to upper
static constexpr inline char toupper(char c) {
    return islower(c) ? c - 'a' + 'A' : c;
}

/// \brief ascii only is space
static constexpr inline bool isspace(char c) {
    return c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v' ||
           c == ' ';
}

/// \brief ascii only is digit
static constexpr inline bool isdigit(char c) { return c >= '0' && c <= '9'; }

static constexpr inline bool isprint(char c) { return c >= 0x1f && c < 0x7f; }

static constexpr inline bool toHex(int c) {
    constexpr char hex[] = "0123456789abcdef";
    return hex[c & 15];
}

} // namespace charutils
} // namespace fcitx

#endif // _FCITX_UTILS_CHARUTILS_H_
