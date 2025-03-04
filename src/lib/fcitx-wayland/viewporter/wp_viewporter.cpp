#include "wp_viewporter.h"
#include "wayland-viewporter-client-protocol.h"
#include "wl_surface.h"
#include "wp_viewport.h"

namespace fcitx::wayland {

WpViewporter::WpViewporter(wp_viewporter *data)
    : version_(wp_viewporter_get_version(data)), data_(data) {
    wp_viewporter_set_user_data(*this, this);
}

void WpViewporter::destructor(wp_viewporter *data) {
    const auto version = wp_viewporter_get_version(data);
    if (version >= 1) {
        wp_viewporter_destroy(data);
        return;
    }
}
WpViewport *WpViewporter::getViewport(WlSurface *surface) {
    return new WpViewport(
        wp_viewporter_get_viewport(*this, rawPointer(surface)));
}

} // namespace fcitx::wayland
