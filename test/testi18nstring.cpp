/*
 * SPDX-FileCopyrightText: 2015-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "fcitx-utils/i18nstring.h"
#include "fcitx-utils/log.h"

using namespace fcitx;

int main() {
    I18NString s;
    s.set("TEST1", "zh");
    s.set("TEST2", "zh_CN");
    s.set("TEST3");

    FCITX_ASSERT("TEST2" == s.match("zh_CN@whatever"));
    FCITX_ASSERT("TEST2" == s.match("zh_CN"));
    FCITX_ASSERT("TEST1" == s.match("zh_TW"));
    FCITX_ASSERT("TEST2" == s.match("zh_CN.UTF-8"));
    FCITX_ASSERT("TEST2" == s.match("zh_CN.utf8"));
    FCITX_ASSERT("TEST3" == s.match(""));
    FCITX_ASSERT("TEST3" == s.match("en"));

    return 0;
}
