/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

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
