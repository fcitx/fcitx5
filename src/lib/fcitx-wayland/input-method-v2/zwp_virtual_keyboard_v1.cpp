#include "zwp_virtual_keyboard_v1.h"
namespace fcitx::wayland {
ZwpVirtualKeyboardV1::ZwpVirtualKeyboardV1(zwp_virtual_keyboard_v1 *data)
    : version_(zwp_virtual_keyboard_v1_get_version(data)), data_(data) {
    zwp_virtual_keyboard_v1_set_user_data(*this, this);
}
void ZwpVirtualKeyboardV1::destructor(zwp_virtual_keyboard_v1 *data) {
    auto version = zwp_virtual_keyboard_v1_get_version(data);
    if (version >= 1) {
        return zwp_virtual_keyboard_v1_destroy(data);
    }
}
void ZwpVirtualKeyboardV1::keymap(uint32_t format, int32_t fd, uint32_t size) {
    return zwp_virtual_keyboard_v1_keymap(*this, format, fd, size);
}
void ZwpVirtualKeyboardV1::key(uint32_t time, uint32_t key, uint32_t state) {
    return zwp_virtual_keyboard_v1_key(*this, time, key, state);
}
void ZwpVirtualKeyboardV1::modifiers(uint32_t modsDepressed,
                                     uint32_t modsLatched, uint32_t modsLocked,
                                     uint32_t group) {
    return zwp_virtual_keyboard_v1_modifiers(*this, modsDepressed, modsLatched,
                                             modsLocked, group);
}
} // namespace fcitx::wayland
