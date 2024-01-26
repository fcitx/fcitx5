#include "wl_subsurface.h"
#include "wl_surface.h"
namespace fcitx::wayland {
WlSubsurface::WlSubsurface(wl_subsurface *data)
    : version_(wl_subsurface_get_version(data)), data_(data) {
    wl_subsurface_set_user_data(*this, this);
}
void WlSubsurface::destructor(wl_subsurface *data) {
    auto version = wl_subsurface_get_version(data);
    if (version >= 1) {
        return wl_subsurface_destroy(data);
    }
}
void WlSubsurface::setPosition(int32_t x, int32_t y) {
    return wl_subsurface_set_position(*this, x, y);
}
void WlSubsurface::placeAbove(WlSurface *sibling) {
    return wl_subsurface_place_above(*this, rawPointer(sibling));
}
void WlSubsurface::placeBelow(WlSurface *sibling) {
    return wl_subsurface_place_below(*this, rawPointer(sibling));
}
void WlSubsurface::setSync() { return wl_subsurface_set_sync(*this); }
void WlSubsurface::setDesync() { return wl_subsurface_set_desync(*this); }
} // namespace fcitx::wayland
