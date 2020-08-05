#include "zwp_input_popup_surface_v2.h"
#include <cassert>
namespace fcitx::wayland {

const struct zwp_input_popup_surface_v2_listener
    ZwpInputPopupSurfaceV2::listener = {
        [](void *data, zwp_input_popup_surface_v2 *wldata, int32_t x, int32_t y,
           int32_t width, int32_t height) {
            auto *obj = static_cast<ZwpInputPopupSurfaceV2 *>(data);
            assert(*obj == wldata);
            { return obj->textInputRectangle()(x, y, width, height); }
        },
};
ZwpInputPopupSurfaceV2::ZwpInputPopupSurfaceV2(zwp_input_popup_surface_v2 *data)
    : version_(zwp_input_popup_surface_v2_get_version(data)), data_(data) {
    zwp_input_popup_surface_v2_set_user_data(*this, this);
    zwp_input_popup_surface_v2_add_listener(
        *this, &ZwpInputPopupSurfaceV2::listener, this);
}
void ZwpInputPopupSurfaceV2::destructor(zwp_input_popup_surface_v2 *data) {
    auto version = zwp_input_popup_surface_v2_get_version(data);
    if (version >= 1) {
        return zwp_input_popup_surface_v2_destroy(data);
    }
}
} // namespace fcitx::wayland
