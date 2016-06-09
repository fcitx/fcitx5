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
#ifndef _FCITX_UTILS_CHARUTILS_H_
#define _FCITX_UTILS_CHARUTILS_H_

namespace fcitx {
namespace charutils {
/**
 * ascii only is lower
 *
 * @param c char
 * @return bool
 */
static inline bool islower(char c) { return c >= 'a' && c <= 'z'; }

/**
 * ascii only is upper
 *
 * @param c char
 * @return bool
 */
static inline bool isupper(char c) { return c >= 'A' && c <= 'Z'; }

static inline char tolower(char c) { return isupper(c) ? c - 'A' + 'a' : c; }

static inline char toupper(char c) { return islower(c) ? c - 'a' + 'A' : c; }

#define FCITX_WHITESPACE "\f\n\r\t\v "

static inline bool isspace(char c) { return c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v' || c == ' '; }

static inline bool isdigit(char c) { return c >= '0' && c <= '9'; }
}
}

#endif // _FCITX_UTILS_CHARUTILS_H_
