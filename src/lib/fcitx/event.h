/*
 * Copyright (C) 2016~2016 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
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
#ifndef _FCITX_EVENT_H_
#define _FCITX_EVENT_H_

#include "fcitxcore_export.h"
#include <fcitx-utils/key.h>
#include <fcitx/userinterface.h>
#include <stdint.h>

namespace fcitx {

class InputContext;

enum class ResetReason { ChangeByInactivate, LostFocus, SwitchIM, Client };

enum class InputMethodSwitchedReason {
    Trigger,
    /**
     * when user press inactivate key, default behavior is commit raw preedit.
     * If you want to OVERRIDE this behavior, be sure to implement this
     * function.
     *
     * in some case, your implementation of OnClose should respect the value of
     * [Output/SendTextWhenSwitchEng], when this value is true, commit something
     * you
     * want.
     */
    Deactivate,
    AltTrigger,
    Activate,
    Enumerate,
    GroupChange,
    Other,
};

enum class EventType : uint32_t {
    EventTypeFlag = 0xffff0000,
    UserTypeFlag = 0xffff0000,
    InputContextEventFlag = 0x0001000,
    InputMethodEventFlag = 0x0002000,
    InstanceEventFlag = 0x0003000,
    // send by frontend, captured by core, input method, or module
    InputContextCreated = InputContextEventFlag | 0x1,
    InputContextDestroyed = InputContextEventFlag | 0x2,
    InputContextFocusIn = InputContextEventFlag | 0x3,
    /**
     * when using lost focus
     * this might be variance case to case. the default behavior is to commit
     * the preedit, and resetIM.
     *
     * Controlled by [Output/DontCommitPreeditWhenUnfocus], this option will not
     * work for application switch doesn't support async commit.
     *
     * So OnClose is called when preedit IS committed (not like
     * CChangeByInactivate,
     * this behavior cannot be overrided), it give im a chance to choose
     * remember this
     * word or not.
     *
     * Input method need to notice, that the commit is already DONE, do not do
     * extra commit.
     */
    InputContextFocusOut = InputContextEventFlag | 0x4,
    InputContextKeyEvent = InputContextEventFlag | 0x5,
    InputContextReset = InputContextEventFlag | 0x6,
    InputContextSurroundingTextUpdated = InputContextEventFlag | 0x7,
    InputContextCapabilityChanged = InputContextEventFlag | 0x8,
    InputContextCursorRectChanged = InputContextEventFlag | 0x9,
    /**
     * when user switch to a different input method by hand
     * such as ctrl+shift by default, or by ui,
     * default behavior is reset IM.
     */
    InputContextSwitchInputMethod = InputContextEventFlag | 0xA,

    // send by im, captured by frontend, or module
    InputContextForwardKey = InputMethodEventFlag | 0x1,
    InputContextCommitString = InputMethodEventFlag | 0x2,
    InputContextDeleteSurroundingText = InputMethodEventFlag | 0x3,
    InputContextUpdatePreedit = InputMethodEventFlag | 0x4,
    InputContextUpdateUI = InputMethodEventFlag | 0x5,

    // send by im or module, captured by ui TODO

    /**
     * captured by everything
     * This would also trigger InputContextSwitchInputMethod afterwards.
     */
    InputMethodGroupChanged = InstanceEventFlag | 0x1,
    InputMethodGroupAboutToReset = InstanceEventFlag | 0x2,
    InputMethodGroupReset = InstanceEventFlag | 0x3,
    NewInputMethodGroup = InstanceEventFlag | 0x4,
    InputMethodInitFailed = InstanceEventFlag | 0x5,
};

class FCITXCORE_EXPORT Event {
public:
    Event(EventType type) : type_(type) {}
    virtual ~Event();

    EventType type() const { return type_; }
    void accept() { accepted_ = true; }
    bool accepted() const { return accepted_; }
    virtual bool filtered() const { return false; }

    bool isInputContextEvent() const {
        auto flag = static_cast<uint32_t>(EventType::InputContextEventFlag);
        return (static_cast<uint32_t>(type_) & flag) == flag;
    }

protected:
    EventType type_;
    bool accepted_ = false;
};

class FCITXCORE_EXPORT InputContextEvent : public Event {
public:
    InputContextEvent(InputContext *context, EventType type)
        : Event(type), ic_(context) {}

    InputContext *inputContext() const { return ic_; }

protected:
    InputContext *ic_;
};

