#include "ext_background_effect_surface_v1.h"
#include "wayland-ext-background-effect-v1-client-protocol.h"
#include "wl_region.h"

namespace fcitx::wayland {

ExtBackgroundEffectSurfaceV1::ExtBackgroundEffectSurfaceV1(
    ext_background_effect_surface_v1 *data)
    : version_(ext_background_effect_surface_v1_get_version(data)),
      data_(data) {
    ext_background_effect_surface_v1_set_user_data(*this, this);
}

void ExtBackgroundEffectSurfaceV1::destructor(
    ext_background_effect_surface_v1 *data) {
    ext_background_effect_surface_v1_destroy(data);
}
void ExtBackgroundEffectSurfaceV1::setBlurRegion(WlRegion *region) {
    ext_background_effect_surface_v1_set_blur_region(*this, rawPointer(region));
}

} // namespace fcitx::wayland
