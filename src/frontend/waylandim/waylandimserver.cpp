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

constexpr CapabilityFlags baseFlags{CapabilityFlag::Preedit,
                                    CapabilityFlag::FormattedPreedit,
                                    CapabilityFlag::SurroundingText};

static inline unsigned int waylandFormat(TextFormatFlags flags) {
    if (flags & TextFormatFlag::HighLight) {
        return ZWP_TEXT_INPUT_V1_PREEDIT_STYLE_HIGHLIGHT;
    }
    if (flags & TextFormatFlag::Bold) {
        return ZWP_TEXT_INPUT_V1_PREEDIT_STYLE_ACTIVE;
    }
    if (flags & TextFormatFlag::Strike) {
        return ZWP_TEXT_INPUT_V1_PREEDIT_STYLE_INCORRECT;
    }
    if (flags & TextFormatFlag::Underline) {
        return ZWP_TEXT_INPUT_V1_PREEDIT_STYLE_DEFAULT;
    }
    return ZWP_TEXT_INPUT_V1_PREEDIT_STYLE_NONE;
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
        globalIc_->setCapabilityFlags(baseFlags);
        inputMethodV1_->activate().connect(
            [this](wayland::ZwpInputMethodContextV1 *ic) {
                WAYLANDIM_DEBUG() << "ACTIVATE " << ic;
                activate(ic);
            });
        inputMethodV1_->deactivate().connect(
            [this](wayland::ZwpInputMethodContextV1 *ic) {
                WAYLANDIM_DEBUG() << "DEACTIVATE " << ic;
                deactivate(ic);
            });
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
    wl_array array;
    wl_array_init(&array);
    constexpr char data[] = "Shift\0Control\0Mod1\0Mod4";
    wl_array_add(&array, sizeof(data));
    memcpy(array.data, data, sizeof(data));
    ic_->modifiersMap(&array);
    wl_array_release(&array);
    focusIn();
}

void WaylandIMInputContextV1::deactivate(wayland::ZwpInputMethodContextV1 *ic) {
    if (ic_.get() == ic) {
        ic_.reset();
        keyboard_.reset();
        timeEvent_->setEnabled(false);
        // If last key to vk is press, send a release.
        if (lastVKKey_ && lastVKState_ == WL_KEYBOARD_KEY_STATE_PRESSED) {
            sendKeyToVK(lastVKTime_, lastVKKey_,
                        WL_KEYBOARD_KEY_STATE_RELEASED);
            lastVKTime_ = lastVKKey_ = 0;
            lastVKState_ = WL_KEYBOARD_KEY_STATE_RELEASED;
        }
        server_->display_->sync();
        focusOut();
    } else {
        // This should not happen, but just in case.
        delete ic;
    }
}

void WaylandIMInputContextV1::repeat() {
    if (!ic_ || !hasFocus()) {
        return;
    }
    KeyEvent event(
        this,
        Key(repeatSym_, server_->modifiers_ | KeyState::Repeat, repeatKey_ + 8),
        false, repeatTime_);

    sendKeyToVK(repeatTime_, event.rawKey().code() - 8,
                WL_KEYBOARD_KEY_STATE_RELEASED);
    if (!keyEvent(event)) {
        sendKeyToVK(repeatTime_, event.rawKey().code() - 8,
                    WL_KEYBOARD_KEY_STATE_PRESSED);
    }

    timeEvent_->setNextInterval(1000000 / repeatRate_);
    timeEvent_->setOneShot();
    server_->display_->flush();
}

