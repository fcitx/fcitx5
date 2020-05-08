/*
 * SPDX-FileCopyrightText: 2010-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

/// \addtogroup FcitxUtils
/// \{
///
/// \file
/// \brief C-style utf8 utility functions.
#ifndef _FCITX_UTILS_CUTF8_H_
#define _FCITX_UTILS_CUTF8_H_

#include <cstdint>
#include <cstdlib>

//// Max length of a utf8 character
#define FCITX_UTF8_MAX_LENGTH 6

/// \brief Get utf8 string length
size_t fcitx_utf8_strlen(const char *s);

/// \brief Get UCS-4 char in the utf8 string
char *fcitx_utf8_get_char(const char *in, uint32_t *chr);

/// \brief Get the number of bytes of next character.
unsigned int fcitx_utf8_char_len(const char *in);

/// \brief Get the pointer to the nth character.
///
/// This function will not touch the content for s, so const pointer
/// can be safely passed and converted.
char *fcitx_utf8_get_nth_char(const char *s, uint32_t n);

/// \brief Check if the string is valid utf8 string.
bool fcitx_utf8_check_string(const char *s);

/// \brief Get validated character.
///
/// Returns the UCS-4 value if its valid character. Returns (uint32_t) -1 if
/// it is not a valid char, (uint32_t)-2 if length is not enough.
uint32_t fcitx_utf8_get_char_validated(const char *p, int max_len, int *plen);

/// \brief Copy most byte length, but keep utf8 valid.
void fcitx_utf8_strncpy(char *str, const char *s, size_t byte);

/// \brief Count most byte length, utf8 string length.
size_t fcitx_utf8_strnlen(const char *str, size_t byte);

/// \brief Count most byte length, utf8 string length and validates the string
size_t fcitx_utf8_strnlen_validated(const char *str, size_t byte);

/// \brief Return the utf8 bytes of a UCS4 char.
int fcitx_ucs4_char_len(uint32_t c);

/// \brief Convert ucs4 char to utf8, need to have enough memory for it.
int fcitx_ucs4_to_utf8(uint32_t c, char *output);

#endif
