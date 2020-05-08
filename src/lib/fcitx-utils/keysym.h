/*
 * SPDX-FileCopyrightText: 2012-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
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
    Mod1 = Alt,
    Alt_Shift = Alt | Shift,
    Ctrl_Shift = Ctrl | Shift,
    Ctrl_Alt = Ctrl | Alt,
    Ctrl_Alt_Shift = Ctrl | Alt | Shift,
    NumLock = 1 << 4,
    Mod2 = NumLock,
    Mod3 = 1 << 5,
    Super = 1 << 6,
    Mod4 = Super,
    Mod5 = 1 << 7,
    MousePressed = 1 << 8,
    HandledMask = 1 << 24,
    IgnoredMask = 1 << 25,
    Super2 = 1 << 26,
    Hyper = 1 << 27,
    Meta = 1 << 28,
    UsedMask = 0x5c001fff,
    SimpleMask = Ctrl_Alt_Shift | Super | Super2 | Hyper | Meta,
};
} // namespace fcitx

#endif
