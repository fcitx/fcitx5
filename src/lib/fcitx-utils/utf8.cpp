/*
 * Copyright (C) 2016~2016 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
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

#include "utf8.h"
#include "cutf8.h"

namespace fcitx {
namespace utf8 {

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
}
}
