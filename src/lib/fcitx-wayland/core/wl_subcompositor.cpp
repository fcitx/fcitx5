#include "wl_subcompositor.h"
#include "wl_subsurface.h"
#include "wl_surface.h"

namespace fcitx::wayland {

WlSubcompositor::WlSubcompositor(wl_subcompositor *data)
    : version_(wl_subcompositor_get_version(data)), data_(data) {
    wl_subcompositor_set_user_data(*this, this);
}

void WlSubcompositor::destructor(wl_subcompositor *data) {
    wl_subcompositor_destroy(data);
}
#if defined(WL_SUBCOMPOSITOR_GET_SUBSURFACE_SINCE_VERSION)
WlSubsurface *WlSubcompositor::getSubsurface(WlSurface *surface,
                                             WlSurface *parent) {
    return new WlSubsurface(wl_subcompositor_get_subsurface(
        *this, rawPointer(surface), rawPointer(parent)));
}
#endif

} // namespace fcitx::wayland
