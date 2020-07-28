#include "zwp_input_panel_v1.h"
#include <cassert>
#include "wl_surface.h"
#include "zwp_input_panel_surface_v1.h"
namespace fcitx {
namespace wayland {
constexpr const char *ZwpInputPanelV1::interface;
constexpr const wl_interface *const ZwpInputPanelV1::wlInterface;
const uint32_t ZwpInputPanelV1::version;
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
} // namespace wayland
} // namespace fcitx
