/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_EVENT_H_
#define _FCITX_EVENT_H_

#include <cstdint>
#include <fcitx-utils/capabilityflags.h>
#include <fcitx-utils/key.h>
#include <fcitx/fcitxcore_export.h>
#include <fcitx/userinterface.h>

/// \addtogroup FcitxCore
/// \{
/// \file
/// \brief Input Method event for Fcitx.

namespace fcitx {

class InputContext;
class FocusGroup;

enum class ResetReason {
    ChangeByInactivate FCITXCORE_DEPRECATED,
    LostFocus FCITXCORE_DEPRECATED,
    SwitchIM FCITXCORE_DEPRECATED,
    Client
};

/**
 * The reason why input method is switched to another.
 */
enum class InputMethodSwitchedReason {
    /// Switched by trigger key
    Trigger,
    /// Switched by deactivate key
    Deactivate,
    /// Switched by alternative trigger key
    AltTrigger,
    /// Switched by activate key
    Activate,
    /// Switched by enumerate key
    Enumerate,
    /// Switched by group change
    GroupChange,
    /// Switched by capability change (e.g. password field)
    CapabilityChanged,
    /// miscellaneous reason
    Other,
};

enum class InputMethodMode { PhysicalKeyboard, OnScreenKeyboard };

/**
 * Type of input method events.
 */
enum class EventType : uint32_t {
    EventTypeFlag = 0xfffff000,
    UserTypeFlag = 0xffff0000,
    // send by frontend, captured by core, input method, or module
    InputContextEventFlag = 0x0001000,
    // send by im, captured by frontend, or module
    InputMethodEventFlag = 0x0002000,

    /**
     * captured by everything
     */
    InstanceEventFlag = 0x0003000,
    InputContextCreated = InputContextEventFlag | 0x1,
    InputContextDestroyed = InputContextEventFlag | 0x2,
    /**
     * FocusInEvent is generated when client gets focused.
     *
     * @see FocusInEvent
     */
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
     * this behavior cannot be overridden), it give im a chance to choose
     * remember this
     * word or not.
     *
     * Input method need to notice, that the commit is already DONE, do not do
     * extra commit.
     *
     * @see FocusOutEvent
     */
    InputContextFocusOut = InputContextEventFlag | 0x4,
    /**
     * Key event is generated when client press or release a key.
     * @see KeyEvent
     */
    InputContextKeyEvent = InputContextEventFlag | 0x5,
    /**
     * ResetEvent is generated
     *
     */
    InputContextReset = InputContextEventFlag | 0x6,
    InputContextSurroundingTextUpdated = InputContextEventFlag | 0x7,
    InputContextCapabilityChanged = InputContextEventFlag | 0x8,
    InputContextCursorRectChanged = InputContextEventFlag | 0x9,
    InputContextCapabilityAboutToChange = InputContextEventFlag | 0xD,
    /**
     * when user switch to a different input method by hand
     * such as ctrl+shift by default, or by ui,
     * default behavior is reset IM.
     */
    InputContextSwitchInputMethod = InputContextEventFlag | 0xA,

    // Two convenient event after input method is actually activate and
    // deactivated. Useful for UI to update after specific input method get
    // activated and deactivated. Will not be emitted if the input method is not
    // a valid one.
    InputContextInputMethodActivated = InputContextEventFlag | 0xB,
    InputContextInputMethodDeactivated = InputContextEventFlag | 0xC,
    /**
     * InvokeAction event is generated when client click on the preedit.
     *
     * Not all client support this feature.
     *
     * @since 5.0.11
     */
    InputContextInvokeAction = InputContextEventFlag | 0xE,

    InputContextVirtualKeyboardEvent = InputContextEventFlag | 0xF,

    InputContextForwardKey = InputMethodEventFlag | 0x1,
    InputContextCommitString = InputMethodEventFlag | 0x2,
    InputContextDeleteSurroundingText = InputMethodEventFlag | 0x3,
    InputContextUpdatePreedit = InputMethodEventFlag | 0x4,
    InputContextUpdateUI = InputMethodEventFlag | 0x5,
    InputContextCommitStringWithCursor = InputMethodEventFlag | 0x6,
    InputContextFlushUI = InputMethodEventFlag | 0x7,

