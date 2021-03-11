#include "zwp_input_method_keyboard_grab_v3.h"
#include <cassert>
namespace fcitx::wayland {
const struct zwp_input_method_keyboard_grab_v3_listener
    ZwpInputMethodKeyboardGrabV3::listener = {
        [](void *data, zwp_input_method_keyboard_grab_v3 *wldata,
           uint32_t format, int32_t fd, uint32_t size) {
            auto *obj = static_cast<ZwpInputMethodKeyboardGrabV3 *>(data);
            assert(*obj == wldata);
            { return obj->keymap()(format, fd, size); }
        },
        [](void *data, zwp_input_method_keyboard_grab_v3 *wldata,
           uint32_t serial, uint32_t time, uint32_t key, uint32_t state) {
            auto *obj = static_cast<ZwpInputMethodKeyboardGrabV3 *>(data);
            assert(*obj == wldata);
            { return obj->key()(serial, time, key, state); }
        },
        [](void *data, zwp_input_method_keyboard_grab_v3 *wldata,
           uint32_t serial, uint32_t modsDepressed, uint32_t modsLatched,
           uint32_t modsLocked, uint32_t group) {
            auto *obj = static_cast<ZwpInputMethodKeyboardGrabV3 *>(data);
            assert(*obj == wldata);
            {
                return obj->modifiers()(serial, modsDepressed, modsLatched,
                                        modsLocked, group);
            }
        },
        [](void *data, zwp_input_method_keyboard_grab_v3 *wldata, int32_t rate,
           int32_t delay) {
            auto *obj = static_cast<ZwpInputMethodKeyboardGrabV3 *>(data);
            assert(*obj == wldata);
            { return obj->repeatInfo()(rate, delay); }
        },
};
ZwpInputMethodKeyboardGrabV3::ZwpInputMethodKeyboardGrabV3(
    zwp_input_method_keyboard_grab_v3 *data)
    : version_(zwp_input_method_keyboard_grab_v3_get_version(data)),
      data_(data) {
    zwp_input_method_keyboard_grab_v3_set_user_data(*this, this);
    zwp_input_method_keyboard_grab_v3_add_listener(
        *this, &ZwpInputMethodKeyboardGrabV3::listener, this);
}
void ZwpInputMethodKeyboardGrabV3::destructor(
    zwp_input_method_keyboard_grab_v3 *data) {
    auto version = zwp_input_method_keyboard_grab_v3_get_version(data);
    if (version >= 1) {
        return zwp_input_method_keyboard_grab_v3_release(data);
    } else {
        return zwp_input_method_keyboard_grab_v3_destroy(data);
    }
}
} // namespace fcitx::wayland
