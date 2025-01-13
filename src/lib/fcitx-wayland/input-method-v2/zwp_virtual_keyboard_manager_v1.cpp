#include "zwp_virtual_keyboard_manager_v1.h"
#include "wl_seat.h"
#include "zwp_virtual_keyboard_v1.h"
namespace fcitx::wayland {
ZwpVirtualKeyboardManagerV1::ZwpVirtualKeyboardManagerV1(
    zwp_virtual_keyboard_manager_v1 *data)
    : version_(zwp_virtual_keyboard_manager_v1_get_version(data)), data_(data) {
    zwp_virtual_keyboard_manager_v1_set_user_data(*this, this);
}
void ZwpVirtualKeyboardManagerV1::destructor(
    zwp_virtual_keyboard_manager_v1 *data) {
    {
        return zwp_virtual_keyboard_manager_v1_destroy(data);
    }
}
ZwpVirtualKeyboardV1 *
ZwpVirtualKeyboardManagerV1::createVirtualKeyboard(WlSeat *seat) {
    return new ZwpVirtualKeyboardV1(
        zwp_virtual_keyboard_manager_v1_create_virtual_keyboard(
            *this, rawPointer(seat)));
}
} // namespace fcitx::wayland
