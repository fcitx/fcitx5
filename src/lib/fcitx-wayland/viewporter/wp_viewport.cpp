#include "wp_viewport.h"
#include "wayland-viewporter-client-protocol.h"

namespace fcitx::wayland {

WpViewport::WpViewport(wp_viewport *data)
    : version_(wp_viewport_get_version(data)), data_(data) {
    wp_viewport_set_user_data(*this, this);
}

void WpViewport::destructor(wp_viewport *data) {
    const auto version = wp_viewport_get_version(data);
    if (version >= 1) {
        wp_viewport_destroy(data);
        return;
    }
}
void WpViewport::setSource(wl_fixed_t x, wl_fixed_t y, wl_fixed_t width,
                           wl_fixed_t height) {
    wp_viewport_set_source(*this, x, y, width, height);
}
void WpViewport::setDestination(int32_t width, int32_t height) {
    wp_viewport_set_destination(*this, width, height);
}

} // namespace fcitx::wayland
