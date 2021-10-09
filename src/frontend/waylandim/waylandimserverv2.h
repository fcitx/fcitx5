/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX5_FRONTEND_WAYLANDIM_WAYLANDIMSERVERV2_H_
#define _FCITX5_FRONTEND_WAYLANDIM_WAYLANDIMSERVERV2_H_

#include <fcitx-utils/event.h>
#include <fcitx/focusgroup.h>
#include <fcitx/instance.h>
#include <xkbcommon/xkbcommon.h>
#include "display.h"
#include "zwp_input_method_keyboard_grab_v2.h"
#include "zwp_input_method_manager_v2.h"
#include "zwp_input_method_v2.h"
#include "zwp_virtual_keyboard_manager_v1.h"
#include "zwp_virtual_keyboard_v1.h"

namespace fcitx {
class WaylandIMModule;
class WaylandIMInputContextV2;

class WaylandIMServerV2 {
    friend class WaylandIMInputContextV2;

public:
    WaylandIMServerV2(wl_display *display, FocusGroup *group,
                      const std::string &name, WaylandIMModule *waylandim);

    ~WaylandIMServerV2();

    InputContextManager &inputContextManager();

    void init();
    void refreshSeat();
    void activate(wayland::ZwpInputMethodV2 *id);
    void deactivate(wayland::ZwpInputMethodV2 *id);
    void add(WaylandIMInputContextV2 *ic, wayland::WlSeat *seat);
    void remove(wayland::WlSeat *seat);
    Instance *instance();
    FocusGroup *group() { return group_; }
    auto *xkbState() { return state_.get(); }
    auto *inputMethodManagerV2() { return inputMethodManagerV2_.get(); }

private:
    FocusGroup *group_;
    std::string name_;
    bool init_ = false;
    WaylandIMModule *parent_;
    std::shared_ptr<wayland::ZwpInputMethodManagerV2> inputMethodManagerV2_;
    std::shared_ptr<wayland::ZwpVirtualKeyboardManagerV1>
        virtualKeyboardManagerV1_;

    UniqueCPtr<struct xkb_context, xkb_context_unref> context_;
    std::vector<char> keymapData_;
    UniqueCPtr<struct xkb_keymap, xkb_keymap_unref> keymap_;
    UniqueCPtr<struct xkb_state, xkb_state_unref> state_;

    wayland::Display *display_;
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

    KeyStates modifiers_;

    std::unordered_map<wayland::WlSeat *, WaylandIMInputContextV2 *> icMap_;
};

class WaylandIMInputContextV2 : public InputContext {
public:
    WaylandIMInputContextV2(InputContextManager &inputContextManager,
                            WaylandIMServerV2 *server,
                            std::shared_ptr<wayland::WlSeat> seat,
                            wayland::ZwpVirtualKeyboardV1 *vk);
    ~WaylandIMInputContextV2();

    const char *frontend() const override { return "wayland_v2"; }

    auto inputMethodV2() { return ic_.get(); }

protected:
    void commitStringImpl(const std::string &text) override {
        ic_->commitString(text.c_str());
        ic_->commit(serial_);
    }
    void deleteSurroundingTextImpl(int offset, unsigned int size) override {
        ic_->deleteSurroundingText(-offset, offset + size);
        ic_->commit(serial_);
    }
    void forwardKeyImpl(const ForwardKeyEvent &key) override;

    void updatePreeditImpl() override;

private:
    void repeat();
    void surroundingTextCallback(const char *text, uint32_t cursor,
                                 uint32_t anchor);
    void resetCallback();
    void contentTypeCallback(uint32_t hint, uint32_t purpose);
    void invokeActionCallback(uint32_t button, uint32_t index);
    void commitStateCallback(uint32_t serial);

    void keymapCallback(uint32_t format, int32_t fd, uint32_t size);
    void keyCallback(uint32_t serial, uint32_t time, uint32_t key,
                     uint32_t state);
    void modifiersCallback(uint32_t serial, uint32_t mods_depressed,
                           uint32_t mods_latched, uint32_t mods_locked,
                           uint32_t group);
    void repeatInfoCallback(int32_t rate, int32_t delay);

    WaylandIMServerV2 *server_;
    std::shared_ptr<wayland::WlSeat> seat_;
    std::unique_ptr<wayland::ZwpInputMethodV2> ic_;
    std::unique_ptr<wayland::ZwpInputMethodKeyboardGrabV2> keyboardGrab_;
    std::unique_ptr<wayland::ZwpVirtualKeyboardV1> vk_;
    std::unique_ptr<EventSourceTime> timeEvent_;

    bool pendingActivate_ = false;
    bool pendingDeactivate_ = false;

    uint32_t serial_ = 0;
    uint32_t time_ = 0;

    uint32_t repeatKey_ = 0;
    uint32_t repeatTime_ = 0;
    KeySym repeatSym_ = FcitxKey_None;

    int32_t repeatRate_ = 40, repeatDelay_ = 400;
};

} // namespace fcitx

#endif // _FCITX5_FRONTEND_WAYLANDIM_WAYLANDIMSERVERV2_H_
