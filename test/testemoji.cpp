/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include <iostream>
#include <fcitx-utils/log.h>
#include <fcitx/addonmanager.h>
#include "emoji_public.h"
#include "testdir.h"

int main() {
    setenv("FCITX_ADDON_DIRS", FCITX5_BINARY_DIR "/src/modules/emoji", 1);
    setenv("FCITX_DATA_DIRS",
           FCITX5_BINARY_DIR "/src/modules:" FCITX5_SOURCE_DIR "/src/modules",
           1);
    fcitx::AddonManager manager(FCITX5_BINARY_DIR "/src/modules/emoji");
    manager.registerDefaultLoader(nullptr);
    manager.load();
    auto emoji = manager.addon("emoji", true);
    FCITX_ASSERT(emoji);
    auto emojis = emoji->call<fcitx::IEmoji::query>("zh", "大笑", false);
    FCITX_ASSERT(std::find(emojis.begin(), emojis.end(), "\xf0\x9f\x98\x84") !=
                 emojis.end())
        << emojis;
    emojis = emoji->call<fcitx::IEmoji::query>("en", "eggplant", false);
    FCITX_ASSERT(std::find(emojis.begin(), emojis.end(), "\xf0\x9f\x8d\x86") !=
                 emojis.end())
        << emojis;
    return 0;
}
