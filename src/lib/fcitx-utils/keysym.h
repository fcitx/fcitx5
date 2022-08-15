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
    NoState = 0,
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
    Hyper = 1 << 5,
    Mod3 = Hyper,
    Super = 1 << 6,
    Mod4 = Super,
    Mod5 = 1 << 7,
    MousePressed = 1 << 8,
    HandledMask = 1 << 24,
    IgnoredMask = 1 << 25,
    Super2 = 1 << 26, // Gtk virtual Super
    Hyper2 = 1 << 27, // Gtk virtual Hyper
    Meta = 1 << 28,
    /*
     * Key state that used internally for virtual key board.
     *
     * @since 5.1.0
     */
    Virtual = 1u << 29,
    /**
     * Whether a Key Press is from key repetition.
     *
     * @since 5.0.4
     */
    Repeat = 1u << 31,
    UsedMask = 0x5c001fff,
    SimpleMask = Ctrl_Alt_Shift | Super | Super2 | Hyper | Meta,
};
} // namespace fcitx

#endif
