#include "zwp_input_popup_surface_v2.h"
#include <cassert>
#include "wayland-input-method-unstable-v2-client-protocol.h"

namespace fcitx::wayland {
const struct zwp_input_popup_surface_v2_listener
    ZwpInputPopupSurfaceV2::listener = {
        .text_input_rectangle =
            [](void *data, zwp_input_popup_surface_v2 *wldata, int32_t x,
               int32_t y, int32_t width, int32_t height) {
                auto *obj = static_cast<ZwpInputPopupSurfaceV2 *>(data);
                assert(*obj == wldata);
                {
                    obj->textInputRectangle()(x, y, width, height);
                }
            },
};

ZwpInputPopupSurfaceV2::ZwpInputPopupSurfaceV2(zwp_input_popup_surface_v2 *data)
    : version_(zwp_input_popup_surface_v2_get_version(data)), data_(data) {
    zwp_input_popup_surface_v2_set_user_data(*this, this);
    zwp_input_popup_surface_v2_add_listener(
        *this, &ZwpInputPopupSurfaceV2::listener, this);
}

void ZwpInputPopupSurfaceV2::destructor(zwp_input_popup_surface_v2 *data) {
    const auto version = zwp_input_popup_surface_v2_get_version(data);
    if (version >= 1) {
        zwp_input_popup_surface_v2_destroy(data);
        return;
    }
}

} // namespace fcitx::wayland