void WaylandIMInputContextV1::surroundingTextCallback(const char *text,
                                                      uint32_t cursor,
                                                      uint32_t anchor) {
    std::string_view textView(text);
    if (cursor > textView.size() || anchor > textView.size() ||
        !utf8::validate(textView)) {
        return;
    }
    surroundingText().setText(text, utf8::length(textView, 0, cursor),
                              utf8::length(textView, 0, anchor));
    updateSurroundingText();
}
void WaylandIMInputContextV1::resetCallback() { reset(); }
void WaylandIMInputContextV1::contentTypeCallback(uint32_t hint,
                                                  uint32_t purpose) {
    CapabilityFlags flags = baseFlags;
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
        action = InvokeActionEvent::Action::RightClick;
        break;
    default:
        return;
    }
    InvokeActionEvent event(action, index, this);
    if (!hasFocus()) {
        focusIn();
    }
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
    if (!ic_) {
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
            // Let's trick the key event system by fake our first.
            // Remove 100 from the initial interval.
            timeEvent_->setNextInterval(repeatDelay_ * 1000 - 100);
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
    FCITX_UNUSED(serial);
    if (!server_->state_) {
        return;
    }

    xkb_mod_mask_t mask;

    xkb_state_update_mask(server_->state_.get(), mods_depressed, mods_latched,
                          mods_locked, 0, 0, group);
    server_->instance()->updateXkbStateMask(
        server_->group()->display(), mods_depressed, mods_latched, mods_locked);
    mask = xkb_state_serialize_mods(server_->state_.get(),
                                    XKB_STATE_MODS_EFFECTIVE);

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
    if (mask & server_->stateMask_.mod2_mask) {
        server_->modifiers_ |= KeyState::NumLock;
    }
    if (mask & server_->stateMask_.super_mask) {
        server_->modifiers_ |= KeyState::Super;
    }
    if (mask & server_->stateMask_.mod4_mask) {
        server_->modifiers_ |= KeyState::Super;
    }
    if (mask & server_->stateMask_.hyper_mask) {
        server_->modifiers_ |= KeyState::Hyper;
    }
    if (mask & server_->stateMask_.mod3_mask) {
        server_->modifiers_ |= KeyState::Hyper;
    }
    if (mask & server_->stateMask_.mod5_mask) {
        server_->modifiers_ |= KeyState::Mod5;
    }
    if (mask & server_->stateMask_.meta_mask) {
        server_->modifiers_ |= KeyState::Meta;
    }
}

void WaylandIMInputContextV1::repeatInfoCallback(int32_t rate, int32_t delay) {
    repeatRate_ = rate;
    repeatDelay_ = delay;
    timeEvent_->setAccuracy(std::min(delay * 1000, 1000000 / rate));
}

void WaylandIMInputContextV1::sendKey(uint32_t time, uint32_t sym,
                                      uint32_t state, KeyStates states) {
    if (!ic_) {
        return;
    }
    auto modifiers = toModifiers(states);
    ic_->keysym(serial_, time, sym, state, modifiers);
}

void WaylandIMInputContextV1::sendKeyToVK(uint32_t time, uint32_t key,
                                          uint32_t state) {
    if (!ic_) {
        return;
    }
    lastVKKey_ = key;
    lastVKState_ = state;
    lastVKTime_ = time;
    ic_->key(serial_, time, key, state);
    server_->display_->roundtrip();
}

void WaylandIMInputContextV1::updatePreeditImpl() {
    if (!ic_) {
        return;
    }

    auto preedit =
        server_->instance()->outputFilter(this, inputPanel().clientPreedit());

    for (int i = 0, e = preedit.size(); i < e; i++) {
        if (!utf8::validate(preedit.stringAt(i))) {
            return;
        }
    }

    ic_->preeditCursor(preedit.cursor());
    unsigned int index = 0;
    for (int i = 0, e = preedit.size(); i < e; i++) {
        ic_->preeditStyling(index, preedit.stringAt(i).size(),
                            waylandFormat(preedit.formatAt(i)));
        index += preedit.stringAt(i).size();
    }
    ic_->preeditString(serial_, preedit.toString().c_str(),
                       preedit.toStringForCommit().c_str());
}

void WaylandIMInputContextV1::deleteSurroundingTextImpl(int offset,
                                                        unsigned int size) {
    if (!ic_) {
        return;
    }

    size_t cursor = surroundingText().cursor();
    if (static_cast<ssize_t>(cursor) + offset < 0) {
        return;
    }

    const auto &text = surroundingText().text();
    auto len = utf8::length(text);

    size_t start = cursor + offset;
    size_t end = cursor + offset + size;
    if (cursor > len || start > len || end > len) {
        return;
    }

    auto startBytes = utf8::ncharByteLength(text.begin(), start);
    auto cursorBytes = utf8::ncharByteLength(text.begin(), cursor);
    auto sizeBytes = utf8::ncharByteLength(text.begin() + startBytes, size);
    ic_->deleteSurroundingText(startBytes - cursorBytes, sizeBytes);
    ic_->commitString(serial_, "");
}
} // namespace fcitx
