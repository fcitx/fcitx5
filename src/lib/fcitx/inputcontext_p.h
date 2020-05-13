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

    template <typename E, typename... Args>
    void pushEvent(Args &&... args) {
        if (destroyed_) {
            return;
        }

        if (blockEventToClient_) {
            blockedEvents_.push_back(
                std::make_unique<E>(std::forward<Args>(args)...));
        } else {
            E event(std::forward<Args>(args)...);
            deliverEvent(event);
        }
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

    void deliverEvent(InputContextEvent &icEvent) {
        FCITX_Q();
        if (destroyed_) {
            return;
        }
        switch (icEvent.type()) {
        case EventType::InputContextCommitString: {
            auto &event = static_cast<CommitStringEvent &>(icEvent);
            if (!postEvent(event)) {
                if (auto instance = manager_.instance()) {
                    auto newString = instance->commitFilter(q, event.text());
                    q->commitStringImpl(newString);
                } else {
                    q->commitStringImpl(event.text());
                }
            }
            break;
        }
        case EventType::InputContextForwardKey: {
            auto &event = static_cast<ForwardKeyEvent &>(icEvent);
            if (!postEvent(event)) {
                q->forwardKeyImpl(event);
            }
            break;
        }
        default:
            break;
        }
    }

    void deliverBlockedEvents() {
        for (const auto &event : blockedEvents_) {
            deliverEvent(*event);
        }
        blockedEvents_.clear();
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

    std::list<std::unique_ptr<InputContextEvent>> blockedEvents_;
    bool blockEventToClient_ = false;
};
} // namespace fcitx

#endif // _FCITX_INPUTCONTEXT_P_H_
