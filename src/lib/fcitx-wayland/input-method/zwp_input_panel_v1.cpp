#include "zwp_input_panel_v1.h"
#include <cassert>
#include "wl_surface.h"
#include "zwp_input_panel_surface_v1.h"
namespace fcitx::wayland {

ZwpInputPanelV1::ZwpInputPanelV1(zwp_input_panel_v1 *data)
    : version_(zwp_input_panel_v1_get_version(data)), data_(data) {
    zwp_input_panel_v1_set_user_data(*this, this);
}
void ZwpInputPanelV1::destructor(zwp_input_panel_v1 *data) {
    { return zwp_input_panel_v1_destroy(data); }
}
ZwpInputPanelSurfaceV1 *
ZwpInputPanelV1::getInputPanelSurface(WlSurface *surface) {
    return new ZwpInputPanelSurfaceV1(
        zwp_input_panel_v1_get_input_panel_surface(*this, rawPointer(surface)));
}
} // namespace fcitx::wayland
