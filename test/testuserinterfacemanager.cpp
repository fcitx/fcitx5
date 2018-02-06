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
#include "fcitx/action.h"
#include "fcitx/userinterfacemanager.h"

using namespace fcitx;

int main() {
    auto uiManager = std::make_unique<UserInterfaceManager>(nullptr);
    {
        SimpleAction a;
        FCITX_ASSERT(uiManager->registerAction("test", &a));
        FCITX_ASSERT(uiManager->lookupAction("test") == &a);
        FCITX_ASSERT(a.name() == "test");
    }
    FCITX_ASSERT(uiManager->lookupAction("test") == nullptr);
    SimpleAction a2;
    FCITX_ASSERT(uiManager->registerAction("test", &a2));
    FCITX_ASSERT(a2.name() == "test");
    FCITX_ASSERT(!uiManager->registerAction("test", &a2));
    FCITX_ASSERT(!uiManager->registerAction("test2", &a2));
    FCITX_ASSERT(uiManager->lookupAction("test") == &a2);

    {
        SimpleAction a3;
        FCITX_ASSERT(!uiManager->registerAction("test", &a3));
        FCITX_ASSERT(uiManager->registerAction("test2", &a3));

        FCITX_ASSERT(uiManager->lookupAction("test2") == &a3);
        uiManager->unregisterAction(&a3);
        FCITX_ASSERT(uiManager->lookupAction("test2") == nullptr);
    }

    uiManager.reset();
    return 0;
}
