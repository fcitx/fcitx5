#ifndef WL_TOUCH
#define WL_TOUCH
#include "fcitx-utils/signals.h"
#include <memory>
#include <wayland-client.h>
namespace fcitx {
namespace wayland {
class WlSurface;
class WlTouch final {
public:
    static constexpr const char *interface = "wl_touch";
    static constexpr const wl_interface *const wlInterface =
        &wl_touch_interface;
    static constexpr const uint32_t version = 5;
    typedef wl_touch wlType;
    operator wl_touch *() { return data_.get(); }
    WlTouch(wlType *data);
    WlTouch(WlTouch &&other) noexcept = delete;
    WlTouch &operator=(WlTouch &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    auto &down() { return downSignal_; }
    auto &up() { return upSignal_; }
    auto &motion() { return motionSignal_; }
    auto &frame() { return frameSignal_; }
    auto &cancel() { return cancelSignal_; }

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
    uint32_t version_;
    std::unique_ptr<wl_touch, decltype(&destructor)> data_;
};
static inline wl_touch *rawPointer(WlTouch *p) {
    return p ? static_cast<wl_touch *>(*p) : nullptr;
}
}
}
#endif