class FCITXCORE_EXPORT KeyEventBase : public InputContextEvent {
public:
    KeyEventBase(EventType type, InputContext *context, Key rawKey,
                 bool isRelease = false, int time = 0)
        : InputContextEvent(context, type), key_(rawKey.normalize()),
          rawKey_(rawKey), isRelease_(isRelease), time_(time) {}
    KeyEventBase(const KeyEventBase &) = default;

    Key key() const { return key_; }
    Key rawKey() const { return rawKey_; }
    bool isRelease() const { return isRelease_; }
    int time() const { return time_; }

protected:
    Key key_, rawKey_;
    bool isRelease_;
    int time_;
};

class FCITXCORE_EXPORT KeyEvent : public KeyEventBase {
public:
    KeyEvent(InputContext *context, Key rawKey, bool isRelease = false,
             int time = 0)
        : KeyEventBase(EventType::InputContextKeyEvent, context, rawKey,
                       isRelease, time) {}

    void filter() { filtered_ = true; }
    virtual bool filtered() const { return filtered_; }
    void filterAndAccept() {
        filter();
        accept();
    }

private:
    bool filtered_ = false;
};

class FCITXCORE_EXPORT ForwardKeyEvent : public KeyEventBase {
public:
    ForwardKeyEvent(InputContext *context, Key rawKey, bool isRelease = false,
                    int time = 0)
        : KeyEventBase(EventType::InputContextForwardKey, context, rawKey,
                       isRelease, time) {}
};

class FCITXCORE_EXPORT CommitStringEvent : public InputContextEvent {
public:
    CommitStringEvent(const std::string &text, InputContext *context)
        : InputContextEvent(context, EventType::InputContextCommitString),
          text_(text) {}

    const std::string text() const { return text_; }

protected:
    std::string text_;
};

class FCITXCORE_EXPORT InputContextSwitchInputMethodEvent
    : public InputContextEvent {
public:
    InputContextSwitchInputMethodEvent(InputMethodSwitchedReason reason,
                                       const std::string &oldIM,
                                       InputContext *context)
        : InputContextEvent(context, EventType::InputContextSwitchInputMethod),
          reason_(reason), oldInputMethod_(oldIM) {}

    InputMethodSwitchedReason reason() const { return reason_; }
    const std::string &oldInputMethod() const { return oldInputMethod_; }

protected:
    InputMethodSwitchedReason reason_;
    std::string oldInputMethod_;
};

class FCITXCORE_EXPORT ResetEvent : public InputContextEvent {
public:
    ResetEvent(ResetReason reason, InputContext *context)
        : InputContextEvent(context, EventType::InputContextReset),
          reason_(reason) {}

    ResetReason reason() const { return reason_; }

protected:
    ResetReason reason_;
};

class FCITXCORE_EXPORT InputContextUpdateUIEvent : public InputContextEvent {
public:
    InputContextUpdateUIEvent(UserInterfaceComponent component,
                              InputContext *context, bool immediate = false)
        : InputContextEvent(context, EventType::InputContextUpdateUI),
          component_(component), immediate_(immediate) {}

    UserInterfaceComponent component() const { return component_; }
    bool immediate() const { return immediate_; }

protected:
    UserInterfaceComponent component_;
    bool immediate_;
};

#define FCITX_DEFINE_SIMPLE_EVENT(NAME, TYPE, ARGS...)                         \
    struct FCITXCORE_EXPORT NAME##Event : public InputContextEvent {           \
        NAME##Event(InputContext *ic)                                          \
            : InputContextEvent(ic, EventType::TYPE) {}                        \
    }

FCITX_DEFINE_SIMPLE_EVENT(InputContextCreated, InputContextCreated);
FCITX_DEFINE_SIMPLE_EVENT(InputContextDestroyed, InputContextDestroyed);
FCITX_DEFINE_SIMPLE_EVENT(FocusIn, InputContextFocusIn);
FCITX_DEFINE_SIMPLE_EVENT(FocusOut, InputContextFocusOut);
FCITX_DEFINE_SIMPLE_EVENT(SurroundingTextUpdated,
                          InputContextSurroundingTextUpdated);
FCITX_DEFINE_SIMPLE_EVENT(CapabilityChanged, InputContextCapabilityChanged);
FCITX_DEFINE_SIMPLE_EVENT(CursorRectChanged, InputContextCursorRectChanged);
FCITX_DEFINE_SIMPLE_EVENT(UpdatePreedit, InputContextUpdatePreedit);

class InputMethodGroupChangedEvent : public Event {
public:
    InputMethodGroupChangedEvent()
        : Event(EventType::InputMethodGroupChanged) {}
};
}

#endif // _FCITX_EVENT_H_
