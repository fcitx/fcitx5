/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include <fcntl.h>
#include <iostream>
#include <fcitx-utils/log.h>
#include <fcitx-utils/standardpath.h>
#include <fcitx-utils/testing.h>
#include <fcitx/addonmanager.h>
#include "emoji_public.h"
#include "testdir.h"

using namespace fcitx;

int main() {
    setupTestingEnvironment(
        FCITX5_BINARY_DIR, {"src/modules/emoji"},
        {"test", "src/modules", FCITX5_SOURCE_DIR "/src/modules"});
    AddonManager manager(FCITX5_BINARY_DIR "/src/modules/emoji");
    manager.registerDefaultLoader(nullptr);
    manager.load();
    auto *emoji = manager.addon("emoji", true);
    FCITX_ASSERT(emoji);
    auto emojis = emoji->call<IEmoji::query>("zh", "大笑", false);
    FCITX_ASSERT(std::find(emojis.begin(), emojis.end(), "\xf0\x9f\x98\x84") !=
                 emojis.end())
        << emojis;
    emojis = emoji->call<IEmoji::query>("en", "eggplant", false);
    FCITX_ASSERT(std::find(emojis.begin(), emojis.end(), "\xf0\x9f\x8d\x86") !=
                 emojis.end())
        << emojis;

    auto files = StandardPath::global().multiOpen(StandardPath::Type::PkgData,
                                                  "emoji/data", O_RDONLY,
                                                  filter::Suffix(".dict"));
    // Check if all languages are loadable.
    for (const auto &[name, __] : files) {
        std::string lang = name.substr(0, name.size() - 5);
        FCITX_ASSERT(emoji->call<IEmoji::check>(lang, false))
            << "Failed to load " << lang;
    }

    return 0;
}
