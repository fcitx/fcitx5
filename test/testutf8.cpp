/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include <cstdio>
#include <cstring>
#include "fcitx-utils/cutf8.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/utf8.h"

#define BUF_SIZE 9

int main() {
    char buf[BUF_SIZE];
    const char string[] = "\xe4\xbd\xa0\xe5\xa5\xbd\xe6\xb5"
                          "\x8b\xe8\xaf\x95\xe5\xb8\x8c\xe6";
    const char result[] = {'\xe4', '\xbd', '\xa0', '\xe5', '\xa5',
                           '\xbd', '\0',   '\0',   '\0'};
    fcitx_utf8_strncpy(buf, string, BUF_SIZE - 1);
    buf[BUF_SIZE - 1] = 0;
    FCITX_ASSERT(memcmp(buf, result, BUF_SIZE) == 0);
    FCITX_UNUSED(result);

    FCITX_ASSERT(fcitx_utf8_strnlen(string, 0) == 0);
    FCITX_ASSERT(fcitx_utf8_strnlen(string, 1) == 0);
    FCITX_ASSERT(fcitx_utf8_strnlen(string, 2) == 0);
    FCITX_ASSERT(fcitx_utf8_strnlen(string, 3) == 1);
    FCITX_ASSERT(fcitx_utf8_strnlen(string, 6) == 2);
    FCITX_ASSERT(fcitx_utf8_strnlen(string, 8) == 2);
    FCITX_ASSERT(fcitx_utf8_strnlen(string, 9) == 3);

    for (uint32_t c = 0; c < 0x4000000; c++) {
        char utf8_buf[7];
        int len = fcitx_ucs4_to_utf8(c, utf8_buf);
        if (fcitx_utf8_check_string(utf8_buf)) {
            uint32_t c2 = 0;
            char *pos = fcitx_utf8_get_char(utf8_buf, &c2);
            if (c != c2) {
                printf("%x %x\n", c, c2);
                printf("%d\n", len);
            }
            FCITX_ASSERT(c == c2);
            FCITX_ASSERT(pos == utf8_buf + len);
        }
    }

    std::string str = "\xe4\xbd\xa0\xe5\xa5\xbd\xe5\x90\x97\x61\x62\x63\x0a";
    FCITX_ASSERT(fcitx::utf8::validate(str));
    FCITX_ASSERT(fcitx::utf8::lengthValidated(str) == 7);
    uint32_t expect[] = {0x4f60, 0x597d, 0x5417, 0x61, 0x62, 0x63, 0x0a};
    uint32_t expectLength[] = {3, 3, 3, 1, 1, 1, 1};
    std::string expectCharStr[] = {
        "\xe4\xbd\xa0", "\xe5\xa5\xbd", "\xe5\x90\x97", "\x61",
        "\x62",         "\x63",         "\x0a"};
    int counter = 0;
    for (auto c : fcitx::utf8::MakeUTF8CharRange(str)) {
        FCITX_ASSERT(expect[counter] == c);
        counter++;
    }

    auto range = fcitx::utf8::MakeUTF8CharRange(str);
    int i = 0;
    for (auto iter = std::begin(range), end = std::end(range); iter != end;
         ++iter, ++i) {
        FCITX_ASSERT(iter.charLength() == iter.view().length());
        FCITX_ASSERT(iter.charLength() == expectLength[i]);
        FCITX_ASSERT(iter.view() == expectCharStr[i]);
    }

    FCITX_ASSERT(fcitx::utf8::getLastChar(str) == 0xa);

    std::string invalidStr = "\xe4\xff";
    FCITX_ASSERT(fcitx::utf8::getLastChar(invalidStr) ==
                 fcitx::utf8::INVALID_CHAR);
    std::string empty;
    FCITX_ASSERT(fcitx::utf8::getLastChar(empty) ==
                 fcitx::utf8::NOT_ENOUGH_SPACE);
    FCITX_ASSERT(fcitx::utf8::length(empty) == 0);
    FCITX_ASSERT(fcitx::utf8::lengthValidated(empty) == 0);
    FCITX_ASSERT(fcitx::utf8::lengthValidated(invalidStr) ==
                 fcitx::utf8::INVALID_LENGTH);

    FCITX_ASSERT(counter == 7);

    FCITX_ASSERT(!fcitx::utf8::UCS4IsValid(0xfdd7));

    return 0;
}
