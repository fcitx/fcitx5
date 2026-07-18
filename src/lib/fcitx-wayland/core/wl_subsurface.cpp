#include "wl_subsurface.h"
#include "wl_surface.h"

namespace fcitx::wayland {

WlSubsurface::WlSubsurface(wl_subsurface *data)
    : version_(wl_subsurface_get_version(data)), data_(data) {
    wl_subsurface_set_user_data(*this, this);
}

void WlSubsurface::destructor(wl_subsurface *data) {
    wl_subsurface_destroy(data);
}
#if defined(WL_SUBSURFACE_SET_POSITION_SINCE_VERSION)
void WlSubsurface::setPosition(int32_t x, int32_t y) {
    wl_subsurface_set_position(*this, x, y);
}
#endif
#if defined(WL_SUBSURFACE_PLACE_ABOVE_SINCE_VERSION)
void WlSubsurface::placeAbove(WlSurface *sibling) {
    wl_subsurface_place_above(*this, rawPointer(sibling));
}
#endif
#if defined(WL_SUBSURFACE_PLACE_BELOW_SINCE_VERSION)
void WlSubsurface::placeBelow(WlSurface *sibling) {
    wl_subsurface_place_below(*this, rawPointer(sibling));
}
#endif
#if defined(WL_SUBSURFACE_SET_SYNC_SINCE_VERSION)
void WlSubsurface::setSync() { wl_subsurface_set_sync(*this); }
#endif
#if defined(WL_SUBSURFACE_SET_DESYNC_SINCE_VERSION)
void WlSubsurface::setDesync() { wl_subsurface_set_desync(*this); }
#endif

} // namespace fcitx::wayland
