/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "waylandimserverv2.h"
#include <sys/mman.h>
#include "fcitx-utils/unixfd.h"
#include "fcitx-utils/utf8.h"
#include "wayland-text-input-unstable-v3-client-protocol.h"
#include "waylandim.h"
#include "wl_seat.h"

namespace fcitx {

constexpr CapabilityFlags baseFlags{CapabilityFlag::Preedit,
                                    CapabilityFlag::FormattedPreedit,
                                    CapabilityFlag::SurroundingText};

WaylandIMServerV2::WaylandIMServerV2(wl_display *display, FocusGroup *group,
                                     const std::string &name,
                                     WaylandIMModule *waylandim)
    : group_(group), name_(name), parent_(waylandim),
      inputMethodManagerV2_(nullptr), display_(static_cast<wayland::Display *>(
                                          wl_display_get_user_data(display))) {
    display_->requestGlobals<wayland::ZwpInputMethodManagerV2>();
    display_->requestGlobals<wayland::ZwpVirtualKeyboardManagerV1>();
    display_->requestGlobals<wayland::WlSeat>();
    WAYLANDIM_DEBUG() << "WAYLANDIM V2";
    globalConn_ = display_->globalCreated().connect(
        [this](const std::string &interface, const std::shared_ptr<void> &) {
            if (interface == wayland::ZwpInputMethodManagerV2::interface) {
                WAYLANDIM_DEBUG() << "WAYLAND IM INTERFACE: " << interface;
                inputMethodManagerV2_ =
                    display_->getGlobal<wayland::ZwpInputMethodManagerV2>();
            }
            if (interface == wayland::ZwpVirtualKeyboardManagerV1::interface) {
                WAYLANDIM_DEBUG() << "WAYLAND VK INTERFACE: " << interface;
                virtualKeyboardManagerV1_ =
                    display_->getGlobal<wayland::ZwpVirtualKeyboardManagerV1>();
            }
            if (interface == wayland::WlSeat::interface) {
                refreshSeat();
            }
            init();
        });

    if (auto im = display_->getGlobal<wayland::ZwpInputMethodManagerV2>()) {
        inputMethodManagerV2_ = im;
    }

    if (auto vk = display_->getGlobal<wayland::ZwpVirtualKeyboardManagerV1>()) {
        virtualKeyboardManagerV1_ = vk;
    }
    init();
}

WaylandIMServerV2::~WaylandIMServerV2() {
    // Delete all input context when server goes away.
    while (!icMap_.empty()) {
        delete icMap_.begin()->second;
    }
}
InputContextManager &WaylandIMServerV2::inputContextManager() {
    return parent_->instance()->inputContextManager();
}

Instance *WaylandIMServerV2::instance() { return parent_->instance(); }

void WaylandIMServerV2::init() {
    if (init_) {
        return;
    }
    if (inputMethodManagerV2_ && virtualKeyboardManagerV1_) {
        init_ = true;
    } else {
        return;
    }
    WAYLANDIM_DEBUG() << "INIT IM V2";
    refreshSeat();

    display_->flush();
}

void WaylandIMServerV2::refreshSeat() {
    if (!init_) {
        return;
    }
    auto seats = display_->getGlobals<wayland::WlSeat>();
    for (const auto &seat : seats) {
        if (icMap_.count(seat.get())) {
            continue;
        }
        auto *ic = new WaylandIMInputContextV2(
            inputContextManager(), this, seat,
            virtualKeyboardManagerV1_->createVirtualKeyboard(seat.get()));
        ic->setFocusGroup(group_);
        ic->setCapabilityFlags(baseFlags);
    }
}

void WaylandIMServerV2::add(WaylandIMInputContextV2 *ic, wayland::WlSeat *id) {
    icMap_[id] = ic;
}

void WaylandIMServerV2::remove(wayland::WlSeat *id) {
    auto iter = icMap_.find(id);
    if (iter != icMap_.end()) {
        icMap_.erase(iter);
    }
}

WaylandIMInputContextV2::WaylandIMInputContextV2(
    InputContextManager &inputContextManager, WaylandIMServerV2 *server,
    std::shared_ptr<wayland::WlSeat> seat, wayland::ZwpVirtualKeyboardV1 *vk)
    : InputContext(inputContextManager), server_(server),
      seat_(std::move(seat)),
      ic_(server->inputMethodManagerV2()->getInputMethod(seat_.get())),
      vk_(vk) {
    server->add(this, seat_.get());
    ic_->surroundingText().connect(
        [this](const char *text, uint32_t cursor, uint32_t anchor) {
            surroundingTextCallback(text, cursor, anchor);
        });
    ic_->activate().connect([this]() {
        WAYLANDIM_DEBUG() << "ACTIVATE";
        pendingActivate_ = true;
    });
    ic_->deactivate().connect([this]() {
        WAYLANDIM_DEBUG() << "DEACTIVATE";
        pendingDeactivate_ = true;
    });
    ic_->done().connect([this]() {
        WAYLANDIM_DEBUG() << "DONE";
        ++serial_;
        if (pendingDeactivate_) {
            pendingDeactivate_ = false;
            keyboardGrab_.reset();
            timeEvent_->setEnabled(false);
            // If last key to vk is press, send a release.
            while (!pressedVKKey_.empty()) {
                auto [vkkey, vktime] = *pressedVKKey_.begin();
                sendKeyToVK(vktime, vkkey, WL_KEYBOARD_KEY_STATE_RELEASED);
            }
            vk_->modifiers(0, 0, 0, 0);
            server_->display_->sync();
            focusOut();
        }
        if (pendingActivate_) {
            pendingActivate_ = false;
            // There can be only one grab. Always release old grab first.
            // It is possible when switching between two client, there will be
            // two activate. In that case we will have already one grab. The
            // second request would fail and cause invalid object.
            keyboardGrab_.reset();
            keyboardGrab_.reset(ic_->grabKeyboard());
            if (!keyboardGrab_) {
                WAYLANDIM_DEBUG() << "Failed to grab keyboard";
            } else {
                keyboardGrab_->keymap().connect(
                    [this](uint32_t format, int32_t fd, uint32_t size) {
                        keymapCallback(format, fd, size);
                    });
                keyboardGrab_->key().connect([this](uint32_t serial,
                                                    uint32_t time, uint32_t key,
                                                    uint32_t state) {
                    keyCallback(serial, time, key, state);
                });
                keyboardGrab_->modifiers().connect(
                    [this](uint32_t serial, uint32_t mods_depressed,
                           uint32_t mods_latched, uint32_t mods_locked,
                           uint32_t group) {
                        modifiersCallback(serial, mods_depressed, mods_latched,
                                          mods_locked, group);
                    });
                keyboardGrab_->repeatInfo().connect(
                    [this](int32_t rate, int32_t delay) {
                        repeatInfoCallback(rate, delay);
                    });
                repeatInfoCallback(repeatRate_, repeatDelay_);
                focusIn();
                server_->display_->sync();
            }
        }
    });
    ic_->contentType().connect([this](uint32_t hint, uint32_t purpose) {
        WAYLANDIM_DEBUG() << "contentTypeCallback:" << hint << purpose;
        contentTypeCallback(hint, purpose);
    });
    ic_->textChangeCause().connect([this](uint32_t cause) {
        WAYLANDIM_DEBUG() << "TEXTCHANGECAUSE:" << cause << keyboardGrab_.get();
    });
    ic_->unavailable().connect([]() { WAYLANDIM_DEBUG() << "UNAVAILABLE"; });
    timeEvent_ = server_->instance()->eventLoop().addTimeEvent(
        CLOCK_MONOTONIC, now(CLOCK_MONOTONIC), 0,
        [this](EventSourceTime *, uint64_t) {
            repeat();
            return true;
        });
    timeEvent_->setAccuracy(1);
    timeEvent_->setEnabled(false);
    created();
}

WaylandIMInputContextV2::~WaylandIMInputContextV2() {
    server_->remove(seat_.get());
    destroy();
}

void WaylandIMInputContextV2::repeat() {
    if (!hasFocus()) {
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
    uint64_t interval = 1000000 / repeatRate_;
    timeEvent_->setTime(timeEvent_->time() + interval);
    timeEvent_->setOneShot();
}

void WaylandIMInputContextV2::surroundingTextCallback(const char *text,
                                                      uint32_t cursor,
                                                      uint32_t anchor) {
    std::string str(text);
    surroundingText().invalidate();
    do {
        auto length = utf8::lengthValidated(str);
        if (length != utf8::INVALID_LENGTH) {
            break;
        }
        if (cursor > str.size() || anchor > str.size()) {
            break;
        }
        size_t cursorByChar =
            utf8::lengthValidated(str.begin(), str.begin() + cursor);
        if (cursorByChar == utf8::INVALID_LENGTH) {
            break;
        }
        size_t anchorByChar =
            utf8::lengthValidated(str.begin(), str.begin() + anchor);
        if (anchorByChar == utf8::INVALID_LENGTH) {
            break;
        }
        surroundingText().setText(text, cursorByChar, anchorByChar);
    } while (0);
    updateSurroundingText();
}
void WaylandIMInputContextV2::resetCallback() { reset(); }
void WaylandIMInputContextV2::contentTypeCallback(uint32_t hint,
                                                  uint32_t purpose) {
    CapabilityFlags flags = baseFlags;

    if (hint & ZWP_TEXT_INPUT_V3_CONTENT_HINT_COMPLETION) {
        flags |= CapabilityFlag::WordCompletion;
    }
    if (hint & ZWP_TEXT_INPUT_V3_CONTENT_HINT_SPELLCHECK) {
        flags |= CapabilityFlag::SpellCheck;
    }
    if (hint & ZWP_TEXT_INPUT_V3_CONTENT_HINT_AUTO_CAPITALIZATION) {
        flags |= CapabilityFlag::UppercaseWords;
    }
    if (hint & ZWP_TEXT_INPUT_V3_CONTENT_HINT_LOWERCASE) {
        flags |= CapabilityFlag::Lowercase;
    }
    if (hint & ZWP_TEXT_INPUT_V3_CONTENT_HINT_UPPERCASE) {
        flags |= CapabilityFlag::Uppercase;
    }
    if (hint & ZWP_TEXT_INPUT_V3_CONTENT_HINT_TITLECASE) {
        // ??
    }
    if (hint & ZWP_TEXT_INPUT_V3_CONTENT_HINT_HIDDEN_TEXT) {
        flags |= CapabilityFlag::Password;
    }
    if (hint & ZWP_TEXT_INPUT_V3_CONTENT_HINT_SENSITIVE_DATA) {
        flags |= CapabilityFlag::Sensitive;
    }
    if (hint & ZWP_TEXT_INPUT_V3_CONTENT_HINT_LATIN) {
        flags |= CapabilityFlag::Alpha;
    }
    if (hint & ZWP_TEXT_INPUT_V3_CONTENT_HINT_MULTILINE) {
        flags |= CapabilityFlag::Multiline;
    }
    if (purpose == ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_ALPHA) {
        flags |= CapabilityFlag::Alpha;
    }
    if (purpose == ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_DIGITS) {
        flags |= CapabilityFlag::Digit;
    }
    if (purpose == ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_NUMBER) {
        flags |= CapabilityFlag::Number;
    }
    if (purpose == ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_PHONE) {
        flags |= CapabilityFlag::Dialable;
    }
    if (purpose == ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_URL) {
        flags |= CapabilityFlag::Url;
    }
    if (purpose == ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_EMAIL) {
        flags |= CapabilityFlag::Email;
    }
    if (purpose == ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_NAME) {
        flags |= CapabilityFlag::Name;
    }
    if (purpose == ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_PASSWORD) {
        flags |= CapabilityFlag::Password;
    }
    if (purpose == ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_PIN) {
        flags |= CapabilityFlag::Password;
        flags |= CapabilityFlag::Digit;
    }
    if (purpose == ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_DATE) {
        flags |= CapabilityFlag::Date;
    }
    if (purpose == ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_TIME) {
        flags |= CapabilityFlag::Time;
    }
    if (purpose == ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_DATETIME) {
        flags |= CapabilityFlag::Date;
        flags |= CapabilityFlag::Time;
    }
    if (purpose == ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_TERMINAL) {
        flags |= CapabilityFlag::Terminal;
    }
    setCapabilityFlags(flags);
}

void WaylandIMInputContextV2::keymapCallback(uint32_t format, int32_t fd,
                                             uint32_t size) {
    WAYLANDIM_DEBUG() << "keymapCallback";
    if (!server_->context_) {
        server_->context_.reset(xkb_context_new(XKB_CONTEXT_NO_FLAGS));
        xkb_context_set_log_level(server_->context_.get(),
                                  XKB_LOG_LEVEL_CRITICAL);
    }

    UnixFD scopeFD = UnixFD::own(fd);

    if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
        return;
    }

    auto *mapStr = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapStr == MAP_FAILED) {
        return;
    }

