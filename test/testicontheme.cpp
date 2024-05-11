/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "fcitx-utils/log.h"
#include "fcitx/icontheme.h"

using namespace fcitx;

int main() {
    IconTheme theme("breeze");
    FCITX_INFO() << IconTheme::defaultIconThemeName();
    FCITX_INFO() << theme.name().match();
    FCITX_INFO() << theme.comment().match();
    FCITX_ASSERT(IconTheme::iconName("input-keyboard", false) ==
                 "input-keyboard");
    FCITX_ASSERT(IconTheme::iconName("input-keyboard", true) ==
                 "input-keyboard");
    FCITX_ASSERT(IconTheme::iconName("fcitx-pinyin", false) == "fcitx-pinyin");
    FCITX_ASSERT(IconTheme::iconName("fcitx-pinyin", true) ==
                 "org.fcitx.Fcitx5.fcitx-pinyin");
    FCITX_ASSERT(IconTheme::iconName("fcitx_mozc", true) ==
                 "org.fcitx.Fcitx5.fcitx_mozc");
    FCITX_ASSERT(IconTheme::iconName("fcitx", true) == "org.fcitx.Fcitx5");

    for (const auto &inheritTheme : theme.inherits()) {
        FCITX_INFO() << inheritTheme.name().match();
    }
#if 0
    for (auto &directory : theme.directories()) {
        FCITX_INFO() << directory.name();
        FCITX_INFO() << directory.size();
        FCITX_INFO() << directory.scale();
        FCITX_INFO() << directory.context();
        FCITX_INFO() << to_string(directory.type());
        FCITX_INFO() << directory.maxSize();
        FCITX_INFO() << directory.minSize();
        FCITX_INFO() << directory.threshold();
    }
#endif

    FCITX_INFO() << theme.findIcon("fcitx-pinyin", 32, 1);

    return 0;
}
