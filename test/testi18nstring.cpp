/*
 * Copyright (C) 2015~2015 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
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
#include <cassert>

using namespace fcitx;

int main() {
    I18NString s;
    s.set("TEST1", "zh");
    s.set("TEST2", "zh_CN");
    s.set("TEST3");

    assert("TEST2" == s.match("zh_CN@whatever"));
    assert("TEST2" == s.match("zh_CN"));
    assert("TEST1" == s.match("zh_TW"));
    assert("TEST2" == s.match("zh_CN.UTF-8"));
    assert("TEST2" == s.match("zh_CN.utf8"));
    assert("TEST3" == s.match(""));
    assert("TEST3" == s.match("en"));

    return 0;
}
