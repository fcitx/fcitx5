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
#include "fcitx-utils/utf8.h"
#include "fcitx/inputcontext.h"
#include "wayland-text-input-unstable-v1-client-protocol.h"
#include <cassert>
#include <cstring>
#include <unistd.h>
#include <xkbcommon/xkbcommon.h>

namespace fcitx {

class WaylandIMServer {
    friend class WaylandIMInputContextV1;
public:
    WaylandIMServer(wl_display *display, FocusGroup *group, const std::string &name, WaylandIMModule *waylandim);

    InputContextManager &inputContextManager() { return parent_->instance()->inputContextManager(); }

    void registryHandlerGlobal(struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version);
    void registryHandlerGlobalRemove(struct wl_registry *registry, uint32_t name);

    void activate(struct zwp_input_method_v1 *inputMethod, struct zwp_input_method_context_v1 *id);

    void deactivate(struct zwp_input_method_v1 *inputMethod, struct zwp_input_method_context_v1 *id);

    ~WaylandIMServer() {}

private:
    static const struct wl_registry_listener registryListener;
    static const struct zwp_input_method_v1_listener inputMethodListener;
    FocusGroup *group_;
    std::string name_;
    WaylandIMModule *parent_;
    zwp_input_method_v1 *inputMethodV1_;

    std::unique_ptr<struct xkb_context, decltype(&xkb_context_unref)> context_;
    std::unique_ptr<struct xkb_keymap, decltype(&xkb_keymap_unref)> keymap_;
    std::unique_ptr<struct xkb_state, decltype(&xkb_state_unref)> state_;
    
    struct StateMask {
        uint32_t shift_mask;
        uint32_t lock_mask;
        uint32_t control_mask;
        uint32_t mod1_mask;
        uint32_t mod2_mask;
        uint32_t mod3_mask;
        uint32_t mod4_mask;
        uint32_t mod5_mask;
        uint32_t super_mask;
        uint32_t hyper_mask;
        uint32_t meta_mask;
    } stateMask_;
    
