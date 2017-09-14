/*
 * Copyright (C) 2016~2016 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
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
#include "waylandim.h"
#include "display.h"
#include "fcitx-utils/event.h"
#include "fcitx-utils/utf8.h"
#include "fcitx/inputcontext.h"
#include "wayland-text-input-unstable-v1-client-protocol.h"
#include "wl_keyboard.h"
#include "zwp_input_method_context_v1.h"
#include "zwp_input_method_v1.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <sys/mman.h>
#include <unistd.h>
#include <xkbcommon/xkbcommon.h>

namespace fcitx {
class WaylandIMInputContextV1;

class WaylandIMServer {
    friend class WaylandIMInputContextV1;

public:
    WaylandIMServer(wl_display *display, FocusGroup *group,
                    const std::string &name, WaylandIMModule *waylandim);

    InputContextManager &inputContextManager() {
        return parent_->instance()->inputContextManager();
    }

    void init();
    void activate(wayland::ZwpInputMethodContextV1 *id);
    void deactivate(wayland::ZwpInputMethodContextV1 *id);
    void add(WaylandIMInputContextV1 *ic, wayland::ZwpInputMethodContextV1 *id);
    void remove(wayland::ZwpInputMethodContextV1 *id);
    Instance *instance() { return parent_->instance(); }

    ~WaylandIMServer() {}

private:
    FocusGroup *group_;
    std::string name_;
    WaylandIMModule *parent_;
    std::shared_ptr<wayland::ZwpInputMethodV1> inputMethodV1_;

    std::unique_ptr<struct xkb_context, decltype(&xkb_context_unref)> context_;
    std::unique_ptr<struct xkb_keymap, decltype(&xkb_keymap_unref)> keymap_;
    std::unique_ptr<struct xkb_state, decltype(&xkb_state_unref)> state_;

    wayland::Display *display_;

    struct StateMask {
        uint32_t shift_mask = 0;
        uint32_t lock_mask = 0;
        uint32_t control_mask = 0;
        uint32_t mod1_mask = 0;
        uint32_t mod2_mask = 0;
        uint32_t mod3_mask = 0;
        uint32_t mod4_mask = 0;
        uint32_t mod5_mask = 0;
        uint32_t super_mask = 0;
        uint32_t hyper_mask = 0;
        uint32_t meta_mask = 0;
    } stateMask_;

    KeyStates modifiers_;

    std::unordered_map<wayland::ZwpInputMethodContextV1 *,
                       WaylandIMInputContextV1 *>
        icMap_;
};

class WaylandIMInputContextV1 : public InputContext {
public:
    WaylandIMInputContextV1(InputContextManager &inputContextManager,
                            WaylandIMServer *server,
                            wayland::ZwpInputMethodContextV1 *ic)
        : InputContext(inputContextManager), server_(server), ic_(ic) {
        server->add(this, ic);
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
        ic_->preferredLanguage().connect([this](const char *language) {
            preferredLanguageCallback(language);
        });
        timeEvent_.reset(server_->instance()->eventLoop().addTimeEvent(
            CLOCK_MONOTONIC, now(CLOCK_MONOTONIC), 0,
            [this](EventSourceTime *, uint64_t) {
                repeat();
                return true;
            }));
        timeEvent_->setEnabled(false);

        keyboard_.reset(ic_->grabKeyboard());
        keyboard_->keymap().connect(
            [this](uint32_t format, int32_t fd, uint32_t size) {
                keymapCallback(format, fd, size);
            });
        keyboard_->key().connect(
            [this](uint32_t serial, uint32_t time, uint32_t key,
                   uint32_t state) { keyCallback(serial, time, key, state); });
        keyboard_->modifiers().connect([this](
            uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched,
            uint32_t mods_locked, uint32_t group) {
            modifiersCallback(serial, mods_depressed, mods_latched, mods_locked,
                              group);
        });
        keyboard_->repeatInfo().connect([this](int32_t rate, int32_t delay) {
            repeatInfoCallback(rate, delay);
        });
        server_->display_->roundtrip();
        repeatInfoCallback(repeatRate_, repeatDelay_);
        created();
    }
    ~WaylandIMInputContextV1() {
        server_->remove(ic_.get());
        destroy();
    }

protected:
    virtual void commitStringImpl(const std::string &text) override {
        ic_->commitString(serial_, text.c_str());
    }
    virtual void deleteSurroundingTextImpl(int offset,
                                           unsigned int size) override {
        ic_->deleteSurroundingText(offset, size);
    }
    virtual void forwardKeyImpl(const ForwardKeyEvent &key) override {
        ic_->keysym(serial_, time_, key.rawKey().sym(),
                    key.isRelease() ? WL_KEYBOARD_KEY_STATE_RELEASED
                                    : WL_KEYBOARD_KEY_STATE_PRESSED,
                    key.rawKey().states());
    }

    static inline unsigned int waylandFormat(TextFormatFlags flags) {
        unsigned int result = 0;
        if (flags & TextFormatFlag::UnderLine) {
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

    virtual void updatePreeditImpl() override {
        auto preedit = server_->instance()->outputFilter(
            this, inputPanel().clientPreedit());

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

private:
    void repeat();
    void surroundingTextCallback(const char *text, uint32_t cursor,
                                 uint32_t anchor);
    void resetCallback();
    void contentTypeCallback(uint32_t hint, uint32_t purpose);
    void invokeActionCallback(uint32_t button, uint32_t index);
    void commitStateCallback(uint32_t serial);
    void preferredLanguageCallback(const char *language);

    void keymapCallback(uint32_t format, int32_t fd, uint32_t size);
    void keyCallback(uint32_t serial, uint32_t time, uint32_t key,
                     uint32_t state);
    void modifiersCallback(uint32_t serial, uint32_t mods_depressed,
                           uint32_t mods_latched, uint32_t mods_locked,
                           uint32_t group);
    void repeatInfoCallback(int32_t rate, int32_t delay);

    WaylandIMServer *server_;
    std::unique_ptr<wayland::ZwpInputMethodContextV1> ic_;
    std::unique_ptr<wayland::WlKeyboard> keyboard_;
    std::unique_ptr<EventSourceTime> timeEvent_;
    uint32_t serial_ = 0;
    uint32_t time_ = 0;

    uint32_t repeatKey_ = 0;
    uint32_t repeatTime_ = 0;
    KeySym repeatSym_ = FcitxKey_None;

    int32_t repeatRate_ = 40, repeatDelay_ = 400;
};

WaylandIMServer::WaylandIMServer(wl_display *display, FocusGroup *group,
                                 const std::string &name,
                                 WaylandIMModule *waylandim)
    : group_(group), name_(name), parent_(waylandim), inputMethodV1_(nullptr),
      context_(nullptr, &xkb_context_unref),
      keymap_(nullptr, &xkb_keymap_unref), state_(nullptr, &xkb_state_unref),
      display_(
          static_cast<wayland::Display *>(wl_display_get_user_data(display))) {
    display_->requestGlobals<wayland::ZwpInputMethodV1>();
    display_->registry()->global().connect(
        [this](uint32_t, const char *interface, uint32_t) {
            if (0 == strcmp(interface, wayland::ZwpInputMethodV1::interface)) {
                init();
            }
        });

    init();
}

void WaylandIMServer::init() {
    auto im = display_->getGlobal<wayland::ZwpInputMethodV1>();
    if (im && !inputMethodV1_) {
        inputMethodV1_ = im;
        inputMethodV1_->activate().connect(
            [this](wayland::ZwpInputMethodContextV1 *ic) { activate(ic); });
        inputMethodV1_->deactivate().connect(
            [this](wayland::ZwpInputMethodContextV1 *ic) { deactivate(ic); });
        display_->flush();
    }
}

void WaylandIMServer::activate(wayland::ZwpInputMethodContextV1 *id) {
    auto ic = new WaylandIMInputContextV1(
        parent_->instance()->inputContextManager(), this, id);
    ic->setFocusGroup(group_);
}

void WaylandIMServer::deactivate(wayland::ZwpInputMethodContextV1 *id) {
    auto iter = icMap_.find(id);
    delete iter->second;
}

void WaylandIMServer::add(WaylandIMInputContextV1 *ic,
                          wayland::ZwpInputMethodContextV1 *id) {
    icMap_[id] = ic;
}

void WaylandIMServer::remove(wayland::ZwpInputMethodContextV1 *id) {
    auto iter = icMap_.find(id);
    icMap_.erase(iter);
}

void WaylandIMInputContextV1::repeat() {
    KeyEvent event(this, Key(repeatSym_, server_->modifiers_, repeatKey_),
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
        flags |= CapabilityFlag::HiddenText;
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
    FCITX_UNUSED(button);
    FCITX_UNUSED(index);
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

    auto mapStr = mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0);
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

    KeyEvent event(this, Key(static_cast<KeySym>(xkb_state_key_get_one_sym(
                                 server_->state_.get(), code)),
                             server_->modifiers_),
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

    if (!keyEvent(event)) {
        ic_->keysym(serial, time, event.rawKey().sym(),
                    event.isRelease() ? WL_KEYBOARD_KEY_STATE_RELEASED
                                      : WL_KEYBOARD_KEY_STATE_PRESSED,
                    event.rawKey().states());
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
    mask = xkb_state_serialize_mods(
        server_->state_.get(), static_cast<xkb_state_component>(
                                   XKB_STATE_DEPRESSED | XKB_STATE_LATCHED));

    server_->modifiers_ = 0;
    if (mask & server_->stateMask_.shift_mask)
        server_->modifiers_ |= KeyState::Shift;
    if (mask & server_->stateMask_.lock_mask)
        server_->modifiers_ |= KeyState::CapsLock;
    if (mask & server_->stateMask_.control_mask)
        server_->modifiers_ |= KeyState::Ctrl;
    if (mask & server_->stateMask_.mod1_mask)
        server_->modifiers_ |= KeyState::Alt;
    if (mask & server_->stateMask_.super_mask)
        server_->modifiers_ |= KeyState::Super;
    if (mask & server_->stateMask_.hyper_mask)
        server_->modifiers_ |= KeyState::Hyper;
    if (mask & server_->stateMask_.meta_mask)
        server_->modifiers_ |= KeyState::Meta;

    ic_->modifiers(serial, mods_depressed, mods_depressed, mods_latched, group);
}

void WaylandIMInputContextV1::repeatInfoCallback(int32_t rate, int32_t delay) {
    repeatRate_ = rate;
    repeatDelay_ = delay;
    timeEvent_->setAccuracy(std::min(delay * 1000, 1000000 / rate));
}

WaylandIMModule::WaylandIMModule(Instance *instance) : instance_(instance) {
    createdCallback_.reset(
        wayland()->call<IWaylandModule::addConnectionCreatedCallback>([this](
            const std::string &name, wl_display *display, FocusGroup *group) {
            WaylandIMServer *server =
                new WaylandIMServer(display, group, name, this);
            servers_[name].reset(server);
        }));
    closedCallback_.reset(
        wayland()->call<IWaylandModule::addConnectionClosedCallback>([this](
            const std::string &name, wl_display *) { servers_.erase(name); }));
}

AddonInstance *WaylandIMModule::wayland() {
    auto &addonManager = instance_->addonManager();
    return addonManager.addon("wayland");
}

WaylandIMModule::~WaylandIMModule() {}

class WaylandIMModuleFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override {
        return new WaylandIMModule(manager->instance());
    }
};
}

FCITX_ADDON_FACTORY(fcitx::WaylandIMModuleFactory);
