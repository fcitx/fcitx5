#ifndef ZWP_INPUT_PANEL_SURFACE_V1
#define ZWP_INPUT_PANEL_SURFACE_V1
#include "fcitx-utils/signals.h"
#include "wayland-input-method-unstable-v1-client-protocol.h"
#include <memory>
#include <wayland-client.h>
namespace fcitx {
namespace wayland {
class WlOutput;
class ZwpInputPanelSurfaceV1 final {
public:
    static constexpr const char *interface = "zwp_input_panel_surface_v1";
    static constexpr const wl_interface *const wlInterface =
        &zwp_input_panel_surface_v1_interface;
    static constexpr const uint32_t version = 1;
    typedef zwp_input_panel_surface_v1 wlType;
    operator zwp_input_panel_surface_v1 *() { return data_.get(); }
    ZwpInputPanelSurfaceV1(wlType *data);
    ZwpInputPanelSurfaceV1(ZwpInputPanelSurfaceV1 &&other) noexcept = default;
    ZwpInputPanelSurfaceV1 &
    operator=(ZwpInputPanelSurfaceV1 &&other) noexcept = default;
    auto actualVersion() const { return version_; }
    void setToplevel(WlOutput *output, uint32_t position);
    void setOverlayPanel();

private:
    static void destructor(zwp_input_panel_surface_v1 *);
    uint32_t version_;
    std::unique_ptr<zwp_input_panel_surface_v1, decltype(&destructor)> data_;
};
}
}
#endif