    KeyStates modifiers_;
};

const struct wl_registry_listener WaylandIMServer::registryListener = {
    [](void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version) {
        static_cast<WaylandIMServer *>(data)->registryHandlerGlobal(registry, name, interface, version);
    },
    [](void *data, struct wl_registry *registry, uint32_t name) {
        static_cast<WaylandIMServer *>(data)->registryHandlerGlobalRemove(registry, name);
    }};
const struct zwp_input_method_v1_listener WaylandIMServer::inputMethodListener = {
    [](void *data, struct zwp_input_method_v1 *inputMethod, struct zwp_input_method_context_v1 *id) {
        static_cast<WaylandIMServer *>(data)->activate(inputMethod, id);
    },
    [](void *data, struct zwp_input_method_v1 *inputMethod, struct zwp_input_method_context_v1 *id) {
        static_cast<WaylandIMServer *>(data)->deactivate(inputMethod, id);
    }};

class WaylandIMInputContextV1 : public InputContext {
public:
    WaylandIMInputContextV1(InputContextManager &inputContextManager, WaylandIMServer *server,
                            zwp_input_method_context_v1 *ic)
        : InputContext(inputContextManager), server_(server), ic_(ic) {
        zwp_input_method_context_v1_set_user_data(ic_, this);
        zwp_input_method_context_v1_add_listener(ic_, &inputMethodContextListener, this);

        auto keyboard = zwp_input_method_context_v1_grab_keyboard(ic_);
        wl_keyboard_add_listener(keyboard, &keyboardListener, ic);
    }
    ~WaylandIMInputContextV1() { zwp_input_method_context_v1_set_user_data(ic_, nullptr); }

protected:
    virtual void commitStringImpl(const std::string &text) override {
        zwp_input_method_context_v1_commit_string(ic_, serial_, text.c_str());
    }
    virtual void deleteSurroundingTextImpl(int offset, unsigned int size) override {
        zwp_input_method_context_v1_delete_surrounding_text(ic_, offset, size);
    }
    virtual void forwardKeyImpl(const ForwardKeyEvent &key) override {
        zwp_input_method_context_v1_keysym(
            ic_, serial_, 0, key.rawKey().sym(),
            key.isRelease() ? WL_KEYBOARD_KEY_STATE_RELEASED : WL_KEYBOARD_KEY_STATE_PRESSED, key.rawKey().states());
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
        auto &preedit = this->preedit();

        for (int i = 0, e = preedit.size(); i < e; i++) {
            if (!utf8::validate(preedit.stringAt(i))) {
                return;
            }
        }

        zwp_input_method_context_v1_preedit_cursor(ic_, preedit.cursor());
        // FIXME second string should be for commit
        zwp_input_method_context_v1_preedit_string(ic_, serial_, preedit.toString().c_str(),
                                                   preedit.toString().c_str());
        unsigned int index;
        for (int i = 0, e = preedit.size(); i < e; i++) {
            zwp_input_method_context_v1_preedit_styling(ic_, index, preedit.stringAt(i).size(),
                                                        waylandFormat(preedit.formatAt(i)));
            index += preedit.stringAt(i).size();
        }
    }

private:
    void surroundingTextCallback(struct zwp_input_method_context_v1 *inputContext, const char *text, uint32_t cursor,
                                 uint32_t anchor);
    void resetCallback(struct zwp_input_method_context_v1 *inputContext);
    void contentTypeCallback(struct zwp_input_method_context_v1 *inputContext, uint32_t hint, uint32_t purpose);
    void invokeActionCallback(struct zwp_input_method_context_v1 *inputContext, uint32_t button, uint32_t index);
    void commitStateCallback(struct zwp_input_method_context_v1 *inputContext, uint32_t serial);
    void preferredLanguageCallback(struct zwp_input_method_context_v1 *inputContext, const char *language);

    void keymapCallback(struct wl_keyboard *keyboard, uint32_t format, int32_t fd, uint32_t size);
    void keyCallback(struct wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state);
    void modifiersCallback(struct wl_keyboard *keyboard, uint32_t serial, uint32_t mods_depressed,
                           uint32_t mods_latched, uint32_t mods_locked, uint32_t group);

    static const struct zwp_input_method_context_v1_listener inputMethodContextListener;
    static const struct wl_keyboard_listener keyboardListener;
    WaylandIMServer *server_;
    zwp_input_method_context_v1 *ic_;
    uint32_t serial_ = 0;
};

const struct zwp_input_method_context_v1_listener WaylandIMInputContextV1::inputMethodContextListener = {
    [](void *data, struct zwp_input_method_context_v1 *inputContext, const char *text, uint32_t cursor,
       uint32_t anchor) {
        static_cast<WaylandIMInputContextV1 *>(data)->surroundingTextCallback(inputContext, text, cursor, anchor);
    },
    [](void *data, struct zwp_input_method_context_v1 *inputContext) {
        static_cast<WaylandIMInputContextV1 *>(data)->resetCallback(inputContext);
    },
    [](void *data, struct zwp_input_method_context_v1 *inputContext, uint32_t hint, uint32_t purpose) {
        static_cast<WaylandIMInputContextV1 *>(data)->contentTypeCallback(inputContext, hint, purpose);
    },
    [](void *data, struct zwp_input_method_context_v1 *inputContext, uint32_t button, uint32_t index) {
        static_cast<WaylandIMInputContextV1 *>(data)->invokeActionCallback(inputContext, button, index);
    },
    [](void *data, struct zwp_input_method_context_v1 *inputContext, uint32_t serial) {
        static_cast<WaylandIMInputContextV1 *>(data)->commitStateCallback(inputContext, serial);
    },
    [](void *data, struct zwp_input_method_context_v1 *inputContext, const char *language) {
        static_cast<WaylandIMInputContextV1 *>(data)->preferredLanguageCallback(inputContext, language);
    }};

const struct wl_keyboard_listener WaylandIMInputContextV1::keyboardListener = {

    [](void *data, struct wl_keyboard *keyboard, uint32_t format, int32_t fd, uint32_t size) {
        static_cast<WaylandIMInputContextV1 *>(data)->keymapCallback(keyboard, format, fd, size);
    },
    nullptr, nullptr,
    [](void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state) {
        static_cast<WaylandIMInputContextV1 *>(data)->keyCallback(keyboard, serial, time, key, state);
    },
    [](void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched,
       uint32_t mods_locked, uint32_t group) {
        static_cast<WaylandIMInputContextV1 *>(data)->modifiersCallback(keyboard, serial, mods_depressed, mods_latched,
                                                                        mods_locked, group);
    },
    nullptr,
};

WaylandIMServer::WaylandIMServer(wl_display *display, FocusGroup *group, const std::string &name,
                                 WaylandIMModule *waylandim)
    : group_(group), name_(name), parent_(waylandim), context_(nullptr, &xkb_context_unref), keymap_(nullptr, xkb_keymap_unref), state_(nullptr, xkb_state_unref) {
    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &WaylandIMServer::registryListener, waylandim);
}

