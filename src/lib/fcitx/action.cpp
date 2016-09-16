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

namespace fcitx {

class ActionPrivate {
public:
    ActionPrivate(const std::string &name_) : name(name_) {}
    std::string name;
    std::string icon;
    std::string text;
    bool checked = false;
    bool checkable = false;
    bool enabled = true;
    std::function<void()> callback;
};

Action::Action(const std::string &name) : d_ptr(std::make_unique<ActionPrivate>(name)) {
    // actionManager->registerAction(this);
}

Action::~Action() {
    // actionManager->unregisterAction(this);
}

void Action::activate() {}

Action &Action::setIcon(const std::string &icon) {
    FCITX_D();
    d->icon = icon;
    return *this;
}

const std::string &Action::icon() const {
    FCITX_D();
    return d->icon;
}

Action &Action::setText(const std::string &text) {
    FCITX_D();
    d->text = text;
    return *this;
}

const std::string &Action::text() const {
    FCITX_D();
    return d->text;
}

Action &Action::setCheckable(bool checkable) {
    FCITX_D();
    d->checkable = checkable;
    return *this;
}

bool Action::isCheckable() const {
    FCITX_D();
    return d->checkable;
}

Action &Action::setChecked(bool checked) {
    FCITX_D();
    d->checked = checked;
    return *this;
}

bool Action::isChecked() const {
    FCITX_D();
    return d->checked;
}

Action &Action::setEnabled(bool enabled) {
    FCITX_D();
    d->enabled = enabled;
    return *this;
}

bool Action::isEnabled() const {
    FCITX_D();
    return d->enabled;
}

void Action::setCallack(std::function<void()> callback) {
    FCITX_D();
    d->callback = callback;
}

const std::string &Action::name() const {
    FCITX_D();
    return d->name;
}
}
