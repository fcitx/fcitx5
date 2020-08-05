/*
 * SPDX-FileCopyrightText: 2010-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "cutf8.h"
#include <cstdint>
#include <cstring>
#include "fcitxutils_export.h"
#include "utf8.h"

/** check utf8 character */
#define FCITX_ISUTF8_CB(c) (((c)&0xc0) == 0x80)

#define CONT(i) FCITX_ISUTF8_CB(in[i])
#define VAL(i, s) ((in[i] & 0x3f) << s)

#define UTF8_LENGTH(Char)                                                      \
    ((Char) < 0x80                                                             \
         ? 1                                                                   \
         : ((Char) < 0x800                                                     \
                ? 2                                                            \
                : ((Char) < 0x10000                                            \
                       ? 3                                                     \
                       : ((Char) < 0x200000 ? 4                                \
                                            : ((Char) < 0x4000000 ? 5 : 6)))))

#define UNICODE_VALID(Char)                                                    \
    ((Char) < 0x110000 && (((Char)&0xFFFFF800) != 0xD800) &&                   \
     ((Char) < 0xFDD0 || (Char) > 0xFDEF) && ((Char)&0xFFFE) != 0xFFFE)

FCITXUTILS_EXPORT
size_t fcitx_utf8_strlen(const char *s) {
    size_t l = 0;

    while (*s) {
        uint32_t chr;

        s = fcitx_utf8_get_char(s, &chr);
        l++;
    }

    return l;
}

FCITXUTILS_EXPORT
unsigned int fcitx_utf8_char_len(const char *in) {
    if (!(in[0] & 0x80)) {
        return 1;
    }

    /* 2-byte, 0x80-0x7ff */
    if ((in[0] & 0xe0) == 0xc0 && CONT(1)) {
        return 2;
    }

    /* 3-byte, 0x800-0xffff */
    if ((in[0] & 0xf0) == 0xe0 && CONT(1) && CONT(2)) {
        return 3;
    }

    /* 4-byte, 0x10000-0x1FFFFF */
    if ((in[0] & 0xf8) == 0xf0 && CONT(1) && CONT(2) && CONT(3)) {
        return 4;
    }

    /* 5-byte, 0x200000-0x3FFFFFF */
    if ((in[0] & 0xfc) == 0xf8 && CONT(1) && CONT(2) && CONT(3) && CONT(4)) {
        return 5;
    }

    /* 6-byte, 0x400000-0x7FFFFFF */
    if ((in[0] & 0xfe) == 0xfc && CONT(1) && CONT(2) && CONT(3) && CONT(4) &&
        CONT(5)) {
        return 6;
    }

    return 1;
}

FCITXUTILS_EXPORT
int fcitx_ucs4_char_len(uint32_t c) {
    if (c < 0x00080) {
        return 1;
    }
    if (c < 0x00800) {
        return 2;
    }
    if (c < 0x10000) {
        return 3;
    }
    if (c < 0x200000) {
        return 4;
        // below is not in UCS4 but in 32bit int.
    }
    if (c < 0x8000000) {
        return 5;
    }
    return 6;
}

