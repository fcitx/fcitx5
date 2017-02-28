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

#include "action.h"
#include "fcitx-utils/dynamictrackableobject.h"
#include "menu.h"
#include "userinterfacemanager.h"

namespace fcitx {

class ActionPrivate {
public:
    ActionPrivate(Action *q) : ActionActivatedAdaptor(q), ActionUpdateAdaptor(q) {}
    std::string name_;
    std::string icon_;
    std::string text_;
    bool checked_ = false;
    bool checkable_ = false;
    bool enabled_ = true;
    bool visible_ = true;
    bool separator_ = false;
    ScopedConnection connection_;
    FCITX_DEFINE_SIGNAL_PRIVATE(Action, Activated);
    FCITX_DEFINE_SIGNAL_PRIVATE(Action, Update);
};

Action::Action() : d_ptr(std::make_unique<ActionPrivate>(this)) {}

Action::~Action() { destroy(); }

void Action::activate() {
    FCITX_D();
    if (!d->enabled_) {
        return;
    }
    emit<Action::Activated>();
}

bool Action::isSeparator() const {
    FCITX_D();
    return d->separator_;
}

Action &Action::setSeparator(bool separator) {
    FCITX_D();
    d->separator_ = separator;
    return *this;
}

bool Action::registerAction(const std::string &name, UserInterfaceManager *manager) {
    return manager->registerAction(name, this);
}

void Action::setName(const std::string &name) {
    FCITX_D();
    d->name_ = name;
}

Action &Action::setIcon(const std::string &icon) {
    FCITX_D();
    d->icon_ = icon;
    return *this;
}

const std::string &Action::icon() const {
    FCITX_D();
    return d->icon_;
}

Action &Action::setText(const std::string &text) {
    FCITX_D();
    d->text_ = text;
    return *this;
}

const std::string &Action::text() const {
    FCITX_D();
    return d->text_;
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

Action &Action::setChecked(bool checked) {
    FCITX_D();
    d->checked_ = checked;
    return *this;
}

bool Action::isChecked() const {
    FCITX_D();
    return d->checked_;
}

Action &Action::setEnabled(bool enabled) {
    FCITX_D();
    d->enabled_ = enabled;
    return *this;
}

bool Action::isEnabled() const {
    FCITX_D();
    return d->enabled_;
}

void Action::setMenu(Menu *menu) {
    FCITX_D();
    auto oldMenu = this->menu();
    if (oldMenu) {
        oldMenu->removeParent(this);
    }
    if (menu) {
        menu->addParent(this);
        d->connection_ = menu->connect<ObjectDestroyed>([this](void *) { emit<Action::Update>(); });
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
