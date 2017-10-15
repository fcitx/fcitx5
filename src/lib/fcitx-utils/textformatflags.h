/*
 * Copyright (C) 2017~2017 by CSSlayer
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
#ifndef _FCITX_UTILS_TEXTFORMATFLAGS_H_
#define _FCITX_UTILS_TEXTFORMATFLAGS_H_

/// \addtogroup FcitxUtils
/// \{
/// \file
/// \brief Enum flag for text formatting.

#include <fcitx-utils/flags.h>

namespace fcitx {

/// \brief Enum flag for text formatting.
enum class TextFormatFlag : int {
    Underline = (1 << 3), /**< underline is a flag */
    HighLight = (1 << 4), /**< highlight the preedit */
    DontCommit = (1 << 5),
    Bold = (1 << 6),
    Strike = (1 << 7),
    Italic = (1 << 8),
    None = 0,
};

typedef Flags<TextFormatFlag> TextFormatFlags;
}

#endif // _FCITX_UTILS_TEXTFORMATFLAGS_H_
