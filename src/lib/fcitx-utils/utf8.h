/*
 * Copyright (C) 2015~2017 by CSSlayer
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
#ifndef _FCITX_UTILS_UTF8_H_
#define _FCITX_UTILS_UTF8_H_

/// \addtogroup FcitxUtils
/// @{
/// \file
/// \brief C++ Utility functions for handling utf8 strings.

#include "fcitxutils_export.h"
#include <fcitx-utils/cutf8.h>
#include <string>

namespace fcitx {
namespace utf8 {

/// \brief Return the number UTF-8 characters in the string iterator range.
/// \see lengthValidated()
template <typename Iter>
inline size_t length(Iter start, Iter end) {
    return fcitx_utf8_strnlen(&(*start), std::distance(start, end));
}

/// \brief Return the number UTF-8 characters in the string.
/// \see lengthValidated()
template <typename T>
inline size_t length(const T &s) {
    return length(std::begin(s), std::end(s));
}

/// \brief Return the number UTF-8 characters in the string.
template <typename T>
inline size_t length(const T &s, size_t start, size_t end) {
    return length(std::next(std::begin(s), start),
                  std::next(std::begin(s), end));
}

/// \brief Possible return value of lengthValidated if the string is not valid.
/// \see lengthValidated()
constexpr size_t INVALID_LENGTH = static_cast<size_t>(-1);

/// \brief Validate and return the number UTF-8 characters in the string
/// iterator range
///
/// Will return INVALID_LENGTH if string is not a valid utf8 string.
template <typename Iter>
inline size_t lengthValidated(Iter start, Iter end) {
    return fcitx_utf8_strnlen_validated(&(*start), std::distance(start, end));
}

/// \brief Validate and return the number UTF-8 characters in the string
///
/// Will return INVALID_LENGTH if string is not a valid utf8 string.
template <typename T>
inline size_t lengthValidated(const T &s) {
    return lengthValidated(std::begin(s), std::end(s));
    ;
}

/// \brief Check if the string iterator range is valid utf8 string
template <typename Iter>
inline bool validate(Iter start, Iter end) {
    return lengthValidated(start, end) != INVALID_LENGTH;
}

/// \brief Check if the string is valid utf8 string.
template <typename T>
static inline bool validate(const T &s) {
    return validate(std::begin(s), std::end(s));
}

/// \brief Convert UCS4 to UTF8 string.
FCITXUTILS_EXPORT std::string UCS4ToUTF8(uint32_t code);

/// \brief Possible return value for getChar.
constexpr size_t INVALID_CHAR = static_cast<uint32_t>(-1);

/// \brief Possible return value for getChar.
constexpr size_t NOT_ENOUGH_SPACE = static_cast<uint32_t>(-2);

/// \brief Get next UCS4 char from iter, do not cross end.
template <typename Iter>
static uint32_t getChar(Iter iter, Iter end) {
    const char *c = &(*iter);
    return fcitx_utf8_get_char_validated(c, std::distance(iter, end), nullptr);
}

/// \brief Get next UCS4 char
template <typename T>
static uint32_t getChar(const T &s) {
    return getChar(std::begin(s), std::end(s));
}

/// \brief get the byte length of next N utf-8 character.
///
/// This function has no error check on invalid string or end of string. Check
/// the string before use it.
template <typename Iter>
inline int ncharByteLength(Iter iter, size_t n) {
    const char *c = &(*iter);
    int diff = fcitx_utf8_get_nth_char(c, n) - c;
    return diff;
}

/// \brief Move iter over next n character.
template <typename Iter>
inline Iter nextNChar(Iter iter, size_t n) {
    return std::next(iter, ncharByteLength(iter, n));
}

/// \brief Move iter over next one character.
template <typename Iter>
Iter nextChar(Iter iter) {
    return nextNChar(iter, 1);
}
}
}

#endif // _FCITX_UTILS_UTF8_H_
