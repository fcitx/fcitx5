//
// Copyright (C) 2016~2016 by CSSlayer
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

#include "fcitx-utils/log.h"
#include "fcitx-utils/metastring.h"
#include "fcitx/addoninstance.h"
#include "fcitx/addonmanager.h"
#include "addon/dummyaddon_public.h"
#include "testdir.h"

double f(int) { return 0; }

int main() {
    setenv("XDG_DATA_DIRS", FCITX5_SOURCE_DIR "/test/addon2", 1);
    setenv("FCITX_ADDON_DIRS", FCITX5_BINARY_DIR "/test/addon", 1);
    fcitx::AddonManager manager;
    manager.registerDefaultLoader(nullptr);
    manager.load();
    auto addon = manager.addon("dummyaddon");
    FCITX_ASSERT(addon);
    FCITX_ASSERT(6 ==
                 addon->callWithSignature<int(int)>("DummyAddon::addOne", 5));
    FCITX_ASSERT(6 ==
                 addon->callWithSignature<int(int)>("DummyAddon::addOne", 5.3));
    auto result =
        7 ==
        addon->callWithMetaString<fcitxMakeMetaString("DummyAddon::addOne")>(6);
    FCITX_ASSERT(result);
    auto result2 = 8 == addon->call<fcitx::IDummyAddon::addOne>(7);
    FCITX_ASSERT(result2);
    return 0;
}
