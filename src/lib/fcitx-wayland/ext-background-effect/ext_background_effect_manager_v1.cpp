#include "ext_background_effect_manager_v1.h"
#include <cassert>
#include "ext_background_effect_surface_v1.h"
#include "wayland-ext-background-effect-v1-client-protocol.h"
#include "wl_surface.h"

namespace fcitx::wayland {
const struct ext_background_effect_manager_v1_listener
    ExtBackgroundEffectManagerV1::listener = {
        .capabilities =
            [](void *data, ext_background_effect_manager_v1 *wldata,
               uint32_t flags) {
                auto *obj = static_cast<ExtBackgroundEffectManagerV1 *>(data);
                assert(*obj == wldata);
                {
                    obj->capabilities()(flags);
                }
            },
};

ExtBackgroundEffectManagerV1::ExtBackgroundEffectManagerV1(
    ext_background_effect_manager_v1 *data)
    : version_(ext_background_effect_manager_v1_get_version(data)),
      data_(data) {
    ext_background_effect_manager_v1_set_user_data(*this, this);
    ext_background_effect_manager_v1_add_listener(
        *this, &ExtBackgroundEffectManagerV1::listener, this);
}

void ExtBackgroundEffectManagerV1::destructor(
    ext_background_effect_manager_v1 *data) {
    ext_background_effect_manager_v1_destroy(data);
}
ExtBackgroundEffectSurfaceV1 *
ExtBackgroundEffectManagerV1::getBackgroundEffect(WlSurface *surface) {
    return new ExtBackgroundEffectSurfaceV1(
        ext_background_effect_manager_v1_get_background_effect(
            *this, rawPointer(surface)));
}

} // namespace fcitx::wayland
