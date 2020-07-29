#ifndef ZWP_INPUT_POPUP_SURFACE_V2
#define ZWP_INPUT_POPUP_SURFACE_V2
#include <memory>
#include <wayland-client.h>
#include "fcitx-utils/signals.h"
#include "wayland-input-method-unstable-v2-client-protocol.h"
namespace fcitx {
namespace wayland {
class ZwpInputPopupSurfaceV2 final {
public:
    static constexpr const char *interface = "zwp_input_popup_surface_v2";
    static constexpr const wl_interface *const wlInterface =
        &zwp_input_popup_surface_v2_interface;
    static constexpr const uint32_t version = 1;
    typedef zwp_input_popup_surface_v2 wlType;
    operator zwp_input_popup_surface_v2 *() { return data_.get(); }
    ZwpInputPopupSurfaceV2(wlType *data);
    ZwpInputPopupSurfaceV2(ZwpInputPopupSurfaceV2 &&other) noexcept = delete;
    ZwpInputPopupSurfaceV2 &
    operator=(ZwpInputPopupSurfaceV2 &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    auto &textInputRectangle() { return textInputRectangleSignal_; }

private:
    static void destructor(zwp_input_popup_surface_v2 *);
    static const struct zwp_input_popup_surface_v2_listener listener;
    fcitx::Signal<void(int32_t, int32_t, int32_t, int32_t)>
        textInputRectangleSignal_;
    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<zwp_input_popup_surface_v2, &destructor> data_;
};
static inline zwp_input_popup_surface_v2 *
rawPointer(ZwpInputPopupSurfaceV2 *p) {
    return p ? static_cast<zwp_input_popup_surface_v2 *>(*p) : nullptr;
}
} // namespace wayland
} // namespace fcitx
#endif
