/*
 * Copyright (C) 2016~2016 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
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

#include "action.h"
#include "menu.h"
#include "userinterfacemanager.h"

namespace fcitx {

class ActionPrivate : QPtrHolder<Action> {
public:
    ActionPrivate(Action *q) : QPtrHolder<Action>(q) {}
    std::string name_;
    bool checkable_ = false;
    bool separator_ = false;
    FCITX_DEFINE_SIGNAL_PRIVATE(Action, Update);
};

Action::Action() : d_ptr(std::make_unique<ActionPrivate>(this)) {}

Action::~Action() { destroy(); }

bool Action::isSeparator() const {
    FCITX_D();
    return d->separator_;
}

Action &Action::setSeparator(bool separator) {
    FCITX_D();
    d->separator_ = separator;
    return *this;
}

bool Action::registerAction(const std::string &name,
                            UserInterfaceManager *manager) {
    return manager->registerAction(name, this);
}

void Action::setName(const std::string &name) {
    FCITX_D();
    d->name_ = name;
}

Action &Action::setCheckable(bool checkable) {
    FCITX_D();
    d->checkable_ = checkable;
    return *this;
}

bool Action::isCheckable() const {
    FCITX_D();
    return d->checkable_;
}

void Action::setMenu(Menu *menu) {
    auto oldMenu = this->menu();
    if (oldMenu) {
        oldMenu->removeParent(this);
    }
    if (menu) {
        menu->addParent(this);
    }
}

Menu *Action::menu() {
    auto childList = childs();
    if (childList.size()) {
        return static_cast<Menu *>(childList.front());
    }
    return nullptr;
}

const std::string &Action::name() const {
    FCITX_D();
    return d->name_;
}
}
