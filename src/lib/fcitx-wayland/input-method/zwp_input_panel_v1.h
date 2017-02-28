#ifndef ZWP_INPUT_PANEL_V1
#define ZWP_INPUT_PANEL_V1
#include "fcitx-utils/signals.h"
#include "wayland-input-method-unstable-v1-client-protocol.h"
#include <memory>
#include <wayland-client.h>
namespace fcitx {
namespace wayland {
class WlSurface;
class ZwpInputPanelSurfaceV1;
class ZwpInputPanelV1 {
public:
    static constexpr const char *interface = "zwp_input_panel_v1";
    static constexpr const wl_interface *const wlInterface = &zwp_input_panel_v1_interface;
    static constexpr const uint32_t version = 1;
    typedef zwp_input_panel_v1 wlType;
    operator zwp_input_panel_v1 *() { return data_.get(); }
    ZwpInputPanelV1(wlType *data);
    ZwpInputPanelV1(ZwpInputPanelV1 &&other) : data_(std::move(other.data_)) {}
    ZwpInputPanelV1 &operator=(ZwpInputPanelV1 &&other) {
        data_ = std::move(other.data_);
        return *this;
    }
    auto actualVersion() const { return version_; }
    ZwpInputPanelSurfaceV1 *getInputPanelSurface(WlSurface *surface);

private:
    static void destructor(zwp_input_panel_v1 *);
    uint32_t version_;
    std::unique_ptr<zwp_input_panel_v1, decltype(&destructor)> data_;
};
}
}
#endif
