/*
 * SPDX-FileCopyrightText: 2016-2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX5_FRONTEND_WAYLANDIM_WAYLANDIMSERVER_H_
#define _FCITX5_FRONTEND_WAYLANDIM_WAYLANDIMSERVER_H_

#include <memory>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>
#include "fcitx-utils/event.h"
#include "fcitx-utils/key.h"
#include "fcitx-utils/keysymgen.h"
#include "fcitx-utils/macros.h"
#include "fcitx/focusgroup.h"
#include "fcitx/inputcontext.h"
#include "fcitx/inputcontextmanager.h"
#include "fcitx/instance.h"
#include "virtualinputcontext.h"
#include "waylandimserverbase.h"
#include "wl_keyboard.h"
#include "zwp_input_method_context_v1.h"
#include "zwp_input_method_v1.h"

namespace fcitx {
class WaylandIMModule;
class WaylandIMInputContextV1;

class WaylandIMServer : public WaylandIMServerBase {
    friend class WaylandIMInputContextV1;

public:
    WaylandIMServer(wl_display *display, FocusGroup *group,
                    const std::string &name, WaylandIMModule *waylandim);

    ~WaylandIMServer() override;

    InputContextManager &inputContextManager();

    void init();
    void activate(wayland::ZwpInputMethodContextV1 *id);
    void deactivate(wayland::ZwpInputMethodContextV1 *id);
    Instance *instance();
    FocusGroup *group() { return group_; }
    bool hasKeyboardGrab() const;

private:
    std::shared_ptr<wayland::ZwpInputMethodV1> inputMethodV1_;

    ScopedConnection globalConn_;

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

    TrackableObjectReference<InputContext> globalIc_;
};

class WaylandIMInputContextV1 : public VirtualInputContextGlue {
public:
    WaylandIMInputContextV1(InputContextManager &inputContextManager,
                            WaylandIMServer *server);
    ~WaylandIMInputContextV1() override;

    const char *frontend() const override { return "wayland"; }

    void activate(wayland::ZwpInputMethodContextV1 *ic);
    void deactivate(wayland::ZwpInputMethodContextV1 *ic);
    bool hasKeyboardGrab() const { return keyboard_.get(); }

protected:
    void commitStringDelegate(const InputContext *ic,
                              const std::string &text) const override {
        FCITX_UNUSED(ic);
        if (!ic_) {
            return;
        }
        ic_->commitString(serial_, text.c_str());
    }
    void deleteSurroundingTextDelegate(InputContext *ic, int offset,
                                       unsigned int size) const override;
    void forwardKeyDelegate(InputContext *ic,
                            const ForwardKeyEvent &key) const override {
        FCITX_UNUSED(ic);
        if (!ic_) {
            return;
        }
        if (key.rawKey().code() && key.rawKey().states() == KeyState::NoState) {
            sendKeyToVK(time_, key.rawKey(),
                        key.isRelease() ? WL_KEYBOARD_KEY_STATE_RELEASED
                                        : WL_KEYBOARD_KEY_STATE_PRESSED);
            if (!key.isRelease()) {
                sendKeyToVK(time_, key.rawKey(),
                            WL_KEYBOARD_KEY_STATE_RELEASED);
            }
        } else {
            sendKey(time_, key.rawKey().sym(),
                    key.isRelease() ? WL_KEYBOARD_KEY_STATE_RELEASED
                                    : WL_KEYBOARD_KEY_STATE_PRESSED,
                    key.rawKey().states());
            if (!key.isRelease()) {
                sendKey(time_, key.rawKey().sym(),
                        WL_KEYBOARD_KEY_STATE_RELEASED, key.rawKey().states());
            }
        }
    }

    void updatePreeditDelegate(InputContext *ic) const override;

private:
    void repeat();
    void surroundingTextCallback(const char *text, uint32_t cursor,
                                 uint32_t anchor);
    void resetCallback();
    void contentTypeCallback(uint32_t hint, uint32_t purpose);
    void invokeActionCallback(uint32_t button, uint32_t index);
    void commitStateCallback(uint32_t serial);
    static void preferredLanguageCallback(const char *language);

    void keymapCallback(uint32_t format, int32_t fd, uint32_t size);
    void keyCallback(uint32_t serial, uint32_t time, uint32_t key,
                     uint32_t state);
    void modifiersCallback(uint32_t serial, uint32_t mods_depressed,
                           uint32_t mods_latched, uint32_t mods_locked,
                           uint32_t group);
    void repeatInfoCallback(int32_t rate, int32_t delay);

    void sendKey(uint32_t time, uint32_t sym, uint32_t state,
                 KeyStates states) const;
    void sendKeyToVK(uint32_t time, const Key &key, uint32_t state) const;

    static uint32_t toModifiers(KeyStates states) {
        uint32_t modifiers = 0;
        // We use Shift Control Mod1 Mod4
        if (states.test(KeyState::Shift)) {
            modifiers |= (1 << 0);
        }
        if (states.test(KeyState::Ctrl)) {
            modifiers |= (1 << 1);
        }
        if (states.test(KeyState::Alt)) {
            modifiers |= (1 << 2);
        }
        if (states.test(KeyState::Super)) {
            modifiers |= (1 << 3);
        }
        return modifiers;
    }

    WaylandIMServer *server_;
    std::unique_ptr<wayland::ZwpInputMethodContextV1> ic_;
    std::unique_ptr<wayland::WlKeyboard> keyboard_;
    std::unique_ptr<EventSourceTime> timeEvent_;
    std::unique_ptr<VirtualInputContextManager> virtualICManager_;
    uint32_t serial_ = 0;
    uint32_t time_ = 0;

    uint32_t repeatKey_ = 0;
    uint32_t repeatTime_ = 0;
    KeySym repeatSym_ = FcitxKey_None;

    int32_t repeatRate_ = 40, repeatDelay_ = 400;
};

} // namespace fcitx

#endif // _FCITX5_FRONTEND_WAYLANDIM_WAYLANDIMSERVER_H_
