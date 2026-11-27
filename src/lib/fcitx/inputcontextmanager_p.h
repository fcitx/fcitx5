/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_INPUTCONTEXTMANAGER_P_H_
#define _FCITX_INPUTCONTEXTMANAGER_P_H_

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "fcitx-utils/intrusivelist.h"
#include "fcitx-utils/trackableobject.h"
#include "fcitx-utils/uuid_p.h"
#include "focusgroup.h"
#include "focusgroup_p.h"
#include "inputcontext.h"
#include "inputcontext_p.h"
#include "inputcontextmanager.h"
#include "inputcontextproperty_p.h"
#include "misc_p.h"

namespace fcitx {

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

struct InputContextListHelper {
    [[maybe_unused]] static IntrusiveListNode &
    toNode(InputContext &value) noexcept;
    [[maybe_unused]] static InputContext &
    toValue(IntrusiveListNode &node) noexcept;
    [[maybe_unused]] static const IntrusiveListNode &
    toNode(const InputContext &value) noexcept;
    [[maybe_unused]] static const InputContext &
    toValue(const IntrusiveListNode &node) noexcept;
};

struct InputContextFocusedListHelper {
    [[maybe_unused]] static IntrusiveListNode &
    toNode(InputContext &value) noexcept;
    [[maybe_unused]] static InputContext &
    toValue(IntrusiveListNode &node) noexcept;
    [[maybe_unused]] static const IntrusiveListNode &
    toNode(const InputContext &value) noexcept;
    [[maybe_unused]] static const InputContext &
    toValue(const IntrusiveListNode &node) noexcept;
};

struct FocusGroupListHelper {
    [[maybe_unused]] static IntrusiveListNode &
    toNode(FocusGroup &value) noexcept;
    [[maybe_unused]] static FocusGroup &
    toValue(IntrusiveListNode &node) noexcept;
    [[maybe_unused]] static const IntrusiveListNode &
    toNode(const FocusGroup &value) noexcept;
    [[maybe_unused]] static const FocusGroup &
    toValue(const IntrusiveListNode &node) noexcept;
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

    bool registerProperty(InputContextManager *q_ptr, const std::string &name,
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

    void unregisterProperty(const std::string &name) {
        auto iter = propertyFactories_.find(name);
        if (iter == propertyFactories_.end()) {
            return;
        }
        auto *factory = iter->second;
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

    void registerInputContext(InputContext &inputContext) {
        inputContexts_.push_back(inputContext);
        int maxRetry = 3;
        do {
            generateUUID(inputContext.d_func()->uuid_.data());
            maxRetry -= 1;
        } while (!uuidMap_.emplace(inputContext.uuid(), &inputContext).second &&
                 maxRetry > 0);
        if (!inputContext.program().empty()) {
            programMap_[inputContext.program()].insert(&inputContext);
        }
        for (auto *factory : propertyFactoriesSlots_) {
            inputContext.d_func()->registerProperty(
                factory->slot_, factory->q_func()->create(inputContext));
        }
        for (auto *factory : propertyFactoriesSlots_) {
            auto *property = inputContext.d_func()->property(factory->slot_);
            if (property->needCopy() &&
                (propertyPropagatePolicy_ == PropertyPropagatePolicy::All ||
                 (!inputContext.program().empty() &&
                  propertyPropagatePolicy_ ==
                      PropertyPropagatePolicy::Program))) {
                auto copyProperty = [factory, &inputContext,
                                     &property](auto &container) {
                    for (auto &dstInputContext : container) {
                        if (toInputContextPointer(dstInputContext) !=
                            &inputContext) {
                            toInputContextPointer(dstInputContext)
                                ->d_func()
                                ->property(factory->slot_)
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

    void resetMostRecentInputContext(InputContext &inputContext) {
        if (focusedInputContexts_.isInList(inputContext)) {
            focusedInputContexts_.erase(
                focusedInputContexts_.iterator_to(inputContext));
        }
        if (mostRecentInputContext_.get() == &inputContext) {
            mostRecentInputContext_.unwatch();
        }
    }

    std::unordered_map<ICUUID, InputContext *, container_hasher> uuidMap_;
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
    bool preeditEnabledByDefault_ = true;
    std::unique_ptr<InputContext> dummyInputContext_;
};

} // namespace fcitx

#endif // _FCITX_INPUTCONTEXTMANAGER_P_H_
