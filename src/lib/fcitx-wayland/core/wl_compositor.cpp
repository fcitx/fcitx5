#include "wl_compositor.h"
#include "wl_region.h"
#include "wl_surface.h"

namespace fcitx::wayland {

WlCompositor::WlCompositor(wl_compositor *data)
    : version_(wl_compositor_get_version(data)), data_(data) {
    wl_compositor_set_user_data(*this, this);
}

void WlCompositor::destructor(wl_compositor *data) {
    const auto version = wl_compositor_get_version(data);
#if defined(WL_COMPOSITOR_RELEASE_SINCE_VERSION)
    if (version >= 7) {
        wl_compositor_release(data);
        return;
    }
#endif
    wl_compositor_destroy(data);
}
WlSurface *WlCompositor::createSurface() {
    return new WlSurface(wl_compositor_create_surface(*this));
}
WlRegion *WlCompositor::createRegion() {
    return new WlRegion(wl_compositor_create_region(*this));
}

} // namespace fcitx::wayland
