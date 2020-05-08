/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
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
} // namespace fcitx

#endif // _FCITX_UTILS_TEXTFORMATFLAGS_H_