    /**
     * This is generated when input method group changed.
     * This would also trigger InputContextSwitchInputMethod afterwards.
     */
    InputMethodGroupChanged = InstanceEventFlag | 0x1,
    /**
     * InputMethodGroupAboutToChangeEvent is generated when input method group
     * is about to be changed.
     *
     * @see InputMethodGroupAboutToChange
     */
    InputMethodGroupAboutToChange = InstanceEventFlag | 0x2,
    /**
     * UIChangedEvent is posted when the UI implementation is changed.
     */
    UIChanged = InstanceEventFlag | 0x3,
    /**
     * CheckUpdateEvent is posted when the Instance is requested to check for
     * newly installed addons and input methods.
     *
     * This can be used for addons to pick up new input methods if it provides
     * input method at runtime.
     */
    CheckUpdate = InstanceEventFlag | 0x4,
    /**
     * FocusGroupFocusChanged is posted when a focus group changed its focused
     * input context.
     *
     * This is a more fine grained control over focus in and focus out event.
     * This is more useful for UI to keep track of what input context is being
     * focused.
     *
     * @see FocusInEvent
     * @see FocusOutEvent
     */
    FocusGroupFocusChanged = InstanceEventFlag | 0x5,
    /**
     * Input method mode changed
     */
    InputMethodModeChanged = InstanceEventFlag | 0x6,
    /**
     * Global config is reloaded
     *
     * This only fires after fcitx has entered running state.
     * The initial load will not trigger this event.
     * @see GlobalConfig
     * @since 5.1.0
     */
    GlobalConfigReloaded = InstanceEventFlag | 0x7,
    /**
     * Virtual keyboard visibility changed
     *
     * @see UserInterfaceManager
     * @since 5.1.0
     */
    VirtualKeyboardVisibilityChanged = InstanceEventFlag | 0x8,
};

/**
 * Base class for fcitx event.
 */
class FCITXCORE_EXPORT Event {
public:
    Event(EventType type) : type_(type) {}
    virtual ~Event();

    /**
     * Type of event, can be used to decide event class.
     *
     * @return fcitx::EventType
     */
    EventType type() const { return type_; }

    void accept() { accepted_ = true; }
    /**
     * Return value used by Instance::postEvent.
     *
     * @see Instance::postEvent
     * @return bool
     */
    bool accepted() const { return accepted_; }

    /**
     * Whether a event is filtered by handler.
     *
     * If event is filtered, it will not send to another handler.
     * For now only keyevent from input context can be filtered.
     *
     * @return bool
     */
    virtual bool filtered() const { return false; }

    /**
     * A helper function to check if a event is input context event.
     *
     * @return bool
     */
    bool isInputContextEvent() const {
        auto flag = static_cast<uint32_t>(EventType::InputContextEventFlag);
        auto mask = static_cast<uint32_t>(EventType::EventTypeFlag);
        return (static_cast<uint32_t>(type_) & mask) == flag;
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
                 bool isRelease = false, int time = 0);
    KeyEventBase(const KeyEventBase &) = default;

    /**
     * Normalized key event.
     *
     * @return fcitx::Key
     */
    Key key() const { return key_; }

    /**
     * It will automatically be called if input method layout does not match the
     * system keyboard layout.
     *
     * @param key p_key:...
     */
    void setKey(const Key &key) {
        key_ = key;
        forward_ = true;
    }

    /**
     * It is designed for faking the key event. Normally should not be used.
     *
     * @param key key event to override.
     * @since 5.0.4
     */
    void setRawKey(const Key &key) {
        rawKey_ = key;
        key_ = key.normalize();
        forward_ = true;
    }

    /**
     * It is designed for overriding the key forward option. Normally should not
     * be used.
     *
     * @param forward
     * @since 5.0.4
     */
    void setForward(bool forward) { forward_ = forward; }

    /**
     * Key event regardless of keyboard layout conversion.
     *
     * @return fcitx::Key
     */
    Key origKey() const { return origKey_; }

    /**
     * Key event after layout conversion.
     *
     * Basically it is the "unnormalized" key event.
     *
     * @return fcitx::Key
     */
    Key rawKey() const { return rawKey_; }
    bool isRelease() const { return isRelease_; }
    int time() const { return time_; }

    /**
     * If true, the key that produce character will commit a string.
     *
     * This is currently used by internal keyboard layout translation.
     *
     * @return bool
     */
    bool forward() const { return forward_; }

    /**
     * Whether this key event is derived from a virtual keyboard
     *
     * @return bool
     * @since 5.1.2
     */
    bool isVirtual() const { return origKey_.isVirtual(); }

protected:
    Key key_, origKey_, rawKey_;
    bool isRelease_;
    int time_;
    bool forward_ = false;
};

