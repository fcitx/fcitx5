/*
 * Copyright (C) 2015~2015 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING. If not,
 * see <http://www.gnu.org/licenses/>.
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
