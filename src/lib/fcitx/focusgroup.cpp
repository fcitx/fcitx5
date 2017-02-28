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

#include "focusgroup_p.h"
#include "inputcontext.h"
#include "inputcontextmanager.h"
#include <cassert>

namespace fcitx {

FocusGroup::FocusGroup(const std::string &display, InputContextManager &manager)
    : d_ptr(std::make_unique<FocusGroupPrivate>(this, display, manager)) {
    manager.registerFocusGroup(*this);
}

FocusGroup::~FocusGroup() {
    FCITX_D();
    while (!d->ics_.empty()) {
        auto ic = *d->ics_.begin();
        ic->setFocusGroup(nullptr);
    }
    d->manager_.unregisterFocusGroup(*this);
}

void FocusGroup::setFocusedInputContext(InputContext *ic) {
    FCITX_D();
    assert(!ic || d->ics_.count(ic) > 0);
    if (d->focus_) {
        d->focus_->setHasFocus(false);
    }
    d->focus_ = ic;
    if (d->focus_) {
        d->focus_->setHasFocus(true);
    }
}

InputContext *FocusGroup::focusedInputContext() const {
    FCITX_D();
    return d->focus_;
}

void FocusGroup::addInputContext(InputContext *ic) {
    FCITX_D();
    auto iter = d->ics_.insert(ic);
    assert(iter.second);
}

void FocusGroup::removeInputContext(InputContext *ic) {
    FCITX_D();
    if (ic == d->focus_) {
        setFocusedInputContext(nullptr);
    }
    auto iter = d->ics_.find(ic);
    assert(iter != d->ics_.end());
    d->ics_.erase(ic);
}

const std::string &FocusGroup::display() const {
    FCITX_D();
    return d->display_;
}
}
