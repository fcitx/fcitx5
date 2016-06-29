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

    inline int registerProperty(InputContextPropertyFactory factory) {
        propertyFactories.emplace(propertyIdx, factory);
        for (auto &inputContext : inputContexts) {
            inputContext.registerProperty(propertyIdx, factory(inputContext));
        }
        return propertyIdx++;
    }

    inline void unregisterProperty(int idx) {
        propertyFactories.erase(idx);
        for (auto &inputContext : inputContexts) {
            inputContext.unregisterProperty(idx);
        }
    }

    inline void registerInputContext(InputContext &inputContext) {
        inputContexts.push_back(inputContext);
        uuidMap.emplace(inputContext.uuid(), &inputContext);
        if (!inputContext.program().empty()) {
            programMap[inputContext.program()].insert(&inputContext);
        }
        for (auto &p : propertyFactories) {
            auto property = p.second(inputContext);
            inputContext.registerProperty(p.first, property);
            if (property->needCopy() &&
                (propertyPropagatePolicy == PropertyPropagatePolicy::All ||
                 (!inputContext.program().empty() && propertyPropagatePolicy == PropertyPropagatePolicy::Program))) {
                auto copyProperty = [&p, &inputContext, &property](auto &container) {
                    for (auto &dstInputContext : container) {
                        if (toInputContextPointer(dstInputContext) != &inputContext) {
                            toInputContextPointer(dstInputContext)->property(p.first)->copyTo(property);
                            break;
                        }
                    }
                };
                if (propertyPropagatePolicy == PropertyPropagatePolicy::All) {
                    copyProperty(inputContexts);
                } else {
                    auto iter = programMap.find(inputContext.program());
                    if (iter != programMap.end()) {
                        copyProperty(iter->second);
                    }
                }
            }
        }
    }

    std::unordered_map<std::array<uint8_t, sizeof(uuid_t)>, InputContext *, container_hasher> uuidMap;
    IntrusiveList<InputContext, InputContextListHelper> inputContexts;
    IntrusiveList<FocusGroup, FocusGroupListHelper> groups;
    // order matters, need to delete it before groups gone
    std::unique_ptr<FocusGroup> globalFocusGroup;
    Instance *instance = nullptr;
    int propertyIdx = 0;
    std::unordered_map<int, InputContextPropertyFactory> propertyFactories;
    std::unordered_map<std::string, std::unordered_set<InputContext *>> programMap;
    PropertyPropagatePolicy propertyPropagatePolicy = PropertyPropagatePolicy::None;
};

IntrusiveListNode &InputContextListHelper::toNode(InputContext &ic) noexcept {
    return InputContextManagerPrivate::toInputContextPrivate(ic)->listNode;
}

InputContext &InputContextListHelper::toValue(IntrusiveListNode &node) noexcept {
    return *parentFromMember(&node, &InputContextPrivate::listNode)->q_func();
}

IntrusiveListNode &FocusGroupListHelper::toNode(FocusGroup &group) noexcept {
    return InputContextManagerPrivate::toFocusGroupPrivate(group)->listNode;
}

FocusGroup &FocusGroupListHelper::toValue(IntrusiveListNode &node) noexcept {
    return *parentFromMember(&node, &FocusGroupPrivate::listNode)->q_func();
}

InputContextManager::InputContextManager() : d_ptr(std::make_unique<InputContextManagerPrivate>()) {
    FCITX_D();
    d->globalFocusGroup.reset(new FocusGroup(*this));
}

InputContextManager::~InputContextManager() {}

FocusGroup &InputContextManager::globalFocusGroup() {
    FCITX_D();
    return *d->globalFocusGroup;
}

InputContext *InputContextManager::findByUUID(ICUUID uuid) {
    FCITX_D();
    auto iter = d->uuidMap.find(uuid);
    return (iter == d->uuidMap.end()) ? nullptr : iter->second;
}

int InputContextManager::registerProperty(InputContextPropertyFactory factory) {
    FCITX_D();
    return d->registerProperty(std::move(factory));
}

void InputContextManager::unregisterProperty(int idx) {
    FCITX_D();
    return d->unregisterProperty(idx);
}

void InputContextManager::setPropertyPropagatePolicy(PropertyPropagatePolicy policy) {
    FCITX_D();
    d->propertyPropagatePolicy = policy;
}

void InputContextManager::setInstance(Instance *instance) {
    FCITX_D();
    d->instance = instance;
}

Instance *InputContextManager::instance() {
    FCITX_D();
    return d->instance;
}

void InputContextManager::registerInputContext(InputContext &inputContext) {
    FCITX_D();
    d->registerInputContext(inputContext);
}

void InputContextManager::unregisterInputContext(InputContext &inputContext) {
    FCITX_D();
    if (!inputContext.program().empty()) {
        auto iter = d->programMap.find(inputContext.program());
        if (iter != d->programMap.end()) {
            iter->second.erase(&inputContext);
            if (iter->second.empty()) {
                d->programMap.erase(iter);
            }
        }
    }
    d->uuidMap.erase(inputContext.uuid());
    d->inputContexts.erase(d->inputContexts.iterator_to(inputContext));
}

void InputContextManager::registerFocusGroup(fcitx::FocusGroup &group) {
    FCITX_D();
    d->groups.push_back(group);
}

void InputContextManager::unregisterFocusGroup(fcitx::FocusGroup &group) {
    FCITX_D();
    d->groups.erase(d->groups.iterator_to(group));
}

void InputContextManager::propagateProperty(InputContext &inputContext, int idx) {
    FCITX_D();
    if (d->propertyPropagatePolicy == PropertyPropagatePolicy::None ||
        (inputContext.program().empty() && d->propertyPropagatePolicy == PropertyPropagatePolicy::Program)) {
        return;
    }

    auto property = inputContext.property(idx);
    auto copyProperty = [idx, &inputContext, &property](auto &container) {
        for (auto &dstInputContext_ : container) {
            auto dstInputContext = toInputContextPointer(dstInputContext_);
            if (dstInputContext != &inputContext) {
                property->copyTo(dstInputContext->property(idx));
            }
        }
    };

    if (d->propertyPropagatePolicy == PropertyPropagatePolicy::All) {
        copyProperty(d->inputContexts);
    } else {
        auto iter = d->programMap.find(inputContext.program());
        if (iter != d->programMap.end()) {
            copyProperty(iter->second);
        }
    }
}

void InputContextManager::focusOutNonGlobal() {
    FCITX_D();
    for (auto &group : d->groups) {
        if (&group != d->globalFocusGroup.get()) {
            group.setFocusedInputContext(nullptr);
        }
    }
}
}