class FCITXCORE_EXPORT KeyEvent : public KeyEventBase {
public:
    KeyEvent(InputContext *context, Key rawKey, bool isRelease = false,
             int time = 0)
        : KeyEventBase(EventType::InputContextKeyEvent, context, rawKey,
                       isRelease, time) {}

    void filter() { filtered_ = true; }
    bool filtered() const override { return filtered_; }
    void filterAndAccept() {
        filter();
        accept();
    }

private:
    bool filtered_ = false;
};

class VirtualKeyboardEventPrivate;

class FCITXCORE_EXPORT VirtualKeyboardEvent : public InputContextEvent {
public:
    VirtualKeyboardEvent(InputContext *context, bool isRelease, int time = 0);
    ~VirtualKeyboardEvent();

    int time() const;

    void setKey(Key key);
    const Key &key() const;

    void setPosition(float x, float y);
    float x() const;
    float y() const;

    void setLongPress(bool longPress);
    bool isLongPress() const;

    void setUserAction(uint64_t actionId);
    uint64_t userAction() const;

    void setText(std::string text);
    const std::string &text() const;

    std::unique_ptr<KeyEvent> toKeyEvent() const;

protected:
    FCITX_DECLARE_PRIVATE(VirtualKeyboardEvent);
    std::unique_ptr<VirtualKeyboardEventPrivate> d_ptr;
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

    const std::string &text() const { return text_; }

protected:
    std::string text_;
};

/**
 * Event for commit string with cursor
 *
 * @see InputContext::commitStringWithCursor
 * @since 5.1.2
 */
class FCITXCORE_EXPORT CommitStringWithCursorEvent : public InputContextEvent {
public:
    CommitStringWithCursorEvent(std::string text, size_t cursor,
                                InputContext *context)
        : InputContextEvent(context,
                            EventType::InputContextCommitStringWithCursor),
          text_(std::move(text)), cursor_(cursor) {}

    const std::string &text() const { return text_; }
    size_t cursor() const { return cursor_; }

protected:
    std::string text_;
    size_t cursor_;
};

class FCITXCORE_EXPORT InvokeActionEvent : public InputContextEvent {
public:
    enum class Action { LeftClick, RightClick };
    InvokeActionEvent(Action action, int cursor, InputContext *context)
        : InputContextEvent(context, EventType::InputContextInvokeAction),
          action_(action), cursor_(cursor) {}

    Action action() const { return action_; }
    int cursor() const { return cursor_; }

    void filter() {
        filtered_ = true;
        accept();
    }
    bool filtered() const override { return filtered_; }

protected:
    Action action_;
    int cursor_;
    bool filtered_ = false;
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
    FCITXCORE_DEPRECATED ResetEvent(ResetReason reason, InputContext *context)
        : InputContextEvent(context, EventType::InputContextReset),
          reason_(reason) {}
    ResetEvent(InputContext *context)
        : InputContextEvent(context, EventType::InputContextReset),
          reason_(ResetReason::Client) {}

    FCITXCORE_DEPRECATED ResetReason reason() const { return reason_; }

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

/**
 * Events triggered that user interface manager that flush the UI update.
 *
 * @since 5.1.2
 */
class FCITXCORE_EXPORT InputContextFlushUIEvent : public InputContextEvent {
public:
    InputContextFlushUIEvent(UserInterfaceComponent component,
                             InputContext *context)
        : InputContextEvent(context, EventType::InputContextFlushUI),
          component_(component) {}

    UserInterfaceComponent component() const { return component_; }

protected:
    UserInterfaceComponent component_;
};

class FCITXCORE_EXPORT VirtualKeyboardVisibilityChangedEvent : public Event {
public:
    VirtualKeyboardVisibilityChangedEvent()
        : Event(EventType::VirtualKeyboardVisibilityChanged) {}
};

class FCITXCORE_EXPORT InputMethodNotificationEvent : public InputContextEvent {
public:
    InputMethodNotificationEvent(EventType type, const std::string &name,
                                 InputContext *context)
        : InputContextEvent(context, type), name_(name) {}