    const bool keymapChanged =
        (size != server_->keymapData_.size() ||
         memcmp(mapStr, server_->keymapData_.data(), size) != 0);
    if (keymapChanged) {
        server_->keymapData_.resize(size);
        server_->keymapData_.assign(static_cast<const char *>(mapStr),
                                    static_cast<const char *>(mapStr) + size);
        server_->keymap_.reset(xkb_keymap_new_from_string(
            server_->context_.get(), static_cast<const char *>(mapStr),
            XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS));
    }

    munmap(mapStr, size);

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

    if (keymapChanged) {
        vk_->keymap(format, scopeFD.fd(), size);
    }

    server_->parent_->wayland()->call<IWaylandModule::reloadXkbOption>();
}

void WaylandIMInputContextV2::keyCallback(uint32_t, uint32_t time, uint32_t key,
                                          uint32_t state) {
    time_ = time;
    if (!server_->state_) {
        return;
    }

    if (!hasFocus()) {
        focusIn();
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
            timeEvent_->setNextInterval(repeatDelay_ * 1000 - repeatHackDelay);
            timeEvent_->setOneShot();
        }
    }

    WAYLANDIM_DEBUG() << event.key().toString()
                      << " IsRelease=" << event.isRelease();
    if (!keyEvent(event)) {
        sendKeyToVK(time, event.rawKey().code() - 8,
                    event.isRelease() ? WL_KEYBOARD_KEY_STATE_RELEASED
                                      : WL_KEYBOARD_KEY_STATE_PRESSED);
    }
    server_->display_->flush();
}
void WaylandIMInputContextV2::modifiersCallback(uint32_t,
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

    vk_->modifiers(mods_depressed, mods_latched, mods_locked, group);
}

