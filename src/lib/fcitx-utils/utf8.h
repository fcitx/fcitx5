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
#ifndef _FCITX_UTILS_UTF8_H_
#define _FCITX_UTILS_UTF8_H_

#include <string>
#include "cutf8.h"

namespace fcitx {
namespace utf8 {
inline std::string::size_type length(const std::string &s) {
    return fcitx_utf8_strlen(s.c_str());
}

inline uint32_t getCharValidated(const std::string &s,
                                 std::string::size_type off = 0,
                                 int maxLen = 6) {
    if (off >= s.size()) {
        return 0;
    }
    return fcitx_utf8_get_char_validated(s.c_str() + off, maxLen);
}

inline std::string::size_type charLength(const std::string &s) {
    return fcitx_utf8_char_len(s.c_str());
}

inline int nthChar(const std::string &s, int start, size_t n) {
    int diff = fcitx_utf8_get_nth_char(s.c_str() + start, n) - s.c_str();
    return diff;
}

inline int nthChar(const std::string &s, size_t n) { return nthChar(s, 0, n); }
}
}

#endif // _FCITX_UTILS_UTF8_H_
