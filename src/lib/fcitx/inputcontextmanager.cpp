/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "inputcontextmanager.h"
#include <cassert>
#include <memory>
#include <stdexcept>
#include <string>
#include "fcitx-utils/intrusivelist.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/macros.h"
#include "fcitx-utils/misc.h"
#include "event.h"
#include "focusgroup.h"
#include "focusgroup_p.h"
#include "inputcontext.h"
#include "inputcontext_p.h"
#include "inputcontextmanager_p.h"
#include "inputcontextproperty.h"
#include "inputcontextproperty_p.h"

namespace fcitx {

namespace {

// The dummy input context can be used to propagate context state when the
// PropertyPropagatePolicy is All
class DummyInputContext : public InputContext {
public:
    DummyInputContext(InputContextManager &manager)
        : InputContext(manager, "") {}

    ~DummyInputContext() { destroy(); }

    const char *frontend() const override { return "dummy"; }

    void commitStringImpl(const std::string & /*text*/) override {}
    void deleteSurroundingTextImpl(int /*offset*/,
                                   unsigned int /*size*/) override {}
    void forwardKeyImpl(const ForwardKeyEvent & /*key*/) override {}
    void updatePreeditImpl() override {}
};

} // namespace

#define DEFINE_LIST_HELPERS(HELPERTYPE, TYPE, MEMBER)                          \
    IntrusiveListNode &HELPERTYPE::toNode(TYPE &value) noexcept {              \
        return InputContextManagerPrivate::to##TYPE##Private(value)->MEMBER;   \
    }                                                                          \
    TYPE &HELPERTYPE::toValue(IntrusiveListNode &node) noexcept {              \
        return *parentFromMember(&node, &TYPE##Private::MEMBER)->q_func();     \
    }                                                                          \
    const IntrusiveListNode &HELPERTYPE::toNode(const TYPE &value) noexcept {  \
        return InputContextManagerPrivate::to##TYPE##Private(value)->MEMBER;   \
    }                                                                          \
    const TYPE &HELPERTYPE::toValue(const IntrusiveListNode &node) noexcept {  \
        return *parentFromMember(&node, &TYPE##Private::MEMBER)->q_func();     \
    }

DEFINE_LIST_HELPERS(InputContextListHelper, InputContext, listNode_)
DEFINE_LIST_HELPERS(InputContextFocusedListHelper, InputContext,
                    focusedListNode_)
DEFINE_LIST_HELPERS(FocusGroupListHelper, FocusGroup, listNode_)

InputContextManager::InputContextManager()
    : d_ptr(std::make_unique<InputContextManagerPrivate>()) {
    FCITX_D();

    d->dummyInputContext_ = std::make_unique<DummyInputContext>(*this);
}

InputContextManager::~InputContextManager() {
    FCITX_D();
    // Need to reset this before d becomes invalid.
    d->dummyInputContext_.reset();
}

InputContext *InputContextManager::findByUUID(ICUUID uuid) {
    FCITX_D();
    auto iter = d->uuidMap_.find(uuid);
    return (iter == d->uuidMap_.end()) ? nullptr : iter->second;
}

bool InputContextManager::foreach(const InputContextVisitor &visitor) {
    FCITX_D();
    for (auto &ic : d->inputContexts_) {
        if (!visitor(&ic)) {
            return false;
        }
    }
    return true;
}

bool InputContextManager::foreachFocused(const InputContextVisitor &visitor) {
    FCITX_D();
    for (auto &ic : d->focusedInputContexts_) {
        if (!visitor(&ic)) {
            return false;
        }
    }
    return true;
}

bool InputContextManager::foreachGroup(const FocusGroupVisitor &visitor) {
    FCITX_D();
    for (auto &group : d->groups_) {
        if (!visitor(&group)) {
            return false;
        }
    }
    return true;
}

bool InputContextManager::registerProperty(
    const std::string &name, InputContextPropertyFactory *factory) {
    FCITX_D();
    return d->registerProperty(this, name, factory->d_func());
}

void InputContextManager::unregisterProperty(const std::string &name) {
    FCITX_D();
    d->unregisterProperty(name);
}

void InputContextManager::setPropertyPropagatePolicy(
    PropertyPropagatePolicy policy) {
    FCITX_D();
    d->propertyPropagatePolicy_ = policy;
}

void InputContextManager::finalize() {
    FCITX_D();
    d->finalized_ = true;
    d->dummyInputContext_.reset();
    while (!d->inputContexts_.empty()) {
        delete &d->inputContexts_.front();
    }
}

void InputContextManager::setInstance(Instance *instance) {
    FCITX_D();
    d->instance_ = instance;
}

Instance *InputContextManager::instance() {
    FCITX_D();
    return d->instance_;
}

void InputContextManager::registerInputContext(InputContext &inputContext) {
    FCITX_D();
    if (d->finalized_) {
        throw std::runtime_error(
            "Should not register input context after finalize");
    }
    d->registerInputContext(inputContext);
}

void InputContextManager::unregisterInputContext(InputContext &inputContext) {
    FCITX_D();
    if (!inputContext.program().empty()) {
        auto iter = d->programMap_.find(inputContext.program());
        if (iter != d->programMap_.end()) {
            iter->second.erase(&inputContext);
            if (iter->second.empty()) {
                d->programMap_.erase(iter);
            }
        }
    }
    d->uuidMap_.erase(inputContext.uuid());
    d->inputContexts_.erase(d->inputContexts_.iterator_to(inputContext));

    d->resetMostRecentInputContext(inputContext);
}

void InputContextManager::registerFocusGroup(FocusGroup &group) {
    FCITX_D();
    FCITX_DEBUG() << "Register focus group for display: " << group.display();
    d->groups_.push_back(group);
}

void InputContextManager::unregisterFocusGroup(FocusGroup &group) {
    FCITX_D();
    d->groups_.erase(d->groups_.iterator_to(group));
    if (d->instance_ && d->instance_->exitWhenMainDisplayDisconnected() &&
        d->groups_.empty()) {
        if (!d->instance_->exiting()) {
            FCITX_INFO() << "All display connections are gone, exit now.";
            d->instance_->exit();
        }
    }
}
InputContextPropertyFactory *
InputContextManager::factoryForName(const std::string &name) {
    FCITX_D();
    auto iter = d->propertyFactories_.find(name);
    if (iter == d->propertyFactories_.end()) {
        return nullptr;
    }
    return iter->second->q_func();
}
InputContextProperty *
InputContextManager::property(InputContext &inputContext,
                              const InputContextPropertyFactory *factory) {
    assert(factory->d_func()->manager_ == this);
    return InputContextManagerPrivate::toInputContextPrivate(inputContext)
        ->property(factory->d_func()->slot_);
}

void InputContextManager::propagateProperty(
    InputContext &inputContext, const InputContextPropertyFactory *factory) {
    FCITX_D();
    assert(factory->d_func()->manager_ == this);
    if (d->propertyPropagatePolicy_ == PropertyPropagatePolicy::No ||
        (inputContext.program().empty() &&
         d->propertyPropagatePolicy_ == PropertyPropagatePolicy::Program)) {
        return;
    }

    auto *property = this->property(inputContext, factory);
    auto factoryRef = factory->watch();
    auto copyProperty = [this, &factoryRef, &inputContext,
                         &property](auto &container) {
        for (auto &dstInputContext_ : container) {
            if (const auto *factory = factoryRef.get()) {
                auto dstInputContext = toInputContextPointer(dstInputContext_);
                if (dstInputContext != &inputContext) {
                    property->copyTo(this->property(*dstInputContext, factory));
                }
            }
        }
    };

    if (d->propertyPropagatePolicy_ == PropertyPropagatePolicy::All) {
        copyProperty(d->inputContexts_);
    } else {
        auto iter = d->programMap_.find(inputContext.program());
        if (iter != d->programMap_.end()) {
            copyProperty(iter->second);
        }
    }
}

void InputContextManager::notifyFocus(InputContext &inputContext,
                                      bool hasFocus) {
    FCITX_D();
    if (hasFocus) {
        if (d->focusedInputContexts_.isInList(inputContext)) {
            if (&d->focusedInputContexts_.front() == &inputContext) {
                return;
            }
            auto iter = d->focusedInputContexts_.iterator_to(inputContext);
            d->focusedInputContexts_.erase(iter);
        }
        d->focusedInputContexts_.push_front(inputContext);
        d->mostRecentInputContext_.unwatch();
    } else {
        if (d->focusedInputContexts_.isInList(inputContext)) {
            auto iter = d->focusedInputContexts_.iterator_to(inputContext);
            d->focusedInputContexts_.erase(iter);
        }
        // If this is the last ic. watch it.
        if (d->focusedInputContexts_.empty()) {
            d->mostRecentInputContext_ = inputContext.watch();
        }
    }
}

InputContext *InputContextManager::lastFocusedInputContext() {
    FCITX_D();
    return d->focusedInputContexts_.empty() ? nullptr
                                            : &d->focusedInputContexts_.front();
}

InputContext *InputContextManager::mostRecentInputContext() {
    FCITX_D();
    auto *ic = lastFocusedInputContext();
    if (!ic) {
        ic = d->mostRecentInputContext_.get();
    }

    if (!ic && d->propertyPropagatePolicy_ == PropertyPropagatePolicy::All) {
        ic = d->dummyInputContext_.get();
    }

    return ic;
}

InputContext *InputContextManager::dummyInputContext() const {
    FCITX_D();
    return d->dummyInputContext_.get();
}

void InputContextManager::setPreeditEnabledByDefault(bool enable) {
    FCITX_D();
    d->preeditEnabledByDefault_ = enable;
}

bool InputContextManager::isPreeditEnabledByDefault() const {
    FCITX_D();
    return d->preeditEnabledByDefault_;
}
} // namespace fcitx
