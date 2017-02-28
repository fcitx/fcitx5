/*
 * Copyright (C) 2017~2017 by CSSlayer
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

#include "statusarea.h"
#include "action.h"

namespace fcitx {

class StatusAreaPrivate {
public:
    StatusAreaPrivate(StatusArea *q) : StatusAreaUpdateAdaptor(q) {}
    Action separatorBeforeIM, separatorAfterIM;
    std::unordered_map<Action *, ScopedConnection> actions_;
    FCITX_DEFINE_SIGNAL_PRIVATE(StatusArea, Update);
};

StatusArea::StatusArea() : d_ptr(std::make_unique<StatusAreaPrivate>(this)) {
    FCITX_D();
    addChild(&d->separatorBeforeIM);
    addChild(&d->separatorAfterIM);
}

StatusArea::~StatusArea() {}

void StatusArea::addAction(StatusGroup group, Action *action) {
    FCITX_D();
    switch (group) {
    case StatusGroup::BeforeInputMethod:
        insertChild(&d->separatorBeforeIM, action);
        break;
    case StatusGroup::InputMethod:
        insertChild(&d->separatorAfterIM, action);
        break;
    case StatusGroup::AfterInputMethod:
        addChild(action);
        break;
    }
    ScopedConnection conn = action->connect<ObjectDestroyed>([this](void *p) {
        auto action = static_cast<Action *>(p);
        removeAction(action);
    });
    d->actions_.emplace(std::make_pair(action, std::move(conn)));
    emit<StatusArea::Update>();
}

void StatusArea::removeAction(Action *action) {
    removeChild(action);
    emit<StatusArea::Update>();
}
}
