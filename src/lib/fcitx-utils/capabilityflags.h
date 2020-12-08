/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_CAPABILITYFLAGS_H_
#define _FCITX_UTILS_CAPABILITYFLAGS_H_

#include <cstdint>
#include <fcitx-utils/flags.h>

/// \addtogroup FcitxUtils
/// \{
/// \file
/// \brief Enum type for input context capability.

namespace fcitx {

/// \brief Input context CapabilityFlags.
enum class CapabilityFlag : uint64_t {
    NoFlag = 0,
    ClientSideUI = (1 << 0),
    Preedit = (1 << 1),
    ClientSideControlState = (1 << 2),
    Password = (1 << 3),
    FormattedPreedit = (1 << 4),
    ClientUnfocusCommit = (1 << 5),
    SurroundingText = (1 << 6),
    Email = (1 << 7),
    Digit = (1 << 8),
    Uppercase = (1 << 9),
    Lowercase = (1 << 10),
    NoAutoUpperCase = (1 << 11),
    Url = (1 << 12),
    Dialable = (1 << 13),
    Number = (1 << 14),
    NoOnScreenKeyboard = (1 << 15),
    SpellCheck = (1 << 16),
    NoSpellCheck = (1 << 17),
    WordCompletion = (1 << 18),
    UppercaseWords = (1 << 19),
    UppwercaseSentences = (1 << 20),
    Alpha = (1 << 21),
    Name = (1 << 22),
    GetIMInfoOnFocus = (1 << 23),
    RelativeRect = (1 << 24),
    // 25 ~ 31 are reserved for fcitx 4 compatibility.

    // New addition in fcitx 5.
    Terminal = (1ull << 32),
    Date = (1ull << 33),
    Time = (1ull << 34),
    Multiline = (1ull << 35),
    Sensitive = (1ull << 36),
    KeyEventOrderFix = (1ull << 37),
    /**
     * Whether client will set KeyState::Repeat on the key event.
     *
     * @see KeyState::Repeat
     * @since 5.0.4
     */
    ReportKeyRepeat = (1ull << 38),

    PasswordOrSensitive = Password | Sensitive
};

typedef Flags<CapabilityFlag> CapabilityFlags;
} // namespace fcitx

#endif // _FCITX_UTILS_CAPABILITYFLAGS_H_
