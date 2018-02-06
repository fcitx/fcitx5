#include "wl_compositor.h"
#include "wl_region.h"
#include "wl_surface.h"
#include <cassert>
namespace fcitx {
namespace wayland {
constexpr const char *WlCompositor::interface;
constexpr const wl_interface *const WlCompositor::wlInterface;
const uint32_t WlCompositor::version;
WlCompositor::WlCompositor(wl_compositor *data)
    : version_(wl_compositor_get_version(data)),
      data_(data, &WlCompositor::destructor) {
    wl_compositor_set_user_data(*this, this);
}
void WlCompositor::destructor(wl_compositor *data) {
    { return wl_compositor_destroy(data); }
}
WlSurface *WlCompositor::createSurface() {
    return new WlSurface(wl_compositor_create_surface(*this));
}
WlRegion *WlCompositor::createRegion() {
    return new WlRegion(wl_compositor_create_region(*this));
}
} // namespace wayland
} // namespace fcitx