void WaylandIMServer::registryHandlerGlobal(struct wl_registry *registry, uint32_t name, const char *interface,
                                            uint32_t version) {
    FCITX_UNUSED(version);
    if (0 == strcmp(interface, "zwp_input_method_v1")) {
        inputMethodV1_ =
            static_cast<zwp_input_method_v1 *>(wl_registry_bind(registry, name, &zwp_input_method_v1_interface, 1));
        zwp_input_method_v1_add_listener(inputMethodV1_, &WaylandIMServer::inputMethodListener, this);
    }
}

void WaylandIMServer::registryHandlerGlobalRemove(struct wl_registry *registry, uint32_t name) {
    // FIXME do we need anything here?
    FCITX_UNUSED(registry);
    FCITX_UNUSED(name);
}

void WaylandIMServer::activate(struct zwp_input_method_v1 *inputMethod, struct zwp_input_method_context_v1 *id) {
    assert(inputMethod == inputMethodV1_);
    auto ic = new WaylandIMInputContextV1(parent_->instance()->inputContextManager(), this, id);
    ic->setDisplayServer("wayland:" + name_);
    ic->setFocusGroup(group_);
}

void WaylandIMServer::deactivate(struct zwp_input_method_v1 *inputMethod, struct zwp_input_method_context_v1 *id) {
    assert(inputMethod == inputMethodV1_);

    auto ic = static_cast<WaylandIMInputContextV1 *>(zwp_input_method_context_v1_get_user_data(id));
    delete ic;
}

