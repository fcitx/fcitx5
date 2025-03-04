#include "zwp_input_method_keyboard_grab_v2.h"
#include <cassert>
#include "wayland-input-method-unstable-v2-client-protocol.h"

namespace fcitx::wayland {
const struct zwp_input_method_keyboard_grab_v2_listener
    ZwpInputMethodKeyboardGrabV2::listener = {
        .keymap =
            [](void *data, zwp_input_method_keyboard_grab_v2 *wldata,
               uint32_t format, int32_t fd, uint32_t size) {
                auto *obj = static_cast<ZwpInputMethodKeyboardGrabV2 *>(data);
                assert(*obj == wldata);
                {
                    obj->keymap()(format, fd, size);
                }
            },
        .key =
            [](void *data, zwp_input_method_keyboard_grab_v2 *wldata,
               uint32_t serial, uint32_t time, uint32_t key, uint32_t state) {
                auto *obj = static_cast<ZwpInputMethodKeyboardGrabV2 *>(data);
                assert(*obj == wldata);
                {
                    obj->key()(serial, time, key, state);
                }
            },
        .modifiers =
            [](void *data, zwp_input_method_keyboard_grab_v2 *wldata,
               uint32_t serial, uint32_t modsDepressed, uint32_t modsLatched,
               uint32_t modsLocked, uint32_t group) {
                auto *obj = static_cast<ZwpInputMethodKeyboardGrabV2 *>(data);
                assert(*obj == wldata);
                {
                    obj->modifiers()(serial, modsDepressed, modsLatched,
                                     modsLocked, group);
                }
            },
        .repeat_info =
            [](void *data, zwp_input_method_keyboard_grab_v2 *wldata,
               int32_t rate, int32_t delay) {
                auto *obj = static_cast<ZwpInputMethodKeyboardGrabV2 *>(data);
                assert(*obj == wldata);
                {
                    obj->repeatInfo()(rate, delay);
                }
            },
};

ZwpInputMethodKeyboardGrabV2::ZwpInputMethodKeyboardGrabV2(
    zwp_input_method_keyboard_grab_v2 *data)
    : version_(zwp_input_method_keyboard_grab_v2_get_version(data)),
      data_(data) {
    zwp_input_method_keyboard_grab_v2_set_user_data(*this, this);
    zwp_input_method_keyboard_grab_v2_add_listener(
        *this, &ZwpInputMethodKeyboardGrabV2::listener, this);
}

void ZwpInputMethodKeyboardGrabV2::destructor(
    zwp_input_method_keyboard_grab_v2 *data) {
    const auto version = zwp_input_method_keyboard_grab_v2_get_version(data);
    if (version >= 1) {
        zwp_input_method_keyboard_grab_v2_release(data);
        return;
    }
    zwp_input_method_keyboard_grab_v2_destroy(data);
}

} // namespace fcitx::wayland
