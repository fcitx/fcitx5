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

#include "inputcontext.h"
#include "focusgroup.h"
#include "inputcontext_p.h"
#include "inputcontextmanager.h"
#include "instance.h"
#include <exception>
#include <iostream>

namespace fcitx {

InputContext::InputContext(InputContextManager &manager, const std::string &program)
    : d_ptr(std::make_unique<InputContextPrivate>(this, manager, program)) {
    manager.registerInputContext(*this);
    FCITX_D();
    d->emplaceEvent<InputContextCreatedEvent>(this);
}

InputContext::~InputContext() {
    FCITX_D();
    if (!d->destroyed_) {
        throw std::runtime_error("destroy() is not called, this should be a bug.");
    }
}

void InputContext::destroy() {
    FCITX_D();
    d->emplaceEvent<InputContextDestroyedEvent>(this);
    if (d->group_) {
        d->group_->removeInputContext(this);
    }
    d->manager_.unregisterInputContext(*this);
    d->destroyed_ = true;
}

const ICUUID &InputContext::uuid() const {
    FCITX_D();
    return d->uuid_;
}

const std::string &InputContext::program() const {
    FCITX_D();
    return d->program_;
}

std::string InputContext::display() const {
    FCITX_D();
    return d->group_ ? d->group_->display() : "";
}

InputContextProperty *InputContext::property(const std::string &name) {
    FCITX_D();
    auto iter = d->properties_.find(name);
    if (iter == d->properties_.end()) {
        return nullptr;
    }
    return iter->second.get();
}

void InputContext::updateProperty(const std::string &name) {
    FCITX_D();
    auto iter = d->properties_.find(name);
    if (iter == d->properties_.end() || !iter->second->needCopy()) {
        return;
    }
    d->manager_.propagateProperty(*this, name);
}

void InputContext::registerProperty(const std::string &name, InputContextProperty *property) {
    FCITX_D();
    d->properties_[name].reset(property);
}

void InputContext::unregisterProperty(const std::string &name) {
    FCITX_D();
    d->properties_.erase(name);
}

void InputContext::setCapabilityFlags(CapabilityFlags flags) {
    FCITX_D();
    if (d->capabilityFlags_ != flags) {
        d->capabilityFlags_ = flags;

        d->emplaceEvent<CapabilityChangedEvent>(this);
    }
}

CapabilityFlags InputContext::capabilityFlags() {
    FCITX_D();
    return d->capabilityFlags_;
}

void InputContext::setCursorRect(Rect rect) {
    FCITX_D();
    if (d->cursorRect_ != rect) {
        d->cursorRect_ = rect;
        d->emplaceEvent<CursorRectChangedEvent>(this);
    }
}

void InputContext::setFocusGroup(FocusGroup *group) {
    FCITX_D();
    focusOut();
    if (d->group_) {
        d->group_->removeInputContext(this);
    }
    d->group_ = group;
    if (d->group_) {
        d->group_->addInputContext(this);
    }
}

FocusGroup *InputContext::focusGroup() const {
    FCITX_D();
    return d->group_;
}

void InputContext::focusIn() {
    FCITX_D();
    if (d->group_) {
        d->group_->setFocusedInputContext(this);
    } else {
        setHasFocus(true);
    }
}

void InputContext::focusOut() {
    FCITX_D();
    if (d->group_) {
        if (d->group_->focusedInputContext() == this) {
            d->group_->setFocusedInputContext(nullptr);
        }
    } else {
        setHasFocus(false);
    }
}

bool InputContext::hasFocus() const {
    FCITX_D();
    return d->hasFocus_;
}

void InputContext::setHasFocus(bool hasFocus) {
    FCITX_D();
    if (hasFocus == d->hasFocus_) {
        return;
    }
    d->hasFocus_ = hasFocus;
    // trigger event
    if (d->hasFocus_) {
        d->emplaceEvent<FocusInEvent>(this);
    } else {
        d->emplaceEvent<FocusOutEvent>(this);
    }
}

bool InputContext::keyEvent(KeyEvent &event) {
    FCITX_D();
    return d->postEvent(event);
}

void InputContext::reset() {
    FCITX_D();
    d->emplaceEvent<ResetEvent>(this);
}

SurroundingText &InputContext::surroundingText() {
    FCITX_D();
    return d->surroundingText_;
}

const SurroundingText &InputContext::surroundingText() const {
    FCITX_D();
    return d->surroundingText_;
}

void InputContext::updateSurroundingText() {
    FCITX_D();
    d->emplaceEvent<SurroundingTextUpdatedEvent>(this);
}

void InputContext::commitString(const std::string &text) {
    FCITX_D();
    CommitStringEvent event(text, this);
    if (!d->postEvent(event)) {
        commitStringImpl(event.text());
    }
}

void InputContext::deleteSurroundingText(int offset, unsigned int size) { deleteSurroundingTextImpl(offset, size); }

void InputContext::forwardKey(const Key &rawKey, bool isRelease, int keyCode, int time) {
    FCITX_D();
    ForwardKeyEvent event(this, rawKey, isRelease, keyCode, time);
    if (!d->postEvent(event)) {
        forwardKeyImpl(event);
    }
}

void InputContext::updatePreedit() {
    FCITX_D();
    UpdatePreeditEvent event(this);
    if (!d->postEvent(event)) {
        updatePreeditImpl();
    }
}

InputPanel &InputContext::inputPanel() {
    FCITX_D();
    return d->inputPanel_;
}
}
