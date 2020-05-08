/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
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
} // namespace fcitx
