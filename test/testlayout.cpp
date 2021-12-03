/*
 * SPDX-FileCopyrightText: 2021~2021 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "fcitx-config/rawconfig.h"
#include "fcitx/virtualkeyboard/layout.h"

using namespace fcitx;

int main() {
    auto layout = virtualkeyboard::Layout::standard26Key();
    RawConfig layoutConfig;
    layout->writeToRawConfig(layoutConfig);
    FCITX_INFO() << layoutConfig;

    auto keymap = virtualkeyboard::Keymap::qwerty();
    RawConfig keymapConfig;
    keymap.writeToRawConfig(keymapConfig);
    FCITX_INFO() << keymapConfig;

    return 0;
}
