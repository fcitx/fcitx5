/*
 * SPDX-FileCopyrightText: 2016-2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "waylandimserver.h"
#include <sys/mman.h>
#include <fcitx-utils/utf8.h>
#include "waylandim.h"

#ifdef __linux__
#include <linux/input-event-codes.h>
#elif __FreeBSD__
#include <dev/evdev/input-event-codes.h>
#else
#define BTN_LEFT 0x110
#define BTN_RIGHT 0x111
#endif

namespace fcitx {

static inline unsigned int waylandFormat(TextFormatFlags flags) {
    unsigned int result = 0;
    if (flags & TextFormatFlag::Underline) {
        result |= ZWP_TEXT_INPUT_V1_PREEDIT_STYLE_UNDERLINE;
    }
    if (flags & TextFormatFlag::HighLight) {
        result |= ZWP_TEXT_INPUT_V1_PREEDIT_STYLE_SELECTION;
    }
    if (flags & TextFormatFlag::Bold) {
        result |= ZWP_TEXT_INPUT_V1_PREEDIT_STYLE_ACTIVE;
    }
    if (flags & TextFormatFlag::Strike) {
        result |= ZWP_TEXT_INPUT_V1_PREEDIT_STYLE_INCORRECT;
    }
    return result;
}

WaylandIMServer::WaylandIMServer(wl_display *display, FocusGroup *group,
                                 const std::string &name,
                                 WaylandIMModule *waylandim)
    : group_(group), name_(name), parent_(waylandim), inputMethodV1_(nullptr),
      display_(
          static_cast<wayland::Display *>(wl_display_get_user_data(display))) {
    display_->requestGlobals<wayland::ZwpInputMethodV1>();
    globalConn_ = display_->registry()->global().connect(
        [this](uint32_t, const char *interface, uint32_t) {
            if (0 == strcmp(interface, wayland::ZwpInputMethodV1::interface)) {
                init();
            }
        });

    init();
}

WaylandIMServer::~WaylandIMServer() { delete globalIc_; }
InputContextManager &WaylandIMServer::inputContextManager() {
    return parent_->instance()->inputContextManager();
}

Instance *WaylandIMServer::instance() { return parent_->instance(); }

void WaylandIMServer::init() {
    auto im = display_->getGlobal<wayland::ZwpInputMethodV1>();
    if (im && !inputMethodV1_) {
        inputMethodV1_ = im;
        globalIc_ = new WaylandIMInputContextV1(
            parent_->instance()->inputContextManager(), this);
        globalIc_->setFocusGroup(group_);
        inputMethodV1_->activate().connect(
            [this](wayland::ZwpInputMethodContextV1 *ic) { activate(ic); });
        inputMethodV1_->deactivate().connect(
            [this](wayland::ZwpInputMethodContextV1 *ic) { deactivate(ic); });
        display_->flush();
    }
}

void WaylandIMServer::activate(wayland::ZwpInputMethodContextV1 *id) {
    globalIc_->activate(id);
}

void WaylandIMServer::deactivate(wayland::ZwpInputMethodContextV1 *id) {
    globalIc_->deactivate(id);
}

WaylandIMInputContextV1::WaylandIMInputContextV1(
    InputContextManager &inputContextManager, WaylandIMServer *server)
    : InputContext(inputContextManager), server_(server) {
    timeEvent_ = server_->instance()->eventLoop().addTimeEvent(
        CLOCK_MONOTONIC, now(CLOCK_MONOTONIC), 0,
        [this](EventSourceTime *, uint64_t) {
            repeat();
            return true;
        });
    timeEvent_->setEnabled(false);
    created();
}

WaylandIMInputContextV1::~WaylandIMInputContextV1() { destroy(); }

void WaylandIMInputContextV1::activate(wayland::ZwpInputMethodContextV1 *ic) {
    ic_.reset(ic);
    ic_->surroundingText().connect(
        [this](const char *text, uint32_t cursor, uint32_t anchor) {
            surroundingTextCallback(text, cursor, anchor);
        });
    ic_->reset().connect([this]() { resetCallback(); });
    ic_->contentType().connect([this](uint32_t hint, uint32_t purpose) {
        contentTypeCallback(hint, purpose);
    });
    ic_->invokeAction().connect([this](uint32_t button, uint32_t index) {
        invokeActionCallback(button, index);
    });
    ic_->commitState().connect(
        [this](uint32_t serial) { commitStateCallback(serial); });
    ic_->preferredLanguage().connect(
        [](const char *language) { preferredLanguageCallback(language); });
    keyboard_.reset(ic_->grabKeyboard());
    keyboard_->keymap().connect(
        [this](uint32_t format, int32_t fd, uint32_t size) {
            keymapCallback(format, fd, size);
        });
    keyboard_->key().connect(
        [this](uint32_t serial, uint32_t time, uint32_t key, uint32_t state) {
            keyCallback(serial, time, key, state);
        });
    keyboard_->modifiers().connect(
        [this](uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched,
               uint32_t mods_locked, uint32_t group) {
            modifiersCallback(serial, mods_depressed, mods_latched, mods_locked,
                              group);
        });
    keyboard_->repeatInfo().connect([this](int32_t rate, int32_t delay) {
        repeatInfoCallback(rate, delay);
    });
    repeatInfoCallback(repeatRate_, repeatDelay_);
    server_->display_->sync();
    focusIn();
}

void WaylandIMInputContextV1::deactivate(wayland::ZwpInputMethodContextV1 *ic) {
    if (ic_.get() == ic) {
        ic_.reset();
        keyboard_.reset();
        timeEvent_->setEnabled(false);
        focusOut();
    } else {
        // This should not happen, but just in case.
        delete ic;
    }
}

void WaylandIMInputContextV1::repeat() {
    KeyEvent event(
        this,
        Key(repeatSym_, server_->modifiers_ | KeyState::Repeat, repeatKey_ + 8),
        false, repeatTime_);
    if (!keyEvent(event)) {
        ic_->keysym(serial_, repeatTime_, event.rawKey().sym(),
                    event.isRelease() ? WL_KEYBOARD_KEY_STATE_RELEASED
                                      : WL_KEYBOARD_KEY_STATE_PRESSED,
                    event.rawKey().states());
    }

    timeEvent_->setNextInterval(1000000 / repeatRate_);
    timeEvent_->setOneShot();
    server_->display_->flush();
}

void WaylandIMInputContextV1::surroundingTextCallback(const char *text,
                                                      uint32_t cursor,
                                                      uint32_t anchor) {
    surroundingText().setText(text, cursor, anchor);
    updateSurroundingText();
}
void WaylandIMInputContextV1::resetCallback() { reset(ResetReason::Client); }
void WaylandIMInputContextV1::contentTypeCallback(uint32_t hint,
                                                  uint32_t purpose) {
    CapabilityFlags flags;
    if (hint & ZWP_TEXT_INPUT_V1_CONTENT_HINT_PASSWORD) {
        flags |= CapabilityFlag::Password;
    }
    if (hint & ZWP_TEXT_INPUT_V1_CONTENT_HINT_AUTO_COMPLETION) {
        flags |= CapabilityFlag::WordCompletion;
    }
    if (hint & ZWP_TEXT_INPUT_V1_CONTENT_HINT_AUTO_CORRECTION) {
        flags |= CapabilityFlag::SpellCheck;
    }
    if (hint & ZWP_TEXT_INPUT_V1_CONTENT_HINT_AUTO_CAPITALIZATION) {
        flags |= CapabilityFlag::UppercaseWords;
    }
    if (hint & ZWP_TEXT_INPUT_V1_CONTENT_HINT_LOWERCASE) {
        flags |= CapabilityFlag::Lowercase;
    }
    if (hint & ZWP_TEXT_INPUT_V1_CONTENT_HINT_UPPERCASE) {
        flags |= CapabilityFlag::Uppercase;
    }
    if (hint & ZWP_TEXT_INPUT_V1_CONTENT_HINT_TITLECASE) {
        // ??
    }
    if (hint & ZWP_TEXT_INPUT_V1_CONTENT_HINT_HIDDEN_TEXT) {
        flags |= CapabilityFlag::Password;
    }
    if (hint & ZWP_TEXT_INPUT_V1_CONTENT_HINT_SENSITIVE_DATA) {
        flags |= CapabilityFlag::Sensitive;
    }
    if (hint & ZWP_TEXT_INPUT_V1_CONTENT_HINT_LATIN) {
        flags |= CapabilityFlag::Alpha;
    }
    if (hint & ZWP_TEXT_INPUT_V1_CONTENT_HINT_MULTILINE) {
        flags |= CapabilityFlag::Multiline;
    }
    if (purpose == ZWP_TEXT_INPUT_V1_CONTENT_PURPOSE_ALPHA) {
        flags |= CapabilityFlag::Alpha;
    }
    if (purpose == ZWP_TEXT_INPUT_V1_CONTENT_PURPOSE_DIGITS) {
        flags |= CapabilityFlag::Digit;
    }
    if (purpose == ZWP_TEXT_INPUT_V1_CONTENT_PURPOSE_NUMBER) {
        flags |= CapabilityFlag::Number;
    }
    if (purpose == ZWP_TEXT_INPUT_V1_CONTENT_PURPOSE_PASSWORD) {
        flags |= CapabilityFlag::Password;
    }
    if (purpose == ZWP_TEXT_INPUT_V1_CONTENT_PURPOSE_PHONE) {
        flags |= CapabilityFlag::Dialable;
    }
    if (purpose == ZWP_TEXT_INPUT_V1_CONTENT_PURPOSE_URL) {
        flags |= CapabilityFlag::Url;
    }
    if (purpose == ZWP_TEXT_INPUT_V1_CONTENT_PURPOSE_EMAIL) {
        flags |= CapabilityFlag::Email;
    }
    if (purpose == ZWP_TEXT_INPUT_V1_CONTENT_PURPOSE_NAME) {
        flags |= CapabilityFlag::Name;
    }
    if (purpose == ZWP_TEXT_INPUT_V1_CONTENT_PURPOSE_DATE) {
        flags |= CapabilityFlag::Date;
    }
    if (purpose == ZWP_TEXT_INPUT_V1_CONTENT_PURPOSE_TIME) {
        flags |= CapabilityFlag::Time;
    }
    if (purpose == ZWP_TEXT_INPUT_V1_CONTENT_PURPOSE_DATETIME) {
        flags |= CapabilityFlag::Date;
        flags |= CapabilityFlag::Time;
    }
    if (purpose == ZWP_TEXT_INPUT_V1_CONTENT_PURPOSE_TERMINAL) {
        flags |= CapabilityFlag::Terminal;
    }
    setCapabilityFlags(flags);
}
void WaylandIMInputContextV1::invokeActionCallback(uint32_t button,
                                                   uint32_t index) {
    InvokeActionEvent::Action action;
    switch (button) {
    case BTN_LEFT:
        action = InvokeActionEvent::Action::LeftClick;
        break;
    case BTN_RIGHT:
        action = InvokeActionEvent::Action::LeftClick;
        break;
    default:
        return;
    }
    InvokeActionEvent event(action, index, this);
    invokeAction(event);
}
void WaylandIMInputContextV1::commitStateCallback(uint32_t serial) {
    serial_ = serial;
}
void WaylandIMInputContextV1::preferredLanguageCallback(const char *language) {
    FCITX_UNUSED(language);
}

void WaylandIMInputContextV1::keymapCallback(uint32_t format, int32_t fd,
                                             uint32_t size) {
    FCITX_UNUSED(size);
    if (!server_->context_) {
        server_->context_.reset(xkb_context_new(XKB_CONTEXT_NO_FLAGS));
        xkb_context_set_log_level(server_->context_.get(),
                                  XKB_LOG_LEVEL_CRITICAL);
    }

    if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
        close(fd);
        return;
    }

    if (server_->keymap_) {
        server_->keymap_.reset();
    }

    auto *mapStr = mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0);
    if (mapStr == MAP_FAILED) {
        close(fd);
        return;
    }

    server_->keymap_.reset(xkb_keymap_new_from_string(
        server_->context_.get(), static_cast<const char *>(mapStr),
        XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS));

    munmap(mapStr, size);
    close(fd);

    if (!server_->keymap_) {
        return;
    }

    server_->state_.reset(xkb_state_new(server_->keymap_.get()));
    if (!server_->state_) {
        server_->keymap_.reset();
        return;
    }

    server_->stateMask_.shift_mask =
        1 << xkb_keymap_mod_get_index(server_->keymap_.get(), "Shift");
    server_->stateMask_.lock_mask =
        1 << xkb_keymap_mod_get_index(server_->keymap_.get(), "Lock");
    server_->stateMask_.control_mask =
        1 << xkb_keymap_mod_get_index(server_->keymap_.get(), "Control");
    server_->stateMask_.mod1_mask =
        1 << xkb_keymap_mod_get_index(server_->keymap_.get(), "Mod1");
    server_->stateMask_.mod2_mask =
        1 << xkb_keymap_mod_get_index(server_->keymap_.get(), "Mod2");
    server_->stateMask_.mod3_mask =
        1 << xkb_keymap_mod_get_index(server_->keymap_.get(), "Mod3");
    server_->stateMask_.mod4_mask =
        1 << xkb_keymap_mod_get_index(server_->keymap_.get(), "Mod4");
    server_->stateMask_.mod5_mask =
        1 << xkb_keymap_mod_get_index(server_->keymap_.get(), "Mod5");
    server_->stateMask_.super_mask =
        1 << xkb_keymap_mod_get_index(server_->keymap_.get(), "Super");
    server_->stateMask_.hyper_mask =
        1 << xkb_keymap_mod_get_index(server_->keymap_.get(), "Hyper");
    server_->stateMask_.meta_mask =
        1 << xkb_keymap_mod_get_index(server_->keymap_.get(), "Meta");
}

void WaylandIMInputContextV1::keyCallback(uint32_t serial, uint32_t time,
                                          uint32_t key, uint32_t state) {
    time_ = time;
    if (!server_->state_) {
        return;
    }

    // EVDEV OFFSET
    uint32_t code = key + 8;

    KeyEvent event(this,
                   Key(static_cast<KeySym>(xkb_state_key_get_one_sym(
                           server_->state_.get(), code)),
                       server_->modifiers_, code),
                   state == WL_KEYBOARD_KEY_STATE_RELEASED, time);

    if (state == WL_KEYBOARD_KEY_STATE_RELEASED && key == repeatKey_) {
        timeEvent_->setEnabled(false);
    } else if (state == WL_KEYBOARD_KEY_STATE_PRESSED &&
               xkb_keymap_key_repeats(server_->keymap_.get(), code)) {
        if (repeatRate_) {
            repeatKey_ = key;
            repeatTime_ = time;
            repeatSym_ = event.rawKey().sym();
            timeEvent_->setNextInterval(repeatDelay_ * 1000);
            timeEvent_->setOneShot();
        }
    }

    WAYLANDIM_DEBUG() << event.key().toString()
                      << " IsRelease=" << event.isRelease();
    if (!keyEvent(event)) {
        ic_->key(serial, time, key, state);
    }
    server_->display_->flush();
}
void WaylandIMInputContextV1::modifiersCallback(uint32_t serial,
                                                uint32_t mods_depressed,
                                                uint32_t mods_latched,
                                                uint32_t mods_locked,
                                                uint32_t group) {
    if (!server_->state_) {
        return;
    }

    xkb_mod_mask_t mask;

    xkb_state_update_mask(server_->state_.get(), mods_depressed, mods_latched,
                          mods_locked, 0, 0, group);
    server_->instance()->updateXkbStateMask(
        server_->group()->display(), mods_depressed, mods_latched, mods_locked);
    mask = xkb_state_serialize_mods(
        server_->state_.get(), static_cast<xkb_state_component>(
                                   XKB_STATE_DEPRESSED | XKB_STATE_LATCHED));

    server_->modifiers_ = 0;
    if (mask & server_->stateMask_.shift_mask) {
        server_->modifiers_ |= KeyState::Shift;
    }
    if (mask & server_->stateMask_.lock_mask) {
        server_->modifiers_ |= KeyState::CapsLock;
    }
    if (mask & server_->stateMask_.control_mask) {
        server_->modifiers_ |= KeyState::Ctrl;
    }
    if (mask & server_->stateMask_.mod1_mask) {
        server_->modifiers_ |= KeyState::Alt;
    }
    if (mask & server_->stateMask_.super_mask) {
        server_->modifiers_ |= KeyState::Super;
    }
    if (mask & server_->stateMask_.hyper_mask) {
        server_->modifiers_ |= KeyState::Hyper;
    }
    if (mask & server_->stateMask_.meta_mask) {
        server_->modifiers_ |= KeyState::Meta;
    }

    ic_->modifiers(serial, mods_depressed, mods_depressed, mods_latched, group);
}

void WaylandIMInputContextV1::repeatInfoCallback(int32_t rate, int32_t delay) {
    repeatRate_ = rate;
    repeatDelay_ = delay;
    timeEvent_->setAccuracy(std::min(delay * 1000, 1000000 / rate));
}

void WaylandIMInputContextV1::updatePreeditImpl() {
    auto preedit =
        server_->instance()->outputFilter(this, inputPanel().clientPreedit());

    for (int i = 0, e = preedit.size(); i < e; i++) {
        if (!utf8::validate(preedit.stringAt(i))) {
            return;
        }
    }

    ic_->preeditCursor(preedit.cursor());
    ic_->preeditString(serial_, preedit.toString().c_str(),
                       preedit.toStringForCommit().c_str());
    unsigned int index = 0;
    for (int i = 0, e = preedit.size(); i < e; i++) {
        ic_->preeditStyling(index, preedit.stringAt(i).size(),
                            waylandFormat(preedit.formatAt(i)));
        index += preedit.stringAt(i).size();
    }
}
} // namespace fcitx
