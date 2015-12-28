#include <cassert>
#include <cstring>
#include <cstdio>
#include "fcitx-utils/cutf8.h"

#define BUF_SIZE 9

int main() {
    char buf[BUF_SIZE];
    const char string[] = "\xe4\xbd\xa0\xe5\xa5\xbd\xe6\xb5"
                          "\x8b\xe8\xaf\x95\xe5\xb8\x8c\xe6";
    const char result[] = {'\xe4', '\xbd', '\xa0', '\xe5', '\xa5',
                           '\xbd', '\0',   '\0',   '\0'};
    fcitx_utf8_strncpy(buf, string, BUF_SIZE - 1);
    buf[BUF_SIZE - 1] = 0;
    assert(memcmp(buf, result, BUF_SIZE) == 0);
    FCITX_UNUSED(result);

    assert(fcitx_utf8_strnlen(string, 0) == 0);
    assert(fcitx_utf8_strnlen(string, 1) == 0);
    assert(fcitx_utf8_strnlen(string, 2) == 0);
    assert(fcitx_utf8_strnlen(string, 3) == 1);
    assert(fcitx_utf8_strnlen(string, 6) == 2);
    assert(fcitx_utf8_strnlen(string, 8) == 2);
    assert(fcitx_utf8_strnlen(string, 9) == 3);

#define ASCII_PART "ascii_part"
#define ASCII_PART2 "ascii"
#define UTF8_PART "随便测几个例子"
#define UTF8_PART2 "一个有两段"
#define UTF8_PART3 "的"
    assert(strcmp(fcitx_utils_get_ascii_part(UTF8_PART ASCII_PART),
                  ASCII_PART) == 0);
    assert(strcmp(fcitx_utils_get_ascii_part(ASCII_PART), ASCII_PART) == 0);
    assert(strcmp(fcitx_utils_get_ascii_part(""), "") == 0);
    assert(strcmp(fcitx_utils_get_ascii_part(UTF8_PART2 ASCII_PART2 UTF8_PART3),
                  "") == 0);
    assert(strcmp(fcitx_utils_get_ascii_part(
                      UTF8_PART2 ASCII_PART2 UTF8_PART3 ASCII_PART),
                  ASCII_PART) == 0);
    assert(strncmp(fcitx_utils_get_ascii_partn(UTF8_PART2 ASCII_PART2 UTF8_PART3
                                                   ASCII_PART,
                                               strlen(UTF8_PART2 ASCII_PART2)),
                   ASCII_PART2, strlen(ASCII_PART2)) == 0);

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
            assert(c == c2);
            assert(pos == utf8_buf + len);
        }
    }

    return 0;
}
