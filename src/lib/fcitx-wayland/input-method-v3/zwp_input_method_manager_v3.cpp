#include "zwp_input_method_manager_v3.h"
#include <cassert>
#include "wl_seat.h"
#include "zwp_input_method_v3.h"
namespace fcitx::wayland {
ZwpInputMethodManagerV3::ZwpInputMethodManagerV3(
    zwp_input_method_manager_v3 *data)
    : version_(zwp_input_method_manager_v3_get_version(data)), data_(data) {
    zwp_input_method_manager_v3_set_user_data(*this, this);
}
void ZwpInputMethodManagerV3::destructor(zwp_input_method_manager_v3 *data) {
    auto version = zwp_input_method_manager_v3_get_version(data);
    if (version >= 1) {
        return zwp_input_method_manager_v3_destroy(data);
    }
}
ZwpInputMethodV3 *ZwpInputMethodManagerV3::getInputMethod(WlSeat *seat) {
    return new ZwpInputMethodV3(
        zwp_input_method_manager_v3_get_input_method(*this, rawPointer(seat)));
}
} // namespace fcitx::wayland
