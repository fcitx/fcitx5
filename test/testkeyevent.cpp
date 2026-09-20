/*
 * SPDX-FileCopyrightText: 2026-2026 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "fcitx-utils/key.h"
#include "fcitx-utils/keysym.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/macros.h"
#include "fcitx/event.h"

int main() {
    using namespace fcitx;

    KeyEvent keyEvent(nullptr, Key("Shift+exclam"));
    keyEvent.setKey(Key("b"));

    FCITX_ASSERT(keyEvent.check(Key("b")));
    FCITX_ASSERT(!keyEvent.check(Key("exclam")));
    FCITX_ASSERT(!keyEvent.check(Key("b"),
                                 KeyEventMatchingMode::MatchNormalizedOrigKey));
    FCITX_ASSERT(
        keyEvent.check(Key("Shift+exclam"), KeyEventMatchingMode::MatchRawKey));
    FCITX_ASSERT(keyEvent.check(Key("Shift+exclam"),
                                KeyEventMatchingMode::MatchOrigKey));
    FCITX_ASSERT(keyEvent.check(Key("exclam"),
                                KeyEventMatchingMode::MatchNormalizedOrigKey));
    FCITX_ASSERT(
        keyEvent.check(Key("b"), KeyEventMatchingMode::MatchAllNormalizedKeys));
    FCITX_ASSERT(keyEvent.check(Key("exclam"),
                                KeyEventMatchingMode::MatchAllNormalizedKeys));

    const KeyList keys = {Key("x"), Key("exclam")};
    FCITX_ASSERT(keyEvent.checkKeyList(
        keys, KeyEventMatchingMode::MatchNormalizedOrigKey));

    KeyEvent keyCodeEvent(nullptr, Key(FcitxKey_a, KeyState::Ctrl, 42));
    keyCodeEvent.setRawKey(Key(FcitxKey_b, KeyState::Alt, 43));

    const auto originalKey = Key::fromKeyCode(42, KeyState::Ctrl);
    const auto rawKey = Key::fromKeyCode(43, KeyState::Alt);
    FCITX_ASSERT(keyCodeEvent.check(rawKey));
    FCITX_ASSERT(!keyCodeEvent.check(originalKey));
    FCITX_ASSERT(keyCodeEvent.check(rawKey, KeyEventMatchingMode::MatchRawKey));
    FCITX_ASSERT(
        keyCodeEvent.check(originalKey, KeyEventMatchingMode::MatchOrigKey));
    FCITX_ASSERT(keyCodeEvent.check(
        originalKey, KeyEventMatchingMode::MatchNormalizedOrigKey));

    return 0;
}
