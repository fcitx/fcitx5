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

#include "menu.h"
#include <algorithm>

namespace fcitx {

class MenuPrivate : public QPtrHolder<Menu> {
public:
    MenuPrivate(Menu *q) : QPtrHolder<Menu>(q) {}
    std::unordered_map<Action *, ScopedConnection> actions_;
    FCITX_DEFINE_SIGNAL_PRIVATE(Menu, Update);
};

Menu::Menu() : d_ptr(std::make_unique<MenuPrivate>(this)) {}

Menu::~Menu() { destroy(); }

void Menu::addAction(Action *action) { return insertAction(nullptr, action); }

void Menu::insertAction(Action *before, Action *action) {
    FCITX_D();
    insertChild(before, action);
    ScopedConnection conn = action->connect<ObjectDestroyed>([this](void *p) {
        auto action = static_cast<Action *>(p);
        removeAction(action);
    });
    d->actions_.emplace(std::make_pair(action, std::move(conn)));
    emit<Update>();
}

void Menu::removeAction(Action *action) {
    FCITX_D();
    removeChild(action);
    d->actions_.erase(action);
    emit<Update>();
}

std::vector<Action *> Menu::actions() {
    std::vector<Action *> result;
    for (const auto &p : childs()) {
        result.push_back(static_cast<Action *>(p));
    }
    return result;
}
}
