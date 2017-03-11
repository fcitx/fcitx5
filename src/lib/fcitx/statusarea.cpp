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
#include "inputcontext.h"

namespace fcitx {

class StatusAreaPrivate {
public:
    StatusAreaPrivate(InputContext *ic) : ic_(ic) {}
    Action separatorBeforeIM, separatorAfterIM;
    std::unordered_map<Action *, std::vector<ScopedConnection>> actions_;
    InputContext *ic_;
    void update() { ic_->updatePreedit(); }
};

StatusArea::StatusArea(InputContext *ic) : d_ptr(std::make_unique<StatusAreaPrivate>(ic)) { clear(); }

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
    d->actions_[action].emplace_back(action->connect<ObjectDestroyed>([this](void *p) {
        auto action = static_cast<Action *>(p);
        removeAction(action);
    }));
    d->actions_[action].emplace_back(action->connect<Action::Update>([this, d](InputContext *ic) {
        if (ic == d->ic_) {
            d->update();
        }
    }));
    d->update();
}

void StatusArea::removeAction(Action *action) {
    FCITX_D();
    removeChild(action);
    d->actions_.erase(action);
    d->update();
}

void StatusArea::clear() {
    FCITX_D();
    removeAllChild();
    addChild(&d->separatorBeforeIM);
    addChild(&d->separatorAfterIM);
}

std::vector<Action *> StatusArea::actions() {
    FCITX_D();
    std::vector<Action *> result;
    for (auto ele : childs()) {
        if (ele != &d->separatorBeforeIM && ele != &d->separatorAfterIM) {
            result.push_back(static_cast<Action *>(ele));
        }
    }
    return result;
}
}
