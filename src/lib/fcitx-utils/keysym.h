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
#include <fcitx-utils/keysymgen.h> // IWYU pragma: export
#include <fcitx-utils/macros.h>

namespace fcitx {
/// \brief KeyState to represent modifier keys.
enum class KeyState : uint32_t {
    NoState = 0,
    Shift = 1U << 0,
    CapsLock = 1U << 1,
    Ctrl = 1U << 2,
    Alt = 1U << 3,
    Mod1 = Alt,
    Alt_Shift = Alt | Shift,
    Ctrl_Shift = Ctrl | Shift,
    Ctrl_Alt = Ctrl | Alt,
    Ctrl_Alt_Shift = Ctrl | Alt | Shift,
    NumLock = 1U << 4,
    Mod2 = NumLock,
    Hyper = 1U << 5,
    Mod3 = Hyper,
    Super = 1U << 6,
    Mod4 = Super,
    Mod5 = 1U << 7,
    MousePressed = 1U << 8,
    HandledMask = 1U << 24,
    IgnoredMask = 1U << 25,
    Super2 = 1U << 26, // Gtk virtual Super
    Hyper2 = 1U << 27, // Gtk virtual Hyper
    Meta = 1U << 28,
    /*
     * Key state that used internally for virtual key board.
     *
     * @since 5.1.0
     */
    Virtual = 1U << 29,
    /**
     * Whether a Key Press is from key repetition.
     *
     * @since 5.0.4
     */
    Repeat = 1U << 31,
    UsedMask = 0x5c001fff,
    SimpleMask = Ctrl_Alt_Shift | Super | Super2 | Hyper | Meta,
};
} // namespace fcitx

#endif
