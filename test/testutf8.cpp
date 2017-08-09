#include "fcitx-utils/cutf8.h"
#include "fcitx-utils/log.h"
#include <cstdio>
#include <cstring>

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

    return 0;
}