void WaylandIMInputContextV2::repeatInfoCallback(int32_t rate, int32_t delay) {
    repeatRate_ = rate;
    repeatDelay_ = delay;
}

void WaylandIMInputContextV2::sendKeyToVK(uint32_t time, uint32_t key,
                                          uint32_t state) {
    // Erase old to ensure order, and released ones can the be removed.
    pressedVKKey_.erase(key);
    if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        pressedVKKey_[key] = time;
    }
    vk_->key(time, key, state);
    server_->display_->flush();
}

void WaylandIMInputContextV2::forwardKeyImpl(const ForwardKeyEvent &key) {
    uint32_t code = 0;
    if (key.rawKey().code()) {
        code = key.rawKey().code();
    } else if (auto xkbState = server_->xkbState()) {
        auto *map = xkb_state_get_keymap(xkbState);
        auto min = xkb_keymap_min_keycode(map),
             max = xkb_keymap_max_keycode(map);
        for (auto keyCode = min; keyCode < max; keyCode++) {
            if (xkb_state_key_get_one_sym(xkbState, keyCode) ==
                static_cast<uint32_t>(key.rawKey().sym())) {
                code = keyCode;
                break;
            }
        }
    }
    sendKeyToVK(time_, code - 8,
                key.isRelease() ? WL_KEYBOARD_KEY_STATE_RELEASED
                                : WL_KEYBOARD_KEY_STATE_PRESSED);
    if (!key.isRelease()) {
        sendKeyToVK(time_, code - 8, WL_KEYBOARD_KEY_STATE_RELEASED);
    }
}