void WaylandIMInputContextV1::surroundingTextCallback(struct zwp_input_method_context_v1 *inputContext,
                                                      const char *text, uint32_t cursor, uint32_t anchor) {
    assert(ic_ == inputContext);
    surroundingText().setText(text, cursor, anchor);
    updateSurroundingText();
}
void WaylandIMInputContextV1::resetCallback(struct zwp_input_method_context_v1 *inputContext) {
    assert(ic_ == inputContext);
    reset();
}
void WaylandIMInputContextV1::contentTypeCallback(struct zwp_input_method_context_v1 *inputContext, uint32_t hint,
                                                  uint32_t purpose) {
    assert(ic_ == inputContext);
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
void WaylandIMInputContextV1::invokeActionCallback(struct zwp_input_method_context_v1 *inputContext, uint32_t button,
                                                   uint32_t index) {
    assert(ic_ == inputContext);
    FCITX_UNUSED(button);
    FCITX_UNUSED(index);
}
void WaylandIMInputContextV1::commitStateCallback(struct zwp_input_method_context_v1 *inputContext, uint32_t serial) {
    assert(ic_ == inputContext);
    serial_ = serial;
}
void WaylandIMInputContextV1::preferredLanguageCallback(struct zwp_input_method_context_v1 *inputContext,
                                                        const char *language) {
    assert(ic_ == inputContext);
    FCITX_UNUSED(language);
}

void WaylandIMInputContextV1::keymapCallback(struct wl_keyboard *keyboard, uint32_t format, int32_t fd, uint32_t size) {
    if (!server_->context_) {
        server_->context_.reset(xkb_context_new(XKB_CONTEXT_NO_FLAGS));
        xkb_context_set_log_level(server_->context_.get(), XKB_LOG_LEVEL_CRITICAL);
    }
    
    if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
        close(fd);
        return;
    }

    if (server_->keymap_) {
        server_->keymap_.reset();
    }

    std::vector<char> buffer;
    buffer.resize(1024);
    int bufferPtr = 0;
    int readSize;
    while ((readSize = read(fd, &buffer.data()[bufferPtr], buffer.size() - bufferPtr)) > 0) {
        bufferPtr += readSize;
        if (bufferPtr == buffer.size()) {
            buffer.resize(buffer.size() * 2);
        }
    }
    buffer[bufferPtr] = 0;

    server_->keymap_.reset(xkb_keymap_new_from_string (server_->context_.get(),
                                 buffer.data(),
                                 XKB_KEYMAP_FORMAT_TEXT_V1,
                                 XKB_KEYMAP_COMPILE_NO_FLAGS));

    close(fd);

    if (!server_->keymap_) {
        return;
    }

    server_->state_.reset(xkb_state_new (server_->keymap_.get()));
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

void WaylandIMInputContextV1::keyCallback(struct wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state) {
    if (!server_->state_) {
        return;
    }

    // EVDEV OFFSET
    uint32_t code = key + 8;
    const xkb_keysym_t *syms;
    KeyEvent event(
        this,
        Key(static_cast<KeySym>(xkb_state_key_get_one_sym(server_->state_.get(), code)), server_->modifiers_),
        state == WL_KEYBOARD_KEY_STATE_RELEASED, code, time);

    if (!keyEvent(event)) {
        zwp_input_method_context_v1_key(ic_,
                            serial,
                            time,
                            key,
                            state);
    }
}
void WaylandIMInputContextV1::modifiersCallback(struct wl_keyboard *keyboard, uint32_t serial, uint32_t mods_depressed,
                        uint32_t mods_latched, uint32_t mods_locked, uint32_t group) {
    
    xkb_mod_mask_t mask;

    xkb_state_update_mask (server_->state_.get(), mods_depressed,
                           mods_latched, mods_locked, 0, 0, group);
    mask = xkb_state_serialize_mods (server_->state_.get(),
                                     static_cast<xkb_state_component>(XKB_STATE_DEPRESSED |
                                     XKB_STATE_LATCHED));

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

    zwp_input_method_context_v1_modifiers(ic_, serial,
                                       mods_depressed, mods_depressed,
                                       mods_latched, group);
}

WaylandIMModule::WaylandIMModule(Instance *instance)
    : instance_(instance), createdCallback_(wayland()->call<IWaylandModule::addConnectionCreatedCallback>(
                               [this](const std::string &name, wl_display *display, FocusGroup *group) {
                                   WaylandIMServer *server = new WaylandIMServer(display, group, name, this);
                                   servers_[name].reset(server);
                               })),
      closedCallback_(wayland()->call<IWaylandModule::addConnectionClosedCallback>(
          [this](const std::string &name, wl_display *) { servers_.erase(name); })) {}

AddonInstance *WaylandIMModule::wayland() {
    auto &addonManager = instance_->addonManager();
    return addonManager.addon("wayland");
}

WaylandIMModule::~WaylandIMModule() {}
}
