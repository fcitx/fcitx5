/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "waylandimserverv2.h"
#include <sys/mman.h>
#include <ctime>
#include "fcitx-utils/keysymgen.h"
#include "fcitx-utils/unixfd.h"
#include "fcitx-utils/utf8.h"
#include "fcitx/inputcontext.h"
#include "virtualinputcontext.h"
#include "wayland-text-input-unstable-v3-client-protocol.h"
#include "waylandim.h"
#include "wl_seat.h"

namespace fcitx {

constexpr CapabilityFlags baseFlags{
    CapabilityFlag::Preedit, CapabilityFlag::FormattedPreedit,
    CapabilityFlag::SurroundingText, CapabilityFlag::ClientUnfocusCommit};

WaylandIMServerV2::WaylandIMServerV2(wl_display *display, FocusGroup *group,
                                     const std::string &name,
                                     WaylandIMModule *waylandim)
    : WaylandIMServerBase(display, group, name, waylandim),
      inputMethodManagerV2_(nullptr) {
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
        inputMethodManagerV2_ = std::move(im);
    }

    if (auto vk = display_->getGlobal<wayland::ZwpVirtualKeyboardManagerV1>()) {
        virtualKeyboardManagerV1_ = std::move(vk);
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

bool WaylandIMServerV2::hasKeyboardGrab() const {
    return std::any_of(icMap_.begin(), icMap_.end(),
                       [](const decltype(icMap_)::value_type &ic) {
                           return ic.second && ic.second->hasKeyboardGrab();
                       });
}

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
        auto *ic =
            new WaylandIMInputContextV2(inputContextManager(), this, seat);
        ic->setFocusGroup(group_);
        ic->setCapabilityFlags(baseFlags);
    }
}

void WaylandIMServerV2::add(WaylandIMInputContextV2 *ic,
                            wayland::WlSeat *seat) {
    icMap_[seat] = ic;
}

void WaylandIMServerV2::remove(wayland::WlSeat *seat) {
    auto iter = icMap_.find(seat);
    if (iter != icMap_.end()) {
        icMap_.erase(iter);
    }
}

WaylandIMInputContextV2::WaylandIMInputContextV2(
    InputContextManager &inputContextManager, WaylandIMServerV2 *server,
    std::shared_ptr<wayland::WlSeat> seat)
    : VirtualInputContextGlue(inputContextManager), server_(server),
      seat_(std::move(seat)),
      ic_(server->inputMethodManagerV2()->getInputMethod(seat_.get())) {
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
            repeatInfo_.reset();
            // This is the only place we update wayland xkb mask, so it is ok to
            // reset it to 0. This breaks the caps lock or num lock. But we have
            // no other option until we can listen to the mod change globally.
            server_->instance()->clearXkbStateMask(server_->group()->display());

            timeEvent_->setEnabled(false);
            if (realFocus()) {
                if (vkReady_) {
                    // If last key to vk is press, send a release.
                    while (!pressedVKKey_.empty()) {
                        auto [vkkey, vktime] = *pressedVKKey_.begin();
                        // This is key release, so we don't need real sym and
                        // states.
                        sendKeyToVK(vktime,
                                    Key(FcitxKey_None, KeyStates(), vkkey + 8),
                                    WL_KEYBOARD_KEY_STATE_RELEASED);
                    }
                }
                focusOutWrapper();
            }
            vk_.reset();
            vkReady_ = false;
        }
        if (pendingActivate_) {
            pendingActivate_ = false;
            vk_.reset();
            vkReady_ = false;
            vk_.reset(
                server_->virtualKeyboardManagerV1()->createVirtualKeyboard(
                    seat_.get()));
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
                focusInWrapper();
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
        CLOCK_MONOTONIC, now(CLOCK_MONOTONIC), 1,
        [this](EventSourceTime *, uint64_t) {
            repeat();
            return true;
        });
    timeEvent_->setAccuracy(1);
    timeEvent_->setEnabled(false);
    created();

    if (auto *appMonitor = server->parent_->appMonitor(server->name_)) {
        virtualICManager_ = std::make_unique<VirtualInputContextManager>(
            &inputContextManager, this, appMonitor);
    }
}