void WaylandIMInputContextV2::updatePreeditImpl() {
    if (!hasFocus()) {
        return;
    }
    auto preedit =
        server_->instance()->outputFilter(this, inputPanel().clientPreedit());

    int highlightStart = -1, highlightEnd = -1;
    int start = 0, end = 0;
    bool multipleHighlight = false;
    for (int i = 0, e = preedit.size(); i < e; i++) {
        if (!utf8::validate(preedit.stringAt(i))) {
            return;
        }
        end = start + preedit.stringAt(i).size();
        if (preedit.formatAt(i).test(TextFormatFlag::HighLight)) {
            if (highlightStart == -1) {
                highlightStart = start;
                highlightEnd = end;
            } else if (highlightEnd == start) {
                highlightEnd = end;
            } else {
                multipleHighlight = true;
            }
        }
        start = end;
    }

    int cursorStart = preedit.cursor();
    int cursorEnd = preedit.cursor();
    if (!multipleHighlight && highlightStart >= 0 && highlightEnd >= 0) {
        if (cursorStart == highlightStart) {
            cursorEnd = highlightEnd;
        }
    }

    ic_->setPreeditString(preedit.toString().data(), cursorStart, cursorEnd);
    ic_->commit(serial_);
}

void WaylandIMInputContextV2::deleteSurroundingTextImpl(int offset,
                                                        unsigned int size) {
    if (!hasFocus()) {
        return;
    }

    // Cant convert to before/after.
    if (offset > 0 || offset + static_cast<ssize_t>(size) < 0) {
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
    // validate length.
    if (cursor > len || start > len || end > len) {
        return;
    }

    auto startBytes = utf8::ncharByteLength(text.begin(), start);
    auto cursorBytes = utf8::ncharByteLength(text.begin(), cursor);
    auto sizeBytes = utf8::ncharByteLength(text.begin() + startBytes, size);
    ic_->deleteSurroundingText(cursorBytes - startBytes,
                               startBytes + sizeBytes - cursorBytes);
    ic_->commit(serial_);
}

} // namespace fcitx
