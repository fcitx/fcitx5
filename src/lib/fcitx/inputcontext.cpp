//
// Copyright (C) 2016~2016 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//

#include "inputcontext.h"
#include "fcitx-utils/event.h"
#include "focusgroup.h"
#include "inputcontext_p.h"
#include "inputcontextmanager.h"
#include "instance.h"
#include <cassert>
#include <exception>

namespace fcitx {

static_assert(sizeof(uuid_t) == 16, "uuid size mismatch");

InputContext::InputContext(InputContextManager &manager,
                           const std::string &program)
    : d_ptr(std::make_unique<InputContextPrivate>(this, manager, program)) {
    manager.registerInputContext(*this);
}

InputContext::~InputContext() { assert(d_ptr->destroyed_); }

void InputContext::created() {
    FCITX_D();
    d->emplaceEvent<InputContextCreatedEvent>(this);
}

void InputContext::destroy() {
    FCITX_D();
    assert(!d->destroyed_);
    if (d->group_) {
        d->group_->removeInputContext(this);
    }
    d->emplaceEvent<InputContextDestroyedEvent>(this);
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

const Rect &InputContext::cursorRect() const {
    FCITX_D();
    return d->cursorRect_;
}

InputContextProperty *InputContext::property(const std::string &name) {
    FCITX_D();
    auto factory = d->manager_.factoryForName(name);
    if (!factory) {
        return nullptr;
    }
    return d->manager_.property(*this, factory);
}

InputContextProperty *
InputContext::property(const InputContextPropertyFactory *factory) {
    FCITX_D();
    return d->manager_.property(*this, factory);
}

void InputContext::updateProperty(const std::string &name) {
    FCITX_D();
    auto factory = d->manager_.factoryForName(name);
    if (!factory) {
        return;
    }
    auto property = d->manager_.property(*this, factory);
    if (!property->needCopy()) {
        return;
    }
    d->manager_.propagateProperty(*this, factory);
}

void InputContext::setCapabilityFlags(CapabilityFlags flags) {
    FCITX_D();
    if (d->capabilityFlags_ != flags) {
        d->capabilityFlags_ = flags;

        d->emplaceEvent<CapabilityChangedEvent>(this);
    }
}

CapabilityFlags InputContext::capabilityFlags() const {
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
    d->manager_.notifyFocus(*this, d->hasFocus_);
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

void InputContext::reset(ResetReason reason) {
    FCITX_D();
    d->emplaceEvent<ResetEvent>(reason, this);
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
        if (auto instance = d->manager_.instance()) {
            auto newString = instance->commitFilter(this, event.text());
            commitStringImpl(newString);
        } else {
            commitStringImpl(event.text());
        }
    }
}

void InputContext::deleteSurroundingText(int offset, unsigned int size) {
    deleteSurroundingTextImpl(offset, size);
}

void InputContext::forwardKey(const Key &rawKey, bool isRelease, int time) {
    FCITX_D();
    ForwardKeyEvent event(this, rawKey, isRelease, time);
    if (!d->postEvent(event)) {
        forwardKeyImpl(event);
    }
}

void InputContext::updatePreedit() {
    FCITX_D();
    if (!d->emplaceEvent<UpdatePreeditEvent>(this)) {
        updatePreeditImpl();
    }
}

void InputContext::updateUserInterface(UserInterfaceComponent component,
                                       bool immediate) {
    FCITX_D();
    d->emplaceEvent<InputContextUpdateUIEvent>(component, this, immediate);
}

InputPanel &InputContext::inputPanel() {
    FCITX_D();
    return d->inputPanel_;
}

StatusArea &InputContext::statusArea() {
    FCITX_D();
    return d->statusArea_;
}
} // namespace fcitx
