/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "inputcontext.h"
#include <cassert>
#include <chrono>
#include <exception>
#include "fcitx-utils/event.h"
#include "focusgroup.h"
#include "inputcontext_p.h"
#include "inputcontextmanager.h"
#include "instance.h"
#include "misc_p.h"

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
    auto *factory = d->manager_.factoryForName(name);
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
    auto *factory = d->manager_.factoryForName(name);
    if (!factory) {
        return;
    }
    updateProperty(factory);
}

void InputContext::updateProperty(const InputContextPropertyFactory *factory) {
    FCITX_D();
    auto *property = d->manager_.property(*this, factory);
    if (!property->needCopy()) {
        return;
    }
    d->manager_.propagateProperty(*this, factory);
}

void InputContext::setCapabilityFlags(CapabilityFlags flags) {
    FCITX_D();
    if (d->capabilityFlags_ != flags) {
        auto oldFlags = d->capabilityFlags_;
        d->emplaceEvent<CapabilityAboutToChangeEvent>(this, oldFlags, flags);
        d->capabilityFlags_ = flags;
        d->emplaceEvent<CapabilityChangedEvent>(this, oldFlags, flags);
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
    decltype(std::chrono::steady_clock::now()) start;
    // Don't query time if we don't want log.
    if (::keyTrace().checkLogLevel(LogLevel::Debug)) {
        start = std::chrono::steady_clock::now();
    }
    auto result = d->postEvent(event);
    FCITX_KEYTRACE() << "KeyEvent handling time: "
                     << std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now() - start)
                            .count()
                     << "ms";
    return result;
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

void InputContext::setBlockEventToClient(bool block) {
    FCITX_D();
    if (d->blockEventToClient_ == block) {
        throw std::invalid_argument(
            "setBlockEventToClient has invalid argument. Probably a bug in the "
            "implementation.");
    }
    d->blockEventToClient_ = block;
    if (!block) {
        d->deliverBlockedEvents();
    }
}

bool InputContext::hasPendingEvents() const {
    FCITX_D();
    return !d->blockedEvents_.empty();
}

void InputContext::commitString(const std::string &text) {
    FCITX_D();
    d->pushEvent<CommitStringEvent>(text, this);
}

void InputContext::deleteSurroundingText(int offset, unsigned int size) {
    deleteSurroundingTextImpl(offset, size);
}

void InputContext::forwardKey(const Key &rawKey, bool isRelease, int time) {
    FCITX_D();
    d->pushEvent<ForwardKeyEvent>(this, rawKey, isRelease, time);
}

void InputContext::updatePreedit() {
    FCITX_D();
    if (!d->capabilityFlags_.test(CapabilityFlag::Preedit)) {
        return;
    }
    d->pushEvent<UpdatePreeditEvent>(this);
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

InputContextEventBlocker::InputContextEventBlocker(InputContext *inputContext)
    : inputContext_(inputContext->watch()) {
    inputContext->setBlockEventToClient(true);
}

InputContextEventBlocker::~InputContextEventBlocker() {
    if (auto *ic = inputContext_.get()) {
        ic->setBlockEventToClient(false);
    }
}

} // namespace fcitx