WaylandIMInputContextV2::~WaylandIMInputContextV2() {
    server_->remove(seat_.get());
    destroy();
}

void WaylandIMInputContextV2::repeat() {
    if (!realFocus()) {
        return;
    }
    auto *ic = delegatedInputContext();
    KeyEvent event(
        ic,
        Key(repeatSym_, server_->modifiers_ | KeyState::Repeat, repeatKey_ + 8),
        false, repeatTime_);
    sendKeyToVK(repeatTime_, event.rawKey(), WL_KEYBOARD_KEY_STATE_RELEASED);
    if (!ic->keyEvent(event)) {
        sendKeyToVK(repeatTime_, event.rawKey(), WL_KEYBOARD_KEY_STATE_PRESSED);
    }
    uint64_t interval = 1000000 / repeatRate();
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
        if (length == utf8::INVALID_LENGTH) {
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
    } while (false);
    updateSurroundingTextWrapper();
}
void WaylandIMInputContextV2::resetCallback() {
    delegatedInputContext()->reset();
}
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
    setCapabilityFlagsWrapper(flags);
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

    if (keymapChanged || !vkReady_) {
        vk_->keymap(format, scopeFD.fd(), size);
        vkReady_ = true;
    }

    server_->parent_->wayland()->call<IWaylandModule::reloadXkbOption>();
}

void WaylandIMInputContextV2::keyCallback(uint32_t serial, uint32_t time,
                                          uint32_t key, uint32_t state) {
    FCITX_UNUSED(serial);
    time_ = time;
    if (!server_->state_) {
        return;
    }

    if (!realFocus()) {
        focusInWrapper();
    }

    // EVDEV OFFSET
    uint32_t code = key + 8;

    auto *ic = delegatedInputContext();
    KeyEvent event(ic,
                   Key(static_cast<KeySym>(xkb_state_key_get_one_sym(
                           server_->state_.get(), code)),
                       server_->modifiers_, code),
                   state == WL_KEYBOARD_KEY_STATE_RELEASED, time);

    if (state == WL_KEYBOARD_KEY_STATE_RELEASED && key == repeatKey_) {
        timeEvent_->setEnabled(false);
    } else if (state == WL_KEYBOARD_KEY_STATE_PRESSED &&
               xkb_keymap_key_repeats(server_->keymap_.get(), code)) {
        if (repeatRate() > 0) {
            repeatKey_ = key;
            repeatTime_ = time;
            repeatSym_ = event.rawKey().sym();
            // Let's trick the key event system by fake our first.
            // Remove 100 from the initial interval.
            timeEvent_->setNextInterval(repeatDelay() * 1000 - repeatHackDelay);
            timeEvent_->setOneShot();
        }
    }

    WAYLANDIM_DEBUG() << event.key().toString()
                      << " IsRelease=" << event.isRelease();
    if (!ic->keyEvent(event)) {
        sendKeyToVK(time, event.rawKey(),
                    event.isRelease() ? WL_KEYBOARD_KEY_STATE_RELEASED
                                      : WL_KEYBOARD_KEY_STATE_PRESSED);
    }

    // This means our engine is being too slow, this is usually transient (e.g.
    // cold start up due to data loading, high CPU usage etc).
    // To avoid an undesired repetition, reset the delay the next interval so we
    // can handle the release first.
    if (timeEvent_->time() < now(timeEvent_->clock()) &&
        timeEvent_->isOneShot()) {
        WAYLANDIM_DEBUG() << "Engine handling speed can not keep up with key "
                             "repetition rate.";
        timeEvent_->setNextInterval(
            std::clamp(repeatDelay() * 1000 - repeatHackDelay, 0, 1000));
    }
}
void WaylandIMInputContextV2::modifiersCallback(uint32_t /*serial*/,
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
    if (mask & server_->stateMask_.mod4_mask) {
        server_->modifiers_ |= KeyState::Super;
    }
    if (mask & server_->stateMask_.mod3_mask) {
        server_->modifiers_ |= KeyState::Mod3;
    }
    if (mask & server_->stateMask_.mod5_mask) {
        server_->modifiers_ |= KeyState::Mod5;
    }

    if (vkReady_) {
        vk_->modifiers(mods_depressed, mods_latched, mods_locked, group);
    }
}

