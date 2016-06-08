/*
 * Copyright (C) 2016~2016 by CSSlayer
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

#include <cassert>
#include "fcitx/addonmanager.h"
double f(int) {
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        return 1;
    }
    setenv("XDG_DATA_DIRS", argv[1], 1);
    setenv("FCITX_ADDON_DIRS", argv[2], 1);
    fcitx::AddonManager manager;
    manager.registerDefaultLoader(nullptr);
    manager.load();
    auto addon = manager.addon("dummyaddon");
    assert(addon);
    assert(6 == addon->call<int(int)>("addOne", 5));
    assert(6 == addon->call<int(int)>("addOne", 5.3));
    return 0;
}
