/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_INPUTCONTEXT_P_H_
#define _FCITX_INPUTCONTEXT_P_H_

#include <unordered_map>
#include <fcitx-utils/intrusivelist.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputcontextmanager.h>
#include <fcitx/inputcontextproperty.h>
#include <fcitx/inputpanel.h>
#include <fcitx/instance.h>
#include <fcitx/statusarea.h>
#include <uuid.h>

namespace fcitx {

class InputContextPrivate : public QPtrHolder<InputContext> {
public:
    InputContextPrivate(InputContext *q, InputContextManager &manager,
                        const std::string &program)
        : QPtrHolder(q), manager_(manager), group_(nullptr), inputPanel_(q),
          statusArea_(q), hasFocus_(false), program_(program) {
        uuid_generate(uuid_.data());
    }

    template <typename E>
    bool postEvent(E &&event) {
        if (destroyed_) {
            return true;
        }
        if (auto instance = manager_.instance()) {
            return instance->postEvent(event);
        }
        return false;
    }

    template <typename E, typename... Args>
    bool emplaceEvent(Args &&... args) {
        if (destroyed_) {
            return true;
        }
        if (auto instance = manager_.instance()) {
            return instance->postEvent(E(std::forward<Args>(args)...));
        }
        return false;
    }

    void registerProperty(int slot, InputContextProperty *property) {
        if (slot < 0) {
            return;
        }
        if (static_cast<size_t>(slot) >= properties_.size()) {
            properties_.resize(slot + 1);
        }
        properties_[slot].reset(property);
    }

    void unregisterProperty(int slot) {
        properties_[slot] = std::move(properties_.back());
        properties_.pop_back();
    }

    InputContextProperty *property(int slot) { return properties_[slot].get(); }

    InputContextManager &manager_;
    FocusGroup *group_;
    InputPanel inputPanel_;
    StatusArea statusArea_;
    bool hasFocus_ = false;
    std::string program_;
    CapabilityFlags capabilityFlags_;
    SurroundingText surroundingText_;
    Rect cursorRect_;

    IntrusiveListNode listNode_;
    IntrusiveListNode focusedListNode_;
    ICUUID uuid_;
    std::vector<std::unique_ptr<InputContextProperty>> properties_;
    bool destroyed_ = false;
};
} // namespace fcitx

#endif // _FCITX_INPUTCONTEXT_P_H_
