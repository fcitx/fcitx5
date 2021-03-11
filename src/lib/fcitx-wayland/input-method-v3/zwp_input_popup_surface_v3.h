#ifndef ZWP_INPUT_POPUP_SURFACE_V3
#define ZWP_INPUT_POPUP_SURFACE_V3
#include <memory>
#include <wayland-client.h>
#include "fcitx-utils/signals.h"
#include "wayland-input-method-unstable-v3-client-protocol.h"
namespace fcitx::wayland {
class ZwpInputPopupSurfaceV3 final {
public:
    static constexpr const char *interface = "zwp_input_popup_surface_v3";
    static constexpr const wl_interface *const wlInterface =
        &zwp_input_popup_surface_v3_interface;
    static constexpr const uint32_t version = 1;
    typedef zwp_input_popup_surface_v3 wlType;
    operator zwp_input_popup_surface_v3 *() { return data_.get(); }
    ZwpInputPopupSurfaceV3(wlType *data);
    ZwpInputPopupSurfaceV3(ZwpInputPopupSurfaceV3 &&other) noexcept = delete;
    ZwpInputPopupSurfaceV3 &
    operator=(ZwpInputPopupSurfaceV3 &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }

private:
    static void destructor(zwp_input_popup_surface_v3 *);
    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<zwp_input_popup_surface_v3, &destructor> data_;
};
static inline zwp_input_popup_surface_v3 *
rawPointer(ZwpInputPopupSurfaceV3 *p) {
    return p ? static_cast<zwp_input_popup_surface_v3 *>(*p) : nullptr;
}
} // namespace fcitx::wayland
#endif
