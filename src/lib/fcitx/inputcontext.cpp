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
#include <regex>
#include <stdexcept>
#include <string>
#include "fcitx-utils/event.h"
#include "fcitx-utils/utf8.h"
#include "focusgroup.h"
#include "inputcontext_p.h"
#include "inputcontextmanager.h"
#include "instance.h"
#include "misc_p.h"
#include "userinterfacemanager.h"

namespace fcitx {

namespace {

bool shouldDisablePreeditByDefault(const std::string &program) {
    static const std::vector<std::regex> matchers = []() {
        std::vector<std::regex> matchers;
        const char *apps = getenv("FCITX_NO_PREEDIT_APPS");
        if (!apps) {
            apps = NO_PREEDIT_APPS;
        }
        auto matcherStrings = stringutils::split(apps, ",");
        for (const auto &matcherString : matcherStrings) {
            try {
                matchers.emplace_back(matcherString, std::regex::icase |
                                                         std::regex::extended |
                                                         std::regex::nosubs);
            } catch (...) {
            }
        }
        return matchers;
    }();

    return std::any_of(matchers.begin(), matchers.end(),
                       [&program](const std::regex &regex) {
                           return std::regex_match(program, regex);
                       });
}
} // namespace

InputContextPrivate::InputContextPrivate(InputContext *q,
                                         InputContextManager &manager,
                                         const std::string &program)
    : QPtrHolder(q), manager_(manager), group_(nullptr), inputPanel_(q),
      statusArea_(q), program_(program),
      isPreeditEnabled_(manager.isPreeditEnabledByDefault() &&
                        !shouldDisablePreeditByDefault(program)) {}

#define RETURN_IF_HAS_NO_FOCUS(...)                                            \
    do {                                                                       \
        if (!hasFocus()) {                                                     \
            return __VA_ARGS__;                                                \
        }                                                                      \
    } while (0);

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

std::string_view InputContext::frontendName() const { return frontend(); }

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

double InputContext::scaleFactor() const {
    FCITX_D();
    return d->scale_;
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

bool InputContext::isVirtualKeyboardVisible() const {
    FCITX_D();
    if (auto *instance = d->manager_.instance()) {
        return instance->userInterfaceManager().isVirtualKeyboardVisible();
    }
    return false;
}

void InputContext::showVirtualKeyboard() const {
    FCITX_D();
    if (auto *instance = d->manager_.instance()) {
        return instance->userInterfaceManager().showVirtualKeyboard();
    }
}

void InputContext::hideVirtualKeyboard() const {
    FCITX_D();
    if (auto *instance = d->manager_.instance()) {
        return instance->userInterfaceManager().hideVirtualKeyboard();
    }
}

bool InputContext::clientControlVirtualkeyboardShow() const {
    FCITX_D();
    return d->clientControlVirtualkeyboardShow_;
}

void InputContext::setClientControlVirtualkeyboardShow(bool show) {
    FCITX_D();
    d->clientControlVirtualkeyboardShow_ = show;
}

bool InputContext::clientControlVirtualkeyboardHide() const {
    FCITX_D();
    return d->clientControlVirtualkeyboardHide_;
}

void InputContext::setClientControlVirtualkeyboardHide(bool hide) {
    FCITX_D();
    d->clientControlVirtualkeyboardHide_ = hide;
}

CapabilityFlags calculateFlags(CapabilityFlags flag, bool isPreeditEnabled) {
    if (!isPreeditEnabled) {
        flag = flag.unset(CapabilityFlag::Preedit)
                   .unset(CapabilityFlag::FormattedPreedit);
    }
    return flag;
}

void InputContext::setCapabilityFlags(CapabilityFlags flags) {
    FCITX_D();
    if (d->capabilityFlags_ == flags) {
        return;
    }
    const auto oldFlags = capabilityFlags();
    auto newFlags = calculateFlags(flags, d->isPreeditEnabled_);
    if (oldFlags != newFlags) {
        d->emplaceEvent<CapabilityAboutToChangeEvent>(this, oldFlags, flags);
    }
    d->capabilityFlags_ = flags;
    if (oldFlags != newFlags) {
        d->emplaceEvent<CapabilityChangedEvent>(this, oldFlags, flags);
    }
}

CapabilityFlags InputContext::capabilityFlags() const {
    FCITX_D();
    return calculateFlags(d->capabilityFlags_, d->isPreeditEnabled_);
}

void InputContext::setEnablePreedit(bool enable) {
    FCITX_D();
    if (enable == d->isPreeditEnabled_) {
        return;
    }
    const auto oldFlags = capabilityFlags();
    auto newFlags = calculateFlags(d->capabilityFlags_, enable);
    if (oldFlags != newFlags) {
        d->emplaceEvent<CapabilityAboutToChangeEvent>(this, oldFlags, newFlags);
    }
    d->isPreeditEnabled_ = enable;
    if (oldFlags != newFlags) {
        d->emplaceEvent<CapabilityChangedEvent>(this, oldFlags, newFlags);
    }
}

bool InputContext::isPreeditEnabled() const {
    FCITX_D();
    return d->isPreeditEnabled_;
}

void InputContext::setCursorRect(Rect rect) { setCursorRect(rect, 1.0); }

void InputContext::setCursorRect(Rect rect, double scale) {
    FCITX_D();
    if (d->cursorRect_ == rect && d->scale_ == scale) {
        return;
    }
    d->cursorRect_ = rect;
    d->scale_ = scale;
    d->emplaceEvent<CursorRectChangedEvent>(this);
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
    RETURN_IF_HAS_NO_FOCUS(false);
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
                     << "ms result:" << result;
    return result;
}

bool InputContext::virtualKeyboardEvent(VirtualKeyboardEvent &event) {
    FCITX_D();
    RETURN_IF_HAS_NO_FOCUS(false);
    decltype(std::chrono::steady_clock::now()) start;
    // Don't query time if we don't want log.
    if (::keyTrace().checkLogLevel(LogLevel::Debug)) {
        start = std::chrono::steady_clock::now();
    }
    auto result = d->postEvent(event);
    FCITX_KEYTRACE() << "VirtualKeyboardEvent handling time: "
                     << std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now() - start)
                            .count()
                     << "ms result:" << result;
    return result;
}

void InputContext::invokeAction(InvokeActionEvent &event) {
    FCITX_D();
    RETURN_IF_HAS_NO_FOCUS();
    d->postEvent(event);
}

void InputContext::reset(ResetReason) { reset(); }

void InputContext::reset() {
    FCITX_D();
    RETURN_IF_HAS_NO_FOCUS();
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

bool InputContext::hasPendingEventsStrictOrder() const {
    FCITX_D();
    if (d->blockedEvents_.empty()) {
        return false;
    }

    // Check we only have update preedit.
    if (std::any_of(d->blockedEvents_.begin(), d->blockedEvents_.end(),
                    [](const auto &event) {
                        return event->type() !=
                               EventType::InputContextUpdatePreedit;
                    })) {
        return true;
    }

    // Check whether the preedit is non-empty.
    // If key event may produce anything, it still may trigger the clear
    // preedit. In that case, preedit order does matter.
    return !inputPanel().clientPreedit().toString().empty();
}

void InputContext::commitString(const std::string &text) {
    FCITX_D();
    if (auto *instance = d->manager_.instance()) {
        auto newString = instance->commitFilter(this, text);
        d->pushEvent<CommitStringEvent>(std::move(newString), this);
    } else {
        d->pushEvent<CommitStringEvent>(text, this);
    }
}

void InputContext::commitStringWithCursor(const std::string &text,
                                          size_t cursor) {
    FCITX_D();
    if (cursor > utf8::length(text)) {
        throw std::invalid_argument(text);
    }

    if (auto *instance = d->manager_.instance()) {
        auto newString = instance->commitFilter(this, text);
        d->pushEvent<CommitStringWithCursorEvent>(std::move(newString), cursor,
                                                  this);
    } else {
        d->pushEvent<CommitStringWithCursorEvent>(text, cursor, this);
    }
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
    if (!capabilityFlags().test(CapabilityFlag::Preedit)) {
        return;
    }

    const bool preeditIsEmpty = inputPanel().clientPreedit().empty();
    if (preeditIsEmpty && d->lastPreeditUpdateIsEmpty_) {
        return;
    }
    d->lastPreeditUpdateIsEmpty_ = preeditIsEmpty;
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

const InputPanel &InputContext::inputPanel() const {
    FCITX_D();
    return d->inputPanel_;
}

StatusArea &InputContext::statusArea() {
    FCITX_D();
    return d->statusArea_;
}

const StatusArea &InputContext::statusArea() const {
    FCITX_D();
    return d->statusArea_;
}

void InputContext::updateClientSideUIImpl() {}

InputContextV2::~InputContextV2() = default;

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
