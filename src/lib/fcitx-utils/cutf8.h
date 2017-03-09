/*
 * Copyright (C) 2010~2015 by CSSlayer
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

/**
 * @addtogroup FcitxUtils
 * @{
 */

/**
 * @file   cutf8.h
 * @author CS Slayer wengxt@gmail.com
 *
 *  Utf8 related utils function
 *
 */
#ifndef _FCITX_UTILS_CUTF8_H_
#define _FCITX_UTILS_CUTF8_H_

#include <cstdint>
#include <cstdlib>
#include <fcitx-utils/macros.h>

FCITX_C_DECL_BEGIN

/** max length of a utf8 character */
#define FCITX_UTF8_MAX_LENGTH 6

/** check utf8 character */
#define FCITX_ISUTF8_CB(c) (((c)&0xc0) == 0x80)

static inline int fcitx_utf8_type(char c) {
    if (!(c & 0x80))
        return 1;
    if (!(c & 0x40))
        return 0;
    if (!(c & 0x20))
        return 2;
    if (!(c & 0x10))
        return 3;
    if (!(c & 0x08))
        return 4;
    if (!(c & 0x04))
        return 5;
    if (!(c & 0x02))
        return 6;
    return -1;
}

static inline int fcitx_utf8_valid_start(char c) {
    unsigned char uc = (unsigned char)c;
    if (!(uc & 0x80))
        return 1;
    if (!(uc & 0x40))
        return 0;
    return uc < 0xfe;
}

/**
 * Get utf8 string length
 *
 * @param s string
 * @return length
 **/
size_t fcitx_utf8_strlen(const char *s);

/**
 * get next char in the utf8 string
 *
 * @param in string
 * @param chr return unicode
 * @return next char pointer
 **/
char *fcitx_utf8_get_char(const char *in, uint32_t *chr);

/**
 * compare utf8 string, with utf8 string length n
 * result is similar as strcmp, compare with unicode
 *
 * @param s1 string1
 * @param s2 string2
 * @param n length
 * @return result
 **/
int fcitx_utf8_strncmp(const char *s1, const char *s2, int n);

/**
 * get next character length
 *
 * @param in string
 * @return length
 **/
unsigned int fcitx_utf8_char_len(const char *in);

/**
 * next pointer to the nth character, n start with 0
 * this function will not touch the content for s, so const pointer
 * can be safely passed and converted.
 *
 * @param s string
 * @param n index
 * @return next n character pointer
 **/
char *fcitx_utf8_get_nth_char(const char *s, uint32_t n);

/**
 * check utf8 string is valid or not, valid is 1, invalid is 0
 *
 * @param s string
 * @return valid or not
 **/
bool fcitx_utf8_check_string(const char *s);

/**
 * get extened character
 *
 * @param p string
 * @param max_len max length
 * @return int
 **/
uint32_t fcitx_utf8_get_char_extended(const char *p, int max_len);

/**
 * get validated character
 *
 * @param p string
 * @param max_len max length
 * @return int
 **/
uint32_t fcitx_utf8_get_char_validated(const char *p, int max_len);

/**
 * @brief copy most byte length, but keep utf8 valid
 *
 * @param str dest string
 * @param s source string
 * @param byte max length
 *
 * @since 4.2.3
 **/
void fcitx_utf8_strncpy(char *str, const char *s, size_t byte);

/**
 * @brief count most byte length, utf8 string length
 *
 * @param str string
 * @param byte max length
 * @return size_t
 *
 * @since 4.2.4
 **/
size_t fcitx_utf8_strnlen(const char *str, size_t byte);

/**
 * @brief get ucs4 char length
 *
 * @param c ucs4 char
 * @return int
 *
 * @since 4.2.5
 **/
int fcitx_ucs4_char_len(uint32_t c);

/**
 * @brief convert ucs4 char to utf8
 *
 * @param c ucs4 char
 * @param output output string, need to reserve enough space
 * @return int
 *
 * @since 4.2.5
 **/
int fcitx_ucs4_to_utf8(uint32_t c, char *output);

/**
 * @brief get the ascii part at the end of a utf8 string
 *
 * @param string a utf8 string
 * @return string pointer to the ascii part
 *
 * @since 4.2.6
 **/
const char *fcitx_utils_get_ascii_part(const char *string);

/**
 * @brief get the ascii part at the end of a utf8 string (with a given size)
 *
 * @param string a utf8 string
 * @param len the length of the string
 * @return string pointer to the ascii part
 *
 * @since 4.2.6
 **/
const char *fcitx_utils_get_ascii_partn(const char *string, size_t len);

/**
 * @brief get the position of the first non-ascii character in a string (with a
 *size limit)
 *
 * @param string a utf8 string
 * @param len the length of the string
 * @return string pointer to the position of the first non-ascii character or
 *the end of string
 *
 * @since 4.2.6
 **/
char *fcitx_utils_get_ascii_endn(const char *string, size_t len);

/**
 * @brief get the position of the first non-ascii character in a string
 *
 * @param string a utf8 string
 * @return string pointer to the position of the first non-ascii character or
 *the end of string
 *
 * @since 4.2.6
 **/
char *fcitx_utils_get_ascii_end(const char *string);

FCITX_C_DECL_END

#endif
