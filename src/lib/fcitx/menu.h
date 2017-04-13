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
#ifndef _FCITX_MENU_H_
#define _FCITX_MENU_H_

#include "fcitxcore_export.h"
#include <fcitx-utils/macros.h>
#include <fcitx/action.h>
#include <fcitx-utils/element.h>
#include <memory>

namespace fcitx {

class MenuPrivate;

class FCITXCORE_EXPORT Menu : public Element {
public:
    friend class Action;
    Menu();
    virtual ~Menu();

    void addAction(Action *action);
    void removeAction(Action *action);
    void insertAction(Action *before, Action *action);
    std::vector<Action *> actions();

    FCITX_DECLARE_SIGNAL(Menu, Update, void());

private:
    std::unique_ptr<MenuPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(Menu);
};
}

#endif // _FCITX_MENU_H_
