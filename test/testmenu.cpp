/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "fcitx-utils/log.h"
#include "fcitx/menu.h"

using namespace fcitx;

int main() {
    {
        auto menu = std::make_unique<Menu>();
        {
            SimpleAction a;
            menu->addAction(&a);
            FCITX_ASSERT(menu->actions().size() == 1);
        }
        FCITX_ASSERT(menu->actions().size() == 0);
        SimpleAction a2;
        menu->addAction(&a2);
        FCITX_ASSERT(menu->actions().size() == 1);
        menu.reset();
    }
    {
        auto menu = std::make_unique<Menu>();
        SimpleAction a;
        a.setMenu(menu.get());
        menu.reset();
        FCITX_ASSERT(a.menu() == nullptr);
    }
    return 0;
}
