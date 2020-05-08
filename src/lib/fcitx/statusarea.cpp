/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "statusarea.h"
#include "action.h"
#include "inputcontext.h"

namespace fcitx {

class StatusAreaPrivate {
public:
    StatusAreaPrivate(InputContext *ic) : ic_(ic) {}
    SimpleAction separatorBeforeIM, separatorAfterIM;
    std::unordered_map<Action *, std::vector<ScopedConnection>> actions_;
    InputContext *ic_;
    void update() {
        ic_->updateUserInterface(UserInterfaceComponent::StatusArea);
    }
};

StatusArea::StatusArea(InputContext *ic)
    : d_ptr(std::make_unique<StatusAreaPrivate>(ic)) {
    clear();
}

StatusArea::~StatusArea() {}

void StatusArea::addAction(StatusGroup group, Action *action) {
    FCITX_D();
    if (isChild(action)) {
        removeChild(action);
        d->actions_.erase(action);
    }
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
    d->actions_[action].emplace_back(
        action->connect<ObjectDestroyed>([this, d](void *p) {
            auto action = static_cast<Action *>(p);
            removeAction(action);
            d->update();
        }));
    d->actions_[action].emplace_back(
        action->connect<Action::Update>([d](InputContext *ic) {
            if (ic == d->ic_) {
                d->update();
            }
        }));
    d->update();
}

void StatusArea::removeAction(Action *action) {
    FCITX_D();
    if (isChild(action)) {
        removeChild(action);
        d->actions_.erase(action);
        d->update();
    }
}

void StatusArea::clear() {
    FCITX_D();
    removeAllChild();
    addChild(&d->separatorBeforeIM);
    addChild(&d->separatorAfterIM);
}

void StatusArea::clearGroup(StatusGroup group) {
    for (auto action : actions(group)) {
        removeAction(action);
    }
}
std::vector<Action *> StatusArea::allActions() const {
    FCITX_D();
    std::vector<Action *> result;
    for (auto ele : childs()) {
        if (ele == &d->separatorBeforeIM || ele == &d->separatorAfterIM) {
            continue;
        }
        result.push_back(static_cast<Action *>(ele));
    }
    return result;
}

std::vector<Action *> StatusArea::actions(StatusGroup group) const {
    FCITX_D();
    std::vector<Action *> result;
    switch (group) {
    case StatusGroup::BeforeInputMethod:
        for (auto ele : childs()) {
            if (ele == &d->separatorBeforeIM) {
                break;
            }
            result.push_back(static_cast<Action *>(ele));
        }
        break;
    case StatusGroup::InputMethod: {
        bool push = false;
        for (auto ele : childs()) {
            if (ele == &d->separatorBeforeIM) {
                push = true;
                continue;
            }
            if (ele == &d->separatorAfterIM) {
                break;
            }
            if (push) {
                result.push_back(static_cast<Action *>(ele));
            }
        }
        break;
    }
    case StatusGroup::AfterInputMethod: {
        bool push = false;
        for (auto ele : childs()) {
            if (ele == &d->separatorAfterIM) {
                push = true;
                continue;
            }
            if (push) {
                result.push_back(static_cast<Action *>(ele));
            }
        }
        break;
    }
    }
    return result;
}
} // namespace fcitx
