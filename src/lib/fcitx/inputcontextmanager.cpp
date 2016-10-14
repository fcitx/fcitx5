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

#include "inputcontextmanager.h"
#include "fcitx-utils/intrusivelist.h"
#include "focusgroup.h"
#include "focusgroup_p.h"
#include "inputcontext_p.h"
#include <unordered_map>

namespace {

void hash_combine(std::size_t &seed, std::size_t value) { seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2); }

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
}

namespace fcitx {

struct InputContextListHelper {
    static IntrusiveListNode &toNode(InputContext &ic) noexcept;
    static InputContext &toValue(IntrusiveListNode &node) noexcept;
};

struct FocusGroupListHelper {
    static IntrusiveListNode &toNode(FocusGroup &group) noexcept;
    static FocusGroup &toValue(IntrusiveListNode &node) noexcept;
};

inline InputContext *toInputContextPointer(InputContext *self) { return self; }
inline InputContext *toInputContextPointer(InputContext &self) { return &self; }

class InputContextManagerPrivate {
public:
    static InputContextPrivate *toInputContextPrivate(InputContext &ic) { return ic.d_func(); }
    static FocusGroupPrivate *toFocusGroupPrivate(FocusGroup &group) { return group.d_func(); }

    inline bool registerProperty(const std::string &name, InputContextPropertyFactory factory) {
        auto result = propertyFactories_.emplace(name, std::move(factory));
        if (!result.second) {
            return false;
        }
        for (auto &inputContext : inputContexts_) {
            inputContext.registerProperty(name, result.first->second(inputContext));
        }
        return true;
    }

    inline void unregisterProperty(const std::string &name) {
        propertyFactories_.erase(name);
        for (auto &inputContext : inputContexts_) {
            inputContext.unregisterProperty(name);
        }
    }

    inline void registerInputContext(InputContext &inputContext) {
        inputContexts_.push_back(inputContext);
        uuidMap_.emplace(inputContext.uuid(), &inputContext);
        if (!inputContext.program().empty()) {
            programMap_[inputContext.program()].insert(&inputContext);
        }
        for (auto &p : propertyFactories_) {
            auto property = p.second(inputContext);
            inputContext.registerProperty(p.first, property);
            if (property->needCopy() &&
                (propertyPropagatePolicy_ == PropertyPropagatePolicy::All ||
                 (!inputContext.program().empty() && propertyPropagatePolicy_ == PropertyPropagatePolicy::Program))) {
                auto copyProperty = [&p, &inputContext, &property](auto &container) {
                    for (auto &dstInputContext : container) {
                        if (toInputContextPointer(dstInputContext) != &inputContext) {
                            toInputContextPointer(dstInputContext)->property(p.first)->copyTo(property);
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

    std::unordered_map<std::array<uint8_t, sizeof(uuid_t)>, InputContext *, container_hasher> uuidMap_;
    IntrusiveList<InputContext, InputContextListHelper> inputContexts_;
    IntrusiveList<FocusGroup, FocusGroupListHelper> groups_;
    // order matters, need to delete it before groups gone
    std::unique_ptr<FocusGroup> globalFocusGroup_;
    Instance *instance_ = nullptr;
    std::unordered_map<std::string, InputContextPropertyFactory> propertyFactories_;
    std::unordered_map<std::string, std::unordered_set<InputContext *>> programMap_;
    PropertyPropagatePolicy propertyPropagatePolicy_ = PropertyPropagatePolicy::None;
};

IntrusiveListNode &InputContextListHelper::toNode(InputContext &ic) noexcept {
    return InputContextManagerPrivate::toInputContextPrivate(ic)->listNode_;
}

InputContext &InputContextListHelper::toValue(IntrusiveListNode &node) noexcept {
    return *parentFromMember(&node, &InputContextPrivate::listNode_)->q_func();
}

IntrusiveListNode &FocusGroupListHelper::toNode(FocusGroup &group) noexcept {
    return InputContextManagerPrivate::toFocusGroupPrivate(group)->listNode_;
}

FocusGroup &FocusGroupListHelper::toValue(IntrusiveListNode &node) noexcept {
    return *parentFromMember(&node, &FocusGroupPrivate::listNode_)->q_func();
}

InputContextManager::InputContextManager() : d_ptr(std::make_unique<InputContextManagerPrivate>()) {
    FCITX_D();
    d->globalFocusGroup_.reset(new FocusGroup(*this));
}

InputContextManager::~InputContextManager() {}

FocusGroup &InputContextManager::globalFocusGroup() {
    FCITX_D();
    return *d->globalFocusGroup_;
}

InputContext *InputContextManager::findByUUID(ICUUID uuid) {
    FCITX_D();
    auto iter = d->uuidMap_.find(uuid);
    return (iter == d->uuidMap_.end()) ? nullptr : iter->second;
}

bool InputContextManager::registerProperty(const std::string &name, InputContextPropertyFactory factory) {
    FCITX_D();
    return d->registerProperty(name, std::move(factory));
}

void InputContextManager::unregisterProperty(const std::string &name) {
    FCITX_D();
    return d->unregisterProperty(name);
}

void InputContextManager::setPropertyPropagatePolicy(PropertyPropagatePolicy policy) {
    FCITX_D();
    d->propertyPropagatePolicy_ = policy;
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
}

void InputContextManager::registerFocusGroup(FocusGroup &group) {
    FCITX_D();
    d->groups_.push_back(group);
}

void InputContextManager::unregisterFocusGroup(FocusGroup &group) {
    FCITX_D();
    d->groups_.erase(d->groups_.iterator_to(group));
}

void InputContextManager::propagateProperty(InputContext &inputContext, const std::string &name) {
    FCITX_D();
    if (d->propertyPropagatePolicy_ == PropertyPropagatePolicy::None ||
        (inputContext.program().empty() && d->propertyPropagatePolicy_ == PropertyPropagatePolicy::Program)) {
        return;
    }

    auto property = inputContext.property(name);
    auto copyProperty = [&name, &inputContext, &property](auto &container) {
        for (auto &dstInputContext_ : container) {
            auto dstInputContext = toInputContextPointer(dstInputContext_);
            if (dstInputContext != &inputContext) {
                property->copyTo(dstInputContext->property(name));
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

void InputContextManager::focusOutNonGlobal() {
    FCITX_D();
    for (auto &group : d->groups_) {
        if (&group != d->globalFocusGroup_.get()) {
            group.setFocusedInputContext(nullptr);
        }
    }
}
}