FCITXUTILS_EXPORT
int fcitx_ucs4_to_utf8(uint32_t c, char *output) {
    if (c < 0x00080) {
        output[0] = (char)(c & 0xFF);
        output[1] = '\0';
        return 1;
    }
    if (c < 0x00800) {
        output[0] = (char)(0xC0 + ((c >> 6) & 0x1F));
        output[1] = (char)(0x80 + (c & 0x3F));
        output[2] = '\0';
        return 2;
    }
    if (c < 0x10000) {
        output[0] = (char)(0xE0 + ((c >> 12) & 0x0F));
        output[1] = (char)(0x80 + ((c >> 6) & 0x3F));
        output[2] = (char)(0x80 + (c & 0x3F));
        output[3] = '\0';
        return 3;
    }
    if (c < 0x200000) {
        output[0] = (char)(0xF0 + ((c >> 18) & 0x07));
        output[1] = (char)(0x80 + ((c >> 12) & 0x3F));
        output[2] = (char)(0x80 + ((c >> 6) & 0x3F));
        output[3] = (char)(0x80 + (c & 0x3F));
        output[4] = '\0';
        return 4;
        // below is not in UCS4 but in 32bit int.
    }
    if (c < 0x8000000) {
        output[0] = (char)(0xF8 + ((c >> 24) & 0x03));
        output[1] = (char)(0x80 + ((c >> 18) & 0x3F));
        output[2] = (char)(0x80 + ((c >> 12) & 0x3F));
        output[3] = (char)(0x80 + ((c >> 6) & 0x3F));
        output[4] = (char)(0x80 + (c & 0x3F));
        output[5] = '\0';
        return 5;
    }
    output[0] = (char)(0xFC + ((c >> 30) & 0x01));
    output[1] = (char)(0x80 + ((c >> 24) & 0x3F));
    output[2] = (char)(0x80 + ((c >> 18) & 0x3F));
    output[3] = (char)(0x80 + ((c >> 12) & 0x3F));
    output[4] = (char)(0x80 + ((c >> 6) & 0x3F));
    output[5] = (char)(0x80 + (c & 0x3F));
    output[6] = '\0';
    return 6;
}

FCITXUTILS_EXPORT
char *fcitx_utf8_get_char(const char *i, uint32_t *chr) {
    const unsigned char *in = (const unsigned char *)i;
    if (!(in[0] & 0x80)) {
        *(chr) = *(in);
        return (char *)in + 1;
    }

    /* 2-byte, 0x80-0x7ff */
    if ((in[0] & 0xe0) == 0xc0 && CONT(1)) {
        *chr = ((in[0] & 0x1f) << 6) | VAL(1, 0);
        return (char *)in + 2;
    }

    /* 3-byte, 0x800-0xffff */
    if ((in[0] & 0xf0) == 0xe0 && CONT(1) && CONT(2)) {
        *chr = ((in[0] & 0xf) << 12) | VAL(1, 6) | VAL(2, 0);
        return (char *)in + 3;
    }

    /* 4-byte, 0x10000-0x1FFFFF */
    if ((in[0] & 0xf8) == 0xf0 && CONT(1) && CONT(2) && CONT(3)) {
        *chr = ((in[0] & 0x7) << 18) | VAL(1, 12) | VAL(2, 6) | VAL(3, 0);
        return (char *)in + 4;
    }

    /* 5-byte, 0x200000-0x3FFFFFF */
    if ((in[0] & 0xfc) == 0xf8 && CONT(1) && CONT(2) && CONT(3) && CONT(4)) {
        *chr = ((in[0] & 0x3) << 24) | VAL(1, 18) | VAL(2, 12) | VAL(3, 6) |
               VAL(4, 0);
        return (char *)in + 5;
    }

    /* 6-byte, 0x400000-0x7FFFFFF */
    if ((in[0] & 0xfe) == 0xfc && CONT(1) && CONT(2) && CONT(3) && CONT(4) &&
        CONT(5)) {
        *chr = ((in[0] & 0x1) << 30) | VAL(1, 24) | VAL(2, 18) | VAL(3, 12) |
               VAL(4, 6) | VAL(5, 0);
        return (char *)in + 6;
    }

    *chr = *in;

    return (char *)in + 1;
}

FCITXUTILS_EXPORT
char *fcitx_utf8_get_nth_char(const char *s, uint32_t n) {
    size_t l = 0;

    while (*s && l < n) {
        uint32_t chr;

        s = fcitx_utf8_get_char(s, &chr);
        l++;
    }

    return (char *)s;
}

