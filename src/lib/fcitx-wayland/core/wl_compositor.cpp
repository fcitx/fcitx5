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
#if defined(WL_COMPOSITOR_CREATE_SURFACE_SINCE_VERSION)
WlSurface *WlCompositor::createSurface() {
    return new WlSurface(wl_compositor_create_surface(*this));
}
#endif
#if defined(WL_COMPOSITOR_CREATE_REGION_SINCE_VERSION)
WlRegion *WlCompositor::createRegion() {
    return new WlRegion(wl_compositor_create_region(*this));
}
#endif

} // namespace fcitx::wayland
