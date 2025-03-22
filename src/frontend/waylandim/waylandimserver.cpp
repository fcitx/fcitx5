/*
 * SPDX-FileCopyrightText: 2016-2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "waylandimserver.h"
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>
#include <xkbcommon/xkbcommon.h>
#include "fcitx-utils/capabilityflags.h"
#include "fcitx-utils/event.h"
#include "fcitx-utils/eventloopinterface.h"
#include "fcitx-utils/key.h"
#include "fcitx-utils/keysym.h"
#include "fcitx-utils/macros.h"
#include "fcitx-utils/textformatflags.h"
#include "fcitx-utils/utf8.h"
#include "fcitx/event.h"
#include "fcitx/focusgroup.h"
#include "fcitx/inputcontext.h"
#include "fcitx/inputcontextmanager.h"
#include "fcitx/instance.h"
#include "virtualinputcontext.h"
#include "wayland-text-input-unstable-v1-client-protocol.h"
#include "wayland_public.h"
#include "waylandim.h"
#include "waylandimserverbase.h"
#include "wl_seat.h"
#include "zwp_input_method_context_v1.h"
#include "zwp_input_method_v1.h"

#ifdef __linux__
#include <linux/input-event-codes.h>
#elif __FreeBSD__
#include <dev/evdev/input-event-codes.h>
#else
#define BTN_LEFT 0x110
#define BTN_RIGHT 0x111
#endif

namespace fcitx {

namespace {

constexpr CapabilityFlags baseFlags{
    CapabilityFlag::Preedit, CapabilityFlag::FormattedPreedit,
    CapabilityFlag::SurroundingText, CapabilityFlag::ClientUnfocusCommit};

inline unsigned int waylandFormat(TextFormatFlags flags) {
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

} // namespace

WaylandIMServer::WaylandIMServer(wl_display *display, FocusGroup *group,
                                 const std::string &name,
                                 WaylandIMModule *waylandim)
    : WaylandIMServerBase(display, group, name, waylandim),
      inputMethodV1_(nullptr) {
    display_->requestGlobals<wayland::ZwpInputMethodV1>();
    globalConn_ = display_->globalCreated().connect(
        [this](const std::string &interface, const std::shared_ptr<void> &) {
            if (interface == wayland::ZwpInputMethodV1::interface) {
                init();
            }
        });

    init();
}

WaylandIMServer::~WaylandIMServer() { delete globalIc_.get(); }

InputContextManager &WaylandIMServer::inputContextManager() {
    return parent_->instance()->inputContextManager();
}

Instance *WaylandIMServer::instance() { return parent_->instance(); }

void WaylandIMServer::init() {
    auto im = display_->getGlobal<wayland::ZwpInputMethodV1>();
    if (im && !inputMethodV1_) {
        WAYLANDIM_DEBUG() << "WAYLANDIM V1";
        inputMethodV1_ = std::move(im);
        auto *globalIc = new WaylandIMInputContextV1(
            parent_->instance()->inputContextManager(), this);
        globalIc->setFocusGroup(group_);
        globalIc->setCapabilityFlags(baseFlags);
        globalIc_ = globalIc->watch();
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
    }
}

void WaylandIMServer::activate(wayland::ZwpInputMethodContextV1 *id) {
    if (auto *globalIc =
            static_cast<WaylandIMInputContextV1 *>(globalIc_.get())) {
        globalIc->activate(id);
    }
}

void WaylandIMServer::deactivate(wayland::ZwpInputMethodContextV1 *id) {
    if (auto *globalIc =
            static_cast<WaylandIMInputContextV1 *>(globalIc_.get())) {
        globalIc->deactivate(id);
    }
}

bool WaylandIMServer::hasKeyboardGrab() const {
    if (auto *globalIc =
            static_cast<WaylandIMInputContextV1 *>(globalIc_.get())) {
        return globalIc->hasKeyboardGrab();
    }
    return false;
}

WaylandIMInputContextV1::WaylandIMInputContextV1(
    InputContextManager &inputContextManager, WaylandIMServer *server)
    : VirtualInputContextGlue(inputContextManager), server_(server) {
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
    keyboard_.reset();
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
    wl_array array;
    wl_array_init(&array);
    constexpr char data[] = "Shift\0Control\0Mod1\0Mod4";
    wl_array_add(&array, sizeof(data));
    memcpy(array.data, data, sizeof(data));
    ic_->modifiersMap(&array);
    wl_array_release(&array);
    if (virtualICManager_) {
        virtualICManager_->setRealFocus(true);
    } else {
        focusIn();
    }
}

void WaylandIMInputContextV1::deactivate(wayland::ZwpInputMethodContextV1 *ic) {
    if (ic_.get() == ic) {
        ic_.reset();
        keyboard_.reset();
        repeatInfo_.reset();
        // This is the only place we update wayland display mask, so it is ok to
        // reset it to 0. This breaks the caps lock or num lock. But we have no
        // other option until we can listen to the mod change globally.
        server_->instance()->clearXkbStateMask(server_->group()->display());

        timeEvent_->setEnabled(false);
        focusOutWrapper();
    } else {
        // This should not happen, but just in case.
        delete ic;
    }
}

void WaylandIMInputContextV1::repeat() {
    if (!ic_ || !realFocus()) {
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

void WaylandIMInputContextV1::surroundingTextCallback(const char *text,
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
void WaylandIMInputContextV1::resetCallback() {
    delegatedInputContext()->reset();
}
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
    setCapabilityFlagsWrapper(flags);
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
    auto *ic = delegatedInputContext();
    auto preedit = ic->inputPanel().clientPreedit().toString();
    std::string::size_type offset = index;
    offset = std::min(offset, preedit.length());
    auto length =
        utf8::lengthValidated(std::string_view(preedit).substr(0, offset));
    if (length == utf8::INVALID_LENGTH) {
        return;
    }
    InvokeActionEvent event(action, length, ic);

    if (!realFocus()) {
        focusInWrapper();
    }
    ic->invokeAction(event);
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

    auto *mapStr = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
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

    server_->parent_->wayland()->call<IWaylandModule::reloadXkbOption>();
}

void WaylandIMInputContextV1::keyCallback(uint32_t serial, uint32_t time,
                                          uint32_t key, uint32_t state) {
    FCITX_UNUSED(serial);
    time_ = time;
    if (!server_->state_) {
        return;
    }
    if (!ic_) {
        return;
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
            timeEvent_->setNextInterval(std::max(
                0, std::max(0, (repeatDelay() * 1000) - repeatHackDelay)));
            timeEvent_->setOneShot();
        }
    }

    WAYLANDIM_DEBUG() << event.key().toString()
                      << " IsRelease=" << event.isRelease();
    if (!ic->keyEvent(event)) {
        sendKeyToVK(time, event.rawKey(), state);
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
            std::clamp((repeatDelay() * 1000) - repeatHackDelay, 0, 1000));
    }
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

    ic_->modifiers(serial_, mods_depressed, mods_latched, mods_locked, group);
}

// This is not sent by either kwin/weston, but since it's unclear whether any
// one would send it, so keep it as is.
void WaylandIMInputContextV1::repeatInfoCallback(int32_t rate, int32_t delay) {
    repeatInfo_ = std::make_tuple(rate, delay);
}

void WaylandIMInputContextV1::sendKey(uint32_t time, uint32_t sym,
                                      uint32_t state, KeyStates states) const {
    if (!ic_) {
        return;
    }
    auto modifiers = toModifiers(states);
    ic_->keysym(serial_, time, sym, state, modifiers);
}

void WaylandIMInputContextV1::sendKeyToVK(uint32_t time, const Key &key,
                                          uint32_t state) const {
    if (!ic_) {
        return;
    }

    if (auto text = server_->mayCommitAsText(key, state)) {
        ic_->commitString(serial_, text->data());
    } else {
        ic_->key(serial_, time, key.code() - 8, state);
    }
}

void WaylandIMInputContextV1::updatePreeditDelegate(InputContext *ic) const {
    if (!ic_) {
        return;
    }

    auto preedit =
        server_->instance()->outputFilter(ic, ic->inputPanel().clientPreedit());

    for (int i = 0, e = preedit.size(); i < e; i++) {
        if (!utf8::validate(preedit.stringAt(i))) {
            return;
        }
    }

    // Though negative cursor is allowed by protocol, we just don't use it.
    ic_->preeditCursor(preedit.cursor() >= 0 ? preedit.cursor()
                                             : preedit.textLength());
    unsigned int index = 0;
    for (int i = 0, e = preedit.size(); i < e; i++) {
        if (!preedit.stringAt(i).empty()) {
            ic_->preeditStyling(index, preedit.stringAt(i).size(),
                                waylandFormat(preedit.formatAt(i)));
            index += preedit.stringAt(i).size();
        }
    }
    ic_->preeditString(serial_, preedit.toString().c_str(),
                       preedit.toStringForCommit().c_str());
}

void WaylandIMInputContextV1::deleteSurroundingTextDelegate(
    InputContext *ic, int offset, unsigned int size) const {
    if (!ic_) {
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
    if (cursor > len || start > len || end > len) {
        return;
    }

    auto startBytes = utf8::ncharByteLength(text.begin(), start);
    auto cursorBytes = utf8::ncharByteLength(text.begin(), cursor);
    auto sizeBytes = utf8::ncharByteLength(text.begin() + startBytes, size);
    ic_->deleteSurroundingText(startBytes - cursorBytes, sizeBytes);
    ic_->commitString(serial_, "");
}

int32_t WaylandIMInputContextV1::repeatRate() const {
    return server_->repeatRate(nullptr, repeatInfo_);
}

int32_t WaylandIMInputContextV1::repeatDelay() const {
    return server_->repeatDelay(nullptr, repeatInfo_);
}

} // namespace fcitx
