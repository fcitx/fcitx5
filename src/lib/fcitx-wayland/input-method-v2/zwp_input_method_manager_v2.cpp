#include "zwp_input_method_manager_v2.h"
#include <cassert>
#include "wl_seat.h"
#include "zwp_input_method_v2.h"
namespace fcitx::wayland {

ZwpInputMethodManagerV2::ZwpInputMethodManagerV2(
    zwp_input_method_manager_v2 *data)
    : version_(zwp_input_method_manager_v2_get_version(data)), data_(data) {
    zwp_input_method_manager_v2_set_user_data(*this, this);
}
void ZwpInputMethodManagerV2::destructor(zwp_input_method_manager_v2 *data) {
    auto version = zwp_input_method_manager_v2_get_version(data);
    if (version >= 1) {
        return zwp_input_method_manager_v2_destroy(data);
    }
}
ZwpInputMethodV2 *ZwpInputMethodManagerV2::getInputMethod(WlSeat *seat) {
    return new ZwpInputMethodV2(
        zwp_input_method_manager_v2_get_input_method(*this, rawPointer(seat)));
}
} // namespace fcitx::wayland
