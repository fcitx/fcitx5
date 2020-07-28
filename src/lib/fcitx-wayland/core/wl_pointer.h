#ifndef WL_POINTER
#define WL_POINTER
#include <memory>
#include <wayland-client.h>
#include "fcitx-utils/signals.h"
namespace fcitx {
namespace wayland {
class WlSurface;
class WlPointer final {
public:
    static constexpr const char *interface = "wl_pointer";
    static constexpr const wl_interface *const wlInterface =
        &wl_pointer_interface;
    static constexpr const uint32_t version = 7;
    typedef wl_pointer wlType;
    operator wl_pointer *() { return data_.get(); }
    WlPointer(wlType *data);
    WlPointer(WlPointer &&other) noexcept = delete;
    WlPointer &operator=(WlPointer &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    void setCursor(uint32_t serial, WlSurface *surface, int32_t hotspotX,
                   int32_t hotspotY);
    auto &enter() { return enterSignal_; }
    auto &leave() { return leaveSignal_; }
    auto &motion() { return motionSignal_; }
    auto &button() { return buttonSignal_; }
    auto &axis() { return axisSignal_; }
    auto &frame() { return frameSignal_; }
    auto &axisSource() { return axisSourceSignal_; }
    auto &axisStop() { return axisStopSignal_; }
    auto &axisDiscrete() { return axisDiscreteSignal_; }

private:
    static void destructor(wl_pointer *);
    static const struct wl_pointer_listener listener;
    fcitx::Signal<void(uint32_t, WlSurface *, wl_fixed_t, wl_fixed_t)>
        enterSignal_;
    fcitx::Signal<void(uint32_t, WlSurface *)> leaveSignal_;
    fcitx::Signal<void(uint32_t, wl_fixed_t, wl_fixed_t)> motionSignal_;
    fcitx::Signal<void(uint32_t, uint32_t, uint32_t, uint32_t)> buttonSignal_;
    fcitx::Signal<void(uint32_t, uint32_t, wl_fixed_t)> axisSignal_;
    fcitx::Signal<void()> frameSignal_;
    fcitx::Signal<void(uint32_t)> axisSourceSignal_;
    fcitx::Signal<void(uint32_t, uint32_t)> axisStopSignal_;
    fcitx::Signal<void(uint32_t, int32_t)> axisDiscreteSignal_;
    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<wl_pointer, &destructor> data_;
};
static inline wl_pointer *rawPointer(WlPointer *p) {
    return p ? static_cast<wl_pointer *>(*p) : nullptr;
}
} // namespace wayland
} // namespace fcitx
#endif
