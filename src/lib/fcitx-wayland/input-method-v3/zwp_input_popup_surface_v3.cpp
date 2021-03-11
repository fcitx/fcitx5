#include "zwp_input_popup_surface_v3.h"
#include <cassert>
namespace fcitx::wayland {
ZwpInputPopupSurfaceV3::ZwpInputPopupSurfaceV3(zwp_input_popup_surface_v3 *data)
    : version_(zwp_input_popup_surface_v3_get_version(data)), data_(data) {
    zwp_input_popup_surface_v3_set_user_data(*this, this);
}
void ZwpInputPopupSurfaceV3::destructor(zwp_input_popup_surface_v3 *data) {
    auto version = zwp_input_popup_surface_v3_get_version(data);
    if (version >= 1) {
        return zwp_input_popup_surface_v3_destroy(data);
    }
}
} // namespace fcitx::wayland
