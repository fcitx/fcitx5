/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "utf8.h"
#include <cstdint>
#include <string>
#include "cutf8.h"

namespace fcitx::utf8 {

bool UCS4IsValid(uint32_t code) {
    return ((code) < 0x110000 && (((code) & 0xFFFFF800) != 0xD800));
}

std::string UCS4ToUTF8(uint32_t code) {
    if (!code) {
        return "";
    }
    char buf[FCITX_UTF8_MAX_LENGTH + 1];
    auto length = fcitx_ucs4_to_utf8(code, buf);
    return {buf, buf + length};
}

#ifdef _WIN32

std::string UTF16ToUTF8(std::wstring_view data) {
    std::string result;
    for (size_t i = 0; i < data.size();) {
        uint32_t ucs4 = 0;
        uint16_t chr = data[i];
        uint16_t chrNext = (i + 1 == data.size()) ? 0 : data[i + 1];
        if (chr < 0xD800 || chr > 0xDFFF) {
            ucs4 = chr;
            i += 1;
        } else if (0xD800 <= chr && chr <= 0xDBFF) {
            if (!chrNext) {
                return {};
            }
            if (0xDC00 <= chrNext && chrNext <= 0xDFFF) {
                /* We have a valid surrogate pair.  */
                ucs4 = (((chr & 0x3FF) << 10) | (chrNext & 0x3FF)) + (1 << 16);
                i += 2;
            }
        } else if (0xDC00 <= chr && chr <= 0xDFFF) {
            return {};
        }
        result.append(utf8::UCS4ToUTF8(ucs4));
    }
    return result;
}

std::wstring UTF8ToUTF16(std::string_view str) {
    if (!utf8::validate(str)) {
        return {};
    }
    std::wstring result;
    for (const auto ucs4 : utf8::MakeUTF8CharRange(str)) {
        if (ucs4 < 0x10000) {
            result.push_back(static_cast<uint16_t>(ucs4));
        } else if (ucs4 < 0x110000) {
            result.push_back(0xD800 | (((ucs4 - 0x10000) >> 10) & 0x3ff));
            result.push_back(0xDC00 | (ucs4 & 0x3ff));
        } else {
            return {};
        }
    }
    return result;
}

#endif

} // namespace fcitx::utf8