void WaylandIMInputContextV2::repeatInfoCallback(int32_t rate, int32_t delay) {
    repeatInfo_ = std::make_tuple(rate, delay);
}

void WaylandIMInputContextV2::sendKeyToVK(uint32_t time, const Key &key,
                                          uint32_t state) const {
    if (!vkReady_) {
        return;
    }

    uint32_t code = key.code() - 8;
    if (auto text = server_->mayCommitAsText(key, state)) {
        commitStringDelegate(this, *text);
        return;
    }
    // Erase old to ensure order, and released ones can the be removed.
    pressedVKKey_.erase(code);
    if (state == WL_KEYBOARD_KEY_STATE_PRESSED &&
        xkb_keymap_key_repeats(server_->keymap_.get(), key.code())) {
        pressedVKKey_[code] = time;
    }
    vk_->key(time, code, state);
}

void WaylandIMInputContextV2::forwardKeyDelegate(
    InputContext * /*ic*/, const ForwardKeyEvent &key) const {
    uint32_t code = 0;
    if (key.rawKey().code()) {
        code = key.rawKey().code();
    } else if (auto *xkbState = server_->xkbState()) {
        auto *map = xkb_state_get_keymap(xkbState);
        auto min = xkb_keymap_min_keycode(map);
        auto max = xkb_keymap_max_keycode(map);
        for (auto keyCode = min; keyCode < max; keyCode++) {
            if (xkb_state_key_get_one_sym(xkbState, keyCode) ==
                static_cast<uint32_t>(key.rawKey().sym())) {
                code = keyCode;
                break;
            }
        }
    }

    Key keyWithCode(key.rawKey().sym(), key.rawKey().states(), code);

    sendKeyToVK(time_, keyWithCode,
                key.isRelease() ? WL_KEYBOARD_KEY_STATE_RELEASED
                                : WL_KEYBOARD_KEY_STATE_PRESSED);
    if (!key.isRelease()) {
        sendKeyToVK(time_, keyWithCode, WL_KEYBOARD_KEY_STATE_RELEASED);
    }
}

void WaylandIMInputContextV2::updatePreeditDelegate(InputContext *ic) const {
    if (!realFocus()) {
        return;
    }
    auto preedit =
        server_->instance()->outputFilter(ic, ic->inputPanel().clientPreedit());

    int highlightStart = -1;
    int highlightEnd = -1;
    int start = 0;
    int end = 0;
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

    if (preedit.textLength()) {
        if (cursorStart < 0) {
            cursorStart = cursorEnd = preedit.textLength();
        }
        ic_->setPreeditString(preedit.toString().data(), cursorStart,
                              cursorEnd);
    }
    ic_->commit(serial_);
}

void WaylandIMInputContextV2::deleteSurroundingTextDelegate(
    InputContext *ic, int offset, unsigned int size) const {
    if (!realFocus()) {
        return;
    }

    // Cant convert to before/after.
    if (offset > 0 || offset + static_cast<ssize_t>(size) < 0) {
        return;
    }

    size_t cursor = ic->surroundingText().cursor();
    if (static_cast<ssize_t>(cursor) + offset < 0) {
        return;
    }

    const auto &text = ic->surroundingText().text();
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

int32_t WaylandIMInputContextV2::repeatRate() const {
    return server_->repeatRate(seat_, repeatInfo_);
}

int32_t WaylandIMInputContextV2::repeatDelay() const {
    return server_->repeatDelay(seat_, repeatInfo_);
}

} // namespace fcitx
