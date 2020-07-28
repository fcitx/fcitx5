#ifndef ZWP_INPUT_PANEL_SURFACE_V1
#define ZWP_INPUT_PANEL_SURFACE_V1
#include <memory>
#include <wayland-client.h>
#include "fcitx-utils/signals.h"
#include "wayland-input-method-unstable-v1-client-protocol.h"
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
    ZwpInputPanelSurfaceV1(ZwpInputPanelSurfaceV1 &&other) noexcept = delete;
    ZwpInputPanelSurfaceV1 &
    operator=(ZwpInputPanelSurfaceV1 &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    void setToplevel(WlOutput *output, uint32_t position);
    void setOverlayPanel();

private:
    static void destructor(zwp_input_panel_surface_v1 *);
    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<zwp_input_panel_surface_v1, &destructor> data_;
};
static inline zwp_input_panel_surface_v1 *
rawPointer(ZwpInputPanelSurfaceV1 *p) {
    return p ? static_cast<zwp_input_panel_surface_v1 *>(*p) : nullptr;
}
} // namespace wayland
} // namespace fcitx
#endif