    const std::string &name() const { return name_; }

protected:
    std::string name_;
};

class FCITXCORE_EXPORT InputMethodActivatedEvent
    : public InputMethodNotificationEvent {
public:
    InputMethodActivatedEvent(const std::string &name, InputContext *context)
        : InputMethodNotificationEvent(
              EventType::InputContextInputMethodActivated, name, context) {}
};

class FCITXCORE_EXPORT InputMethodDeactivatedEvent
    : public InputMethodNotificationEvent {
public:
    InputMethodDeactivatedEvent(const std::string &name, InputContext *context)
        : InputMethodNotificationEvent(
              EventType::InputContextInputMethodDeactivated, name, context) {}
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
FCITX_DEFINE_SIMPLE_EVENT(CursorRectChanged, InputContextCursorRectChanged);
FCITX_DEFINE_SIMPLE_EVENT(UpdatePreedit, InputContextUpdatePreedit);

class FCITXCORE_EXPORT InputMethodGroupChangedEvent : public Event {
public:
    InputMethodGroupChangedEvent()
        : Event(EventType::InputMethodGroupChanged) {}
};

class FCITXCORE_EXPORT InputMethodGroupAboutToChangeEvent : public Event {
public:
    InputMethodGroupAboutToChangeEvent()
        : Event(EventType::InputMethodGroupAboutToChange) {}
};

class FCITXCORE_EXPORT UIChangedEvent : public Event {
public:
    UIChangedEvent() : Event(EventType::UIChanged) {}
};

class FCITXCORE_EXPORT CheckUpdateEvent : public Event {
public:
    CheckUpdateEvent() : Event(EventType::CheckUpdate) {}

    /// Make checking update short circuit. If anything need a refresh, just
    /// simply break.
    void setHasUpdate() {
        filtered_ = true;
        accept();
    }
    bool filtered() const override { return filtered_; }

private:
    bool filtered_ = false;
};

/**
 * Notify a focus change for focus group.
 *
 * @since 5.0.11
 */
class FCITXCORE_EXPORT FocusGroupFocusChangedEvent : public Event {
public:
    FocusGroupFocusChangedEvent(FocusGroup *group, InputContext *oldFocus,
                                InputContext *newFocus)
        : Event(EventType::FocusGroupFocusChanged), group_(group),
          oldFocus_(oldFocus), newFocus_(newFocus) {}

    FocusGroup *group() const { return group_; }
    InputContext *oldFocus() const { return oldFocus_; };
    InputContext *newFocus() const { return newFocus_; };

private:
    FocusGroup *group_;
    InputContext *oldFocus_;
    InputContext *newFocus_;
};

/**
 * Notify the input method mode is changed.
 *
 * @see Instance::InputMethodMode
 * @since 5.1.0
 */
class FCITXCORE_EXPORT InputMethodModeChangedEvent : public Event {
public:
    InputMethodModeChangedEvent() : Event(EventType::InputMethodModeChanged) {}
};

/**
 * Notify the global config is reloaded.
 *
 * @see GlobalConfig
 * @since 5.1.0
 */
class FCITXCORE_EXPORT GlobalConfigReloadedEvent : public Event {
public:
    GlobalConfigReloadedEvent() : Event(EventType::GlobalConfigReloaded) {}
};

class FCITXCORE_EXPORT CapabilityEvent : public InputContextEvent {
public:
    CapabilityEvent(InputContext *ic, EventType type, CapabilityFlags oldFlags,
                    CapabilityFlags newFlags)
        : InputContextEvent(ic, type), oldFlags_(oldFlags),
          newFlags_(newFlags) {}

    auto oldFlags() const { return oldFlags_; }
    auto newFlags() const { return newFlags_; }

protected:
    const CapabilityFlags oldFlags_;
    const CapabilityFlags newFlags_;
};

class FCITXCORE_EXPORT CapabilityChangedEvent : public CapabilityEvent {
public:
    CapabilityChangedEvent(InputContext *ic, CapabilityFlags oldFlags,
                           CapabilityFlags newFlags)
        : CapabilityEvent(ic, EventType::InputContextCapabilityChanged,
                          oldFlags, newFlags) {}
};

class FCITXCORE_EXPORT CapabilityAboutToChangeEvent : public CapabilityEvent {
public:
    CapabilityAboutToChangeEvent(InputContext *ic, CapabilityFlags oldFlags,
                                 CapabilityFlags newFlags)
        : CapabilityEvent(ic, EventType::InputContextCapabilityAboutToChange,
                          oldFlags, newFlags) {}
};

} // namespace fcitx

#endif // _FCITX_EVENT_H_
