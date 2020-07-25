/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "inputcontextmanager.h"
#include <cassert>
#include <stdexcept>
#include <unordered_map>
#include "fcitx-utils/intrusivelist.h"
#include "fcitx-utils/log.h"
#include "focusgroup.h"
#include "focusgroup_p.h"
#include "inputcontext_p.h"
#include "inputcontextproperty_p.h"

namespace {

void hash_combine(std::size_t &seed, std::size_t value) {
    seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

struct container_hasher {
    template <class T>
    std::size_t operator()(const T &c) const {
        std::size_t seed = 0;
        for (const auto &elem : c) {
            hash_combine(seed, std::hash<typename T::value_type>()(elem));
        }
        return seed;
    }
};
} // namespace

namespace fcitx {

struct InputContextListHelper {
    static IntrusiveListNode &toNode(InputContext &ic) noexcept;
    static InputContext &toValue(IntrusiveListNode &node) noexcept;
    static const IntrusiveListNode &toNode(const InputContext &ic) noexcept;
    static const InputContext &toValue(const IntrusiveListNode &node) noexcept;
};

struct InputContextFocusedListHelper {
    static IntrusiveListNode &toNode(InputContext &ic) noexcept;
    static InputContext &toValue(IntrusiveListNode &node) noexcept;
    static const IntrusiveListNode &toNode(const InputContext &ic) noexcept;
    static const InputContext &toValue(const IntrusiveListNode &node) noexcept;
};

struct FocusGroupListHelper {
    static IntrusiveListNode &toNode(FocusGroup &group) noexcept;
    static FocusGroup &toValue(IntrusiveListNode &node) noexcept;
    static const IntrusiveListNode &toNode(const FocusGroup &group) noexcept;
    static const FocusGroup &toValue(const IntrusiveListNode &node) noexcept;
};

inline InputContext *toInputContextPointer(InputContext *self) { return self; }
inline InputContext *toInputContextPointer(InputContext &self) { return &self; }

class InputContextManagerPrivate {
public:
    static InputContextPrivate *toInputContextPrivate(InputContext &ic) {
        return ic.d_func();
    }
    static FocusGroupPrivate *toFocusGroupPrivate(FocusGroup &group) {
        return group.d_func();
    }
    static const InputContextPrivate *
    toInputContextPrivate(const InputContext &ic) {
        return ic.d_func();
    }
    static const FocusGroupPrivate *
    toFocusGroupPrivate(const FocusGroup &group) {
        return group.d_func();
    }

    inline bool registerProperty(InputContextManager *q_ptr,
                                 const std::string &name,
                                 InputContextPropertyFactoryPrivate *factory) {
        auto result = propertyFactories_.emplace(name, factory);
        if (!result.second) {
            return false;
        }
        factory->manager_ = q_ptr;
        factory->slot_ = propertyFactoriesSlots_.size();
        factory->name_ = name;

        propertyFactoriesSlots_.push_back(factory);
        for (auto &inputContext : inputContexts_) {
            inputContext.d_func()->registerProperty(
                factory->slot_, factory->q_func()->create(inputContext));
        }
        return true;
    }

    inline void unregisterProperty(const std::string &name) {
        auto iter = propertyFactories_.find(name);
        if (iter == propertyFactories_.end()) {
            return;
        }
        auto factory = iter->second;
        auto slot = factory->slot_;
        // move slot, logic need to be same as inputContext
        propertyFactoriesSlots_[slot] = propertyFactoriesSlots_.back();
        propertyFactoriesSlots_[slot]->slot_ = slot;
        propertyFactoriesSlots_.pop_back();

        for (auto &inputContext : inputContexts_) {
            inputContext.d_func()->unregisterProperty(slot);
        }
        propertyFactories_.erase(iter);

        factory->manager_ = nullptr;
        factory->name_ = std::string();
        factory->slot_ = -1;
    }

    inline void registerInputContext(InputContext &inputContext) {
        inputContexts_.push_back(inputContext);
        uuidMap_.emplace(inputContext.uuid(), &inputContext);
        if (!inputContext.program().empty()) {
            programMap_[inputContext.program()].insert(&inputContext);
        }
        for (auto &p : propertyFactories_) {
            auto property = p.second->q_func()->create(inputContext);
            inputContext.d_func()->registerProperty(p.second->slot_, property);
            if (property->needCopy() &&
                (propertyPropagatePolicy_ == PropertyPropagatePolicy::All ||
                 (!inputContext.program().empty() &&
                  propertyPropagatePolicy_ ==
                      PropertyPropagatePolicy::Program))) {
                auto copyProperty = [&p, &inputContext,
                                     &property](auto &container) {
                    for (auto &dstInputContext : container) {
                        if (toInputContextPointer(dstInputContext) !=
                            &inputContext) {
                            toInputContextPointer(dstInputContext)
                                ->property(p.first)
                                ->copyTo(property);
                            break;
                        }
                    }
                };
                if (propertyPropagatePolicy_ == PropertyPropagatePolicy::All) {
                    copyProperty(inputContexts_);
                } else {
                    auto iter = programMap_.find(inputContext.program());
                    if (iter != programMap_.end()) {
                        copyProperty(iter->second);
                    }
                }
            }
        }
    }

