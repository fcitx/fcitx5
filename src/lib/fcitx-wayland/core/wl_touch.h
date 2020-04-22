#ifndef WL_TOUCH
#define WL_TOUCH
#include <memory>
#include <wayland-client.h>
#include "fcitx-utils/signals.h"
namespace fcitx {
namespace wayland {
class WlSurface;
class WlTouch final {
public:
    static constexpr const char *interface = "wl_touch";
    static constexpr const wl_interface *const wlInterface =
        &wl_touch_interface;
    static constexpr const uint32_t version = 7;
    typedef wl_touch wlType;
    operator wl_touch *() { return data_.get(); }
    WlTouch(wlType *data);
    WlTouch(WlTouch &&other) noexcept = delete;
    WlTouch &operator=(WlTouch &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    auto &down() { return downSignal_; }
    auto &up() { return upSignal_; }
    auto &motion() { return motionSignal_; }
    auto &frame() { return frameSignal_; }
    auto &cancel() { return cancelSignal_; }
    auto &shape() { return shapeSignal_; }
    auto &orientation() { return orientationSignal_; }

private:
    static void destructor(wl_touch *);
    static const struct wl_touch_listener listener;
    fcitx::Signal<void(uint32_t, uint32_t, WlSurface *, int32_t, wl_fixed_t,
                       wl_fixed_t)>
        downSignal_;
    fcitx::Signal<void(uint32_t, uint32_t, int32_t)> upSignal_;
    fcitx::Signal<void(uint32_t, int32_t, wl_fixed_t, wl_fixed_t)>
        motionSignal_;
    fcitx::Signal<void()> frameSignal_;
    fcitx::Signal<void()> cancelSignal_;
    fcitx::Signal<void(int32_t, wl_fixed_t, wl_fixed_t)> shapeSignal_;
    fcitx::Signal<void(int32_t, wl_fixed_t)> orientationSignal_;
    uint32_t version_;
    void *userData_ = nullptr;
    std::unique_ptr<wl_touch, decltype(&destructor)> data_;
};
static inline wl_touch *rawPointer(WlTouch *p) {
    return p ? static_cast<wl_touch *>(*p) : nullptr;
}
} // namespace wayland
} // namespace fcitx
#endif
