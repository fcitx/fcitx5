#include "zwp_input_panel_surface_v1.h"
#include "wl_output.h"
namespace fcitx::wayland {
ZwpInputPanelSurfaceV1::ZwpInputPanelSurfaceV1(zwp_input_panel_surface_v1 *data)
    : version_(zwp_input_panel_surface_v1_get_version(data)), data_(data) {
    zwp_input_panel_surface_v1_set_user_data(*this, this);
}
void ZwpInputPanelSurfaceV1::destructor(zwp_input_panel_surface_v1 *data) {
    { return zwp_input_panel_surface_v1_destroy(data); }
}
void ZwpInputPanelSurfaceV1::setToplevel(WlOutput *output, uint32_t position) {
    return zwp_input_panel_surface_v1_set_toplevel(*this, rawPointer(output),
                                                   position);
}
void ZwpInputPanelSurfaceV1::setOverlayPanel() {
    return zwp_input_panel_surface_v1_set_overlay_panel(*this);
}
} // namespace fcitx::wayland
