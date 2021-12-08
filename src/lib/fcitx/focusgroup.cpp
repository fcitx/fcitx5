/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include <cassert>
#include "focusgroup_p.h"
#include "inputcontext.h"
#include "inputcontextmanager.h"
#include "instance.h"

namespace fcitx {

FocusGroup::FocusGroup(const std::string &display, InputContextManager &manager)
    : d_ptr(std::make_unique<FocusGroupPrivate>(this, display, manager)) {
    manager.registerFocusGroup(*this);
}

FocusGroup::~FocusGroup() {
    FCITX_D();
    while (!d->ics_.empty()) {
        auto *ic = *d->ics_.begin();
        ic->setFocusGroup(nullptr);
    }
    d->manager_.unregisterFocusGroup(*this);
}

void FocusGroup::setFocusedInputContext(InputContext *ic) {
    FCITX_D();
    assert(!ic || d->ics_.count(ic) > 0);
    if (ic == d->focus_) {
        return;
    }
    if (d->focus_) {
        d->focus_->setHasFocus(false);
    }
    auto *oldFocus = d->focus_;
    d->focus_ = ic;
    if (d->focus_) {
        d->focus_->setHasFocus(true);
    }
    if (auto *instance = d->manager_.instance()) {
        instance->postEvent(
            FocusGroupFocusChangedEvent(this, oldFocus, d->focus_));
    }
}

InputContext *FocusGroup::focusedInputContext() const {
    FCITX_D();
    return d->focus_;
}

bool FocusGroup::foreach(const InputContextVisitor &visitor) {
    FCITX_D();
    for (auto *ic : d->ics_) {
        if (!visitor(ic)) {
            return false;
        }
    }
    return true;
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

size_t FocusGroup::size() const {
    FCITX_D();
    return d->ics_.size();
}
} // namespace fcitx
