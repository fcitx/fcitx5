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

#include "fcitx/action.h"
#include "fcitx/userinterfacemanager.h"
#include <cassert>

using namespace fcitx;

int main() {
    auto uiManager = std::make_unique<UserInterfaceManager>();
    {
        Action a;
        assert(uiManager->registerAction("test", &a));
        assert(uiManager->lookupAction("test") == &a);
        assert(a.name() == "test");
    }
    assert(uiManager->lookupAction("test") == nullptr);
    Action a2;
    assert(uiManager->registerAction("test", &a2));
    assert(a2.name() == "test");
    assert(!uiManager->registerAction("test", &a2));
    assert(!uiManager->registerAction("test2", &a2));
    assert(uiManager->lookupAction("test") == &a2);

    {
        Action a3;
        assert(!uiManager->registerAction("test", &a3));
        assert(uiManager->registerAction("test2", &a3));

        assert(uiManager->lookupAction("test2") == &a3);
        uiManager->unregisterAction(&a3);
        assert(uiManager->lookupAction("test2") == nullptr);
    }

    uiManager.reset();
    return 0;
}
