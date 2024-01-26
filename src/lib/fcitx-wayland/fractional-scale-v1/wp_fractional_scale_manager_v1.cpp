#include "wp_fractional_scale_manager_v1.h"
#include "wl_surface.h"
#include "wp_fractional_scale_v1.h"
namespace fcitx::wayland {
WpFractionalScaleManagerV1::WpFractionalScaleManagerV1(
    wp_fractional_scale_manager_v1 *data)
    : version_(wp_fractional_scale_manager_v1_get_version(data)), data_(data) {
    wp_fractional_scale_manager_v1_set_user_data(*this, this);
}
void WpFractionalScaleManagerV1::destructor(
    wp_fractional_scale_manager_v1 *data) {
    auto version = wp_fractional_scale_manager_v1_get_version(data);
    if (version >= 1) {
        return wp_fractional_scale_manager_v1_destroy(data);
    }
}
WpFractionalScaleV1 *
WpFractionalScaleManagerV1::getFractionalScale(WlSurface *surface) {
    return new WpFractionalScaleV1(
        wp_fractional_scale_manager_v1_get_fractional_scale(
            *this, rawPointer(surface)));
}
} // namespace fcitx::wayland
