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

#include "fcitxutils_export.h"
#include <fcitx-utils/cutf8.h>
#include <string>

namespace fcitx {
namespace utf8 {
FCITXUTILS_EXPORT inline size_t length(const std::string &s) { return fcitx_utf8_strnlen(s.c_str(), s.size()); }
FCITXUTILS_EXPORT inline size_t lengthN(const std::string &s, size_t n) { return fcitx_utf8_strnlen(s.c_str(), n); }

static const size_t INVALID_LENGTH = static_cast<size_t>(-1);

FCITXUTILS_EXPORT inline size_t lengthValidated(const std::string &s) {
    return fcitx_utf8_strnlen_validated(s.c_str(), s.size());
}

FCITXUTILS_EXPORT inline size_t lengthNValidated(const std::string &s, size_t n) {
    return fcitx_utf8_strnlen_validated(s.c_str(), n);
}

FCITXUTILS_EXPORT inline bool validate(const std::string &s) { return fcitx_utf8_check_string(s.c_str()); }

FCITXUTILS_EXPORT std::string UCS4ToUTF8(uint32_t code);

FCITXUTILS_EXPORT inline uint32_t getCharValidated(const std::string &s, size_t off = 0, int maxLen = 6) {
    if (off >= s.size()) {
        return 0;
    }
    return fcitx_utf8_get_char_validated(s.c_str() + off, maxLen, nullptr);
}

FCITXUTILS_EXPORT inline size_t charLength(const std::string &s) { return fcitx_utf8_char_len(s.c_str()); }

FCITXUTILS_EXPORT inline int nthChar(const std::string &s, int start, size_t n) {
    int diff = fcitx_utf8_get_nth_char(s.c_str() + start, n) - s.c_str();
    return diff;
}

FCITXUTILS_EXPORT inline int nthChar(const std::string &s, size_t n) { return nthChar(s, 0, n); }

template <typename Iter>
Iter nextChar(Iter iter) {
    const char *c = &(*iter);
    uint32_t ch;
    auto nc = fcitx_utf8_get_char(c, &ch);
    return std::next(iter, nc - c);
}
}
}

#endif // _FCITX_UTILS_UTF8_H_