    std::unordered_map<std::array<uint8_t, sizeof(uuid_t)>, InputContext *,
                       container_hasher>
        uuidMap_;
    IntrusiveList<InputContext, InputContextListHelper> inputContexts_;
    IntrusiveList<InputContext, InputContextFocusedListHelper>
        focusedInputContexts_;
    TrackableObjectReference<InputContext> mostRecentInputContext_;
    IntrusiveList<FocusGroup, FocusGroupListHelper> groups_;
    // order matters, need to delete it before groups gone
    Instance *instance_ = nullptr;
    std::unordered_map<std::string, InputContextPropertyFactoryPrivate *>
        propertyFactories_;
    std::vector<InputContextPropertyFactoryPrivate *> propertyFactoriesSlots_;
    std::unordered_map<std::string, std::unordered_set<InputContext *>>
        programMap_;
    PropertyPropagatePolicy propertyPropagatePolicy_ =
        PropertyPropagatePolicy::No;
    bool finalized_ = false;
};

#define DEFINE_LIST_HELPERS(HELPERTYPE, TYPE, MEMBER)                          \
    IntrusiveListNode &HELPERTYPE::toNode(TYPE &ic) noexcept {                 \
        return InputContextManagerPrivate::to##TYPE##Private(ic)->MEMBER;      \
    }                                                                          \
    TYPE &HELPERTYPE::toValue(IntrusiveListNode &node) noexcept {              \
        return *parentFromMember(&node, &TYPE##Private::MEMBER)->q_func();     \
    }                                                                          \
    const IntrusiveListNode &HELPERTYPE::toNode(const TYPE &ic) noexcept {     \
        return InputContextManagerPrivate::to##TYPE##Private(ic)->MEMBER;      \
    }                                                                          \
    const TYPE &HELPERTYPE::toValue(const IntrusiveListNode &node) noexcept {  \
        return *parentFromMember(&node, &TYPE##Private::MEMBER)->q_func();     \
    }

DEFINE_LIST_HELPERS(InputContextListHelper, InputContext, listNode_)
DEFINE_LIST_HELPERS(InputContextFocusedListHelper, InputContext,
                    focusedListNode_)
DEFINE_LIST_HELPERS(FocusGroupListHelper, FocusGroup, listNode_)

InputContextManager::InputContextManager()
    : d_ptr(std::make_unique<InputContextManagerPrivate>()) {}

InputContextManager::~InputContextManager() {}

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
        if (visitor(&ic)) {
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
    return d->unregisterProperty(name);
}

void InputContextManager::setPropertyPropagatePolicy(
    PropertyPropagatePolicy policy) {
    FCITX_D();
    d->propertyPropagatePolicy_ = policy;
}

void InputContextManager::finalize() {
    FCITX_D();
    d->finalized_ = true;
    while (d->inputContexts_.size()) {
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

    if (d->focusedInputContexts_.isInList(inputContext)) {
        d->focusedInputContexts_.erase(
            d->focusedInputContexts_.iterator_to(inputContext));
    }
}

void InputContextManager::registerFocusGroup(FocusGroup &group) {
    FCITX_D();
    FCITX_DEBUG() << "Register focus group for display: " << group.display();
    d->groups_.push_back(group);
}

void InputContextManager::unregisterFocusGroup(FocusGroup &group) {
    FCITX_D();
    d->groups_.erase(d->groups_.iterator_to(group));
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

    auto property = this->property(inputContext, factory);
    auto factoryRef = factory->watch();
    auto copyProperty = [this, &factoryRef, &inputContext,
                         &property](auto &container) {
        for (auto &dstInputContext_ : container) {
            if (auto factory = factoryRef.get()) {
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

void InputContextManager::notifyFocus(InputContext &ic, bool hasFocus) {
    FCITX_D();
    if (hasFocus) {
        if (d->focusedInputContexts_.isInList(ic)) {
            if (&d->focusedInputContexts_.front() == &ic) {
                return;
            }
            auto iter = d->focusedInputContexts_.iterator_to(ic);
            d->focusedInputContexts_.erase(iter);
        }
        d->focusedInputContexts_.push_front(ic);
        d->mostRecentInputContext_.unwatch();
    } else {
        if (d->focusedInputContexts_.isInList(ic)) {
            auto iter = d->focusedInputContexts_.iterator_to(ic);
            d->focusedInputContexts_.erase(iter);
        }
        // If this is the last ic. watch it.
        if (d->focusedInputContexts_.empty()) {
            d->mostRecentInputContext_ = ic.watch();
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
    auto ic = lastFocusedInputContext();
    if (ic) {
        return ic;
    }
    return d->mostRecentInputContext_.get();
}
} // namespace fcitx
