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

#include <unordered_map>
#include "inputcontextmanager.h"
#include "inputcontext_p.h"
#include "focusgroup.h"
#include "focusgroup_p.h"
#include "fcitx-utils/intrusivelist.h"

namespace {

template <class Parent, class Member>
inline std::ptrdiff_t offset_from_pointer_to_member(const Member Parent::*ptr_to_member) {
    const Parent *const parent = 0;
    const char *const member = static_cast<const char *>(static_cast<const void *>(&(parent->*ptr_to_member)));
    return std::ptrdiff_t(member - static_cast<const char *>(static_cast<const void *>(parent)));
}

template <class Parent, class Member>
inline Parent *parent_from_member(Member *member, const Member Parent::*ptr_to_member) {
    return static_cast<Parent *>(static_cast<void *>(static_cast<char *>(static_cast<void *>(member)) -
                                                     offset_from_pointer_to_member(ptr_to_member)));
}

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

class InputContextManagerPrivate {
public:
    static InputContextPrivate *toInputContextPrivate(InputContext &ic) { return ic.d_func(); }
    static FocusGroupPrivate *toFocusGroupPrivate(FocusGroup &group) { return group.d_func(); }

    // order matters, need to delete it before groups gone
    std::unique_ptr<FocusGroup> globalFocusGroup;
    std::unordered_map<std::array<uint8_t, sizeof(uuid_t)>, InputContext *, container_hasher> uuidMap;
    IntrusiveList<InputContext, InputContextListHelper> inputContexts;
    IntrusiveList<FocusGroup, FocusGroupListHelper> groups;
    Instance *instance = nullptr;
};

IntrusiveListNode &InputContextListHelper::toNode(InputContext &ic) noexcept {
    return InputContextManagerPrivate::toInputContextPrivate(ic)->listNode;
}

InputContext &InputContextListHelper::toValue(IntrusiveListNode &node) noexcept {
    return *parent_from_member(&node, &InputContextPrivate::listNode)->q_func();
}

IntrusiveListNode &FocusGroupListHelper::toNode(FocusGroup &group) noexcept {
    return InputContextManagerPrivate::toFocusGroupPrivate(group)->listNode;
}

FocusGroup &FocusGroupListHelper::toValue(IntrusiveListNode &node) noexcept {
    return *parent_from_member(&node, &FocusGroupPrivate::listNode)->q_func();
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
    d->inputContexts.push_back(inputContext);
    // d->uuidMap.insert();
}

void InputContextManager::unregisterInputContext(InputContext &inputContext) {
    FCITX_D();
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

void InputContextManager::focusOutNonGlobal() {
    FCITX_D();
    for (auto &group : d->groups) {
        if (&group != d->globalFocusGroup.get()) {
            group.setFocusedInputContext(nullptr);
        }
    }
}
}
