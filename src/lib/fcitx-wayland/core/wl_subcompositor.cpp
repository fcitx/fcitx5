#include "wl_subcompositor.h"
#include "wl_subsurface.h"
#include "wl_surface.h"
namespace fcitx::wayland {
WlSubcompositor::WlSubcompositor(wl_subcompositor *data)
    : version_(wl_subcompositor_get_version(data)), data_(data) {
    wl_subcompositor_set_user_data(*this, this);
}
void WlSubcompositor::destructor(wl_subcompositor *data) {
    auto version = wl_subcompositor_get_version(data);
    if (version >= 1) {
        return wl_subcompositor_destroy(data);
    }
}
WlSubsurface *WlSubcompositor::getSubsurface(WlSurface *surface,
                                             WlSurface *parent) {
    return new WlSubsurface(wl_subcompositor_get_subsurface(
        *this, rawPointer(surface), rawPointer(parent)));
}
} // namespace fcitx::wayland
