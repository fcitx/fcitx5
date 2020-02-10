//
// Copyright (C) 2017~2017 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//
#include "emoji_public.h"
#include "testdir.h"
#include <fcitx-utils/log.h>
#include <fcitx/addonmanager.h>
#include <iostream>

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