static uint32_t fcitx_utf8_get_char_extended(const char *s, int max_len,
                                             int *plen) {
    const unsigned char *p = (const unsigned char *)s;
    int i, len;
    uint32_t wc = (unsigned char)*p;

    if (wc < 0x80) {
        if (plen) {
            *plen = 1;
        }
        return wc;
    }
    if (wc < 0xc0) {
        return (uint32_t)-1;
    }
    if (wc < 0xe0) {
        len = 2;
        wc &= 0x1f;
    } else if (wc < 0xf0) {
        len = 3;
        wc &= 0x0f;
    } else if (wc < 0xf8) {
        len = 4;
        wc &= 0x07;
    } else if (wc < 0xfc) {
        len = 5;
        wc &= 0x03;
    } else if (wc < 0xfe) {
        len = 6;
        wc &= 0x01;
    } else {
        return (uint32_t)-1;
    }

    if (max_len >= 0 && len > max_len) {
        for (i = 1; i < max_len; i++) {
            if ((((unsigned char *)p)[i] & 0xc0) != 0x80) {
                return (uint32_t)-1;
            }
        }

        return (uint32_t)-2;
    }

    for (i = 1; i < len; ++i) {
        uint32_t ch = ((unsigned char *)p)[i];

        if ((ch & 0xc0) != 0x80) {
            if (ch) {
                return (uint32_t)-1;
            }
            return (uint32_t)-2;
        }

        wc <<= 6;

        wc |= (ch & 0x3f);
    }

    if (UTF8_LENGTH(wc) != len) {
        return (uint32_t)-1;
    }

    if (plen) {
        *plen = len;
    }

    return wc;
}

FCITXUTILS_EXPORT
uint32_t fcitx_utf8_get_char_validated(const char *p, int max_len, int *plen) {
    uint32_t result;

    if (max_len == 0) {
        return fcitx::utf8::NOT_ENOUGH_SPACE;
    }

    int len;
    result = fcitx_utf8_get_char_extended(p, max_len, &len);

    if (result & 0x80000000) {
        return result;
    }
    if (!UNICODE_VALID(result)) {
        return fcitx::utf8::INVALID_CHAR;
    }

    if (plen) {
        *plen = len;
    }
    return result;
}

FCITXUTILS_EXPORT
bool fcitx_utf8_check_string(const char *s) {
    while (*s) {
        uint32_t chr;

        int len = 0;
        chr = fcitx_utf8_get_char_validated(s, FCITX_UTF8_MAX_LENGTH, &len);
        if (chr == fcitx::utf8::NOT_ENOUGH_SPACE ||
            chr == fcitx::utf8::INVALID_CHAR) {
            return false;
        }

        s += len;
    }

    return true;
}

FCITXUTILS_EXPORT
void fcitx_utf8_strncpy(char *str, const char *s, size_t byte) {
    while (*s) {
        uint32_t chr;

        const char *next = fcitx_utf8_get_char(s, &chr);
        size_t diff = next - s;
        if (byte < diff) {
            break;
        }

        memcpy(str, s, diff);
        str += diff;
        byte -= diff;
        s = next;
    }

    while (byte--) {
        *str = '\0';
        str++;
    }
}

FCITXUTILS_EXPORT
size_t fcitx_utf8_strnlen_validated(const char *str, size_t byte) {
    size_t len = 0;
    while (byte && *str) {
        int charLen = 0;
        uint32_t chr = fcitx_utf8_get_char_validated(
            str, (byte > FCITX_UTF8_MAX_LENGTH ? FCITX_UTF8_MAX_LENGTH : byte),
            &charLen);
        if (chr == fcitx::utf8::NOT_ENOUGH_SPACE ||
            chr == fcitx::utf8::INVALID_CHAR) {
            return fcitx::utf8::INVALID_LENGTH;
        }
        str += charLen;
        byte -= charLen;
        len++;
    }
    return len;
}

FCITXUTILS_EXPORT
size_t fcitx_utf8_strnlen(const char *str, size_t byte) {
    size_t len = 0;
    // if byte is zero no need to go further.
    while (byte && *str) {
        uint32_t chr;

        const char *next = fcitx_utf8_get_char(str, &chr);
        size_t diff = next - str;
        if (byte < diff) {
            break;
        }

        byte -= diff;
        str = next;
        len++;
    }
    return len;
}
