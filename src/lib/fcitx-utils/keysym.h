/*
 * Copyright (C) 2012~2015 by CSSlayer
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

#ifndef FCITX_UTILS_KEYSYM_H
#define FCITX_UTILS_KEYSYM_H

/// \addtogroup FcitxUtils
/// \{
/// \file
/// \brief Key sym related types.

#include <cstdint>
#include <fcitx-utils/keysymgen.h>
#include <fcitx-utils/macros.h>

namespace fcitx {
/// \brief KeyState to represent modifier keys.
enum class KeyState : uint32_t {
    None = 0,
    Shift = 1 << 0,
    CapsLock = 1 << 1,
    Ctrl = 1 << 2,
    Alt = 1 << 3,
    Alt_Shift = Alt | Shift,
    Ctrl_Shift = Ctrl | Shift,
    Ctrl_Alt = Ctrl | Alt,
    Ctrl_Alt_Shift = Ctrl | Alt | Shift,
    NumLock = 1 << 4,
    Super = 1 << 6,
    ScrollLock = 1 << 7,
    MousePressed = 1 << 8,
    HandledMask = 1 << 24,
    IgnoredMask = 1 << 25,
    Super2 = 1 << 26,
    Hyper = 1 << 27,
    Meta = 1 << 28,
    UsedMask = 0x5c001fff,
    SimpleMask = Ctrl_Alt_Shift | Super | Super2 | Hyper | Meta,
};
}

#endif
