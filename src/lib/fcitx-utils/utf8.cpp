/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "utf8.h"
#include "cutf8.h"

namespace fcitx::utf8 {

bool UCS4IsValid(uint32_t code) {
    return ((code) < 0x110000 && (((code)&0xFFFFF800) != 0xD800) &&
            ((code) < 0xFDD0 || (code) > 0xFDEF) && ((code)&0xFFFE) != 0xFFFE);
}

std::string UCS4ToUTF8(uint32_t code) {
    if (!code) {
        return "";
    }
    char buf[FCITX_UTF8_MAX_LENGTH + 1];
    auto length = fcitx_ucs4_to_utf8(code, buf);
    return {buf, buf + length};
}
} // namespace fcitx::utf8
