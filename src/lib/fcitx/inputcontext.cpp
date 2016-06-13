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

#include <string.h>
#include "inputcontext.h"
#include "inputcontext_p.h"
#include "inputcontextmanager.h"
#include "focusgroup.h"
#include "instance.h"

namespace fcitx {

InputContext::InputContext(InputContextManager &manager) : d_ptr(std::make_unique<InputContextPrivate>(this, manager)) {
    manager.registerInputContext(*this);
    if (manager.instance()) {
        manager.instance()->postEvent(InputContextCreatedEvent(this));
    }
}

InputContext::~InputContext() {
    FCITX_D();
    if (d->manager.instance()) {
        d->manager.instance()->postEvent(InputContextDestroyedEvent(this));
    }
    if (d->group) {
        d->group->removeInputContext(this);
    }
    d->manager.unregisterInputContext(*this);
}

ICUUID InputContext::uuid() {
    FCITX_D();
    return d->uuid;
}

void InputContext::setCapabilityFlags(CapabilityFlags flags) {
    FCITX_D();
    if (d->capabilityFlags != flags) {
        d->capabilityFlags = flags;

        if (d->manager.instance()) {
            d->manager.instance()->postEvent(CapabilityChangedEvent(this));
        }
    }
}

CapabilityFlags InputContext::capabilityFlags() {
    FCITX_D();
    return d->capabilityFlags;
}

void InputContext::setCursorRect(Rect rect) {
    FCITX_D();
    if (d->cursorRect != rect) {
        d->cursorRect = rect;
        if (d->manager.instance()) {
            d->manager.instance()->postEvent(CursorRectChangedEvent(this));
        }
    }
}

void InputContext::setFocusGroup(FocusGroup *group) {
    FCITX_D();
    focusOut();
    if (d->group) {
        d->group->removeInputContext(this);
    }
    d->group = group;
    if (d->group) {
        d->group->addInputContext(this);
    }
}

FocusGroup *InputContext::focusGroup() const {
    FCITX_D();
    return d->group;
}

void InputContext::focusIn() {
    FCITX_D();
    if (d->group) {
        if (focusGroupType() == FocusGroupType::Global) {
            d->manager.focusOutNonGlobal();
        } else {
            d->manager.globalFocusGroup().setFocusedInputContext(nullptr);
        }
        d->group->setFocusedInputContext(this);
    } else {
        setHasFocus(true);
    }
}

void InputContext::focusOut() {
    FCITX_D();
    if (d->group) {
        if (d->group->focusedInputContext() == this) {
            d->group->setFocusedInputContext(nullptr);
        }
    } else {
        setHasFocus(false);
    }
}

bool InputContext::hasFocus() const {
    FCITX_D();
    return d->hasFocus;
}

void InputContext::setHasFocus(bool hasFocus) {
    FCITX_D();
    if (hasFocus != d->hasFocus) {
        d->hasFocus = hasFocus;
        // trigger event
        if (d->hasFocus) {
            d->manager.instance()->postEvent(FocusInEventEvent(this));
        } else {
            d->manager.instance()->postEvent(FocusOutEventEvent(this));
        }
    }
}

bool InputContext::keyEvent(KeyEvent &event) {
    FCITX_D();
    auto instance = d->manager.instance();
    if (!instance) {
        return false;
    }
    return instance->postEvent(event);
}

void InputContext::reset() {}

FocusGroupType InputContext::focusGroupType() const {
    FCITX_D();
    if (d->group) {
        return d->group == &d->manager.globalFocusGroup() ? FocusGroupType::Global : FocusGroupType::Local;
    }
    return FocusGroupType::Independent;
}

SurroundingText &InputContext::surroundingText() {
    FCITX_D();
    return d->surroundingText;
}

const SurroundingText &InputContext::surroundingText() const {
    FCITX_D();
    return d->surroundingText;
}

Text &InputContext::preedit() {
    FCITX_D();
    return d->preedit;
}

const Text &InputContext::preedit() const {
    FCITX_D();
    return d->preedit;
}

Text &InputContext::clientPreedit() {
    FCITX_D();
    return d->clientPreedit;
}

const Text &InputContext::clientPreedit() const {
    FCITX_D();
    return d->clientPreedit;
}

void InputContext::commitString(const std::string &text) {
    FCITX_D();
    CommitStringEvent event(text, this);
    if (auto instance = d->manager.instance()) {
        instance->postEvent(CommitStringEvent(text, this));
    }
    if (!event.accepted()) {
        commitStringImpl(event.text());
    }
}

void InputContext::deleteSurroundingText(int offset, unsigned int size) { deleteSurroundingTextImpl(offset, size); }

void InputContext::forwardKey(const KeyEvent &key) {}

void InputContext::updatePreedit() {}
}
