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
#include "fcitx-utils/dynamictrackableobject.h"
#include <algorithm>

namespace fcitx {

class MenuPrivate {
public:
    std::vector<std::pair<Action *, ScopedConnection>> actions_;
};

Menu::Menu() : d_ptr(std::make_unique<MenuPrivate>()) {}

Menu::~Menu() {}

void Menu::addAction(Action *action) { return insertAction(nullptr, action); }

void Menu::insertAction(Action *before, Action *action) {
    FCITX_D();
    auto iter = d->actions_.end();
    if (before) {
        iter = std::find_if(d->actions_.begin(), d->actions_.end(),
                            [before](const auto &t) { return (t.first == before); });
        if (iter == d->actions_.end()) {
            return;
        }
    }
    ScopedConnection conn = action->connect<ObjectDestroyed>([this](void *p) {
        auto action = static_cast<Action *>(p);
        removeAction(action);
    });
    d->actions_.emplace(iter, std::make_pair(action, std::move(conn)));
}

void Menu::removeAction(Action *action) {
    FCITX_D();
    auto iter =
        std::find_if(d->actions_.begin(), d->actions_.end(), [action](const auto &t) { return (t.first == action); });
    if (iter != d->actions_.end()) {
        d->actions_.erase(iter);
    }
}

std::vector<Action *> Menu::actions() {
    FCITX_D();
    std::vector<Action *> result;
    for (const auto &p : d->actions_) {
        result.push_back(p.first);
    }
    return result;
}
}
