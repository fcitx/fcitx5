#ifndef WL_SURFACE
#define WL_SURFACE
#include "fcitx-utils/signals.h"
#include <memory>
#include <wayland-client.h>
namespace fcitx {
namespace wayland {
class WlBuffer;
class WlCallback;
class WlOutput;
class WlRegion;
class WlSurface final {
public:
    static constexpr const char *interface = "wl_surface";
    static constexpr const wl_interface *const wlInterface =
        &wl_surface_interface;
    static constexpr const uint32_t version = 4;
    typedef wl_surface wlType;
    operator wl_surface *() { return data_.get(); }
    WlSurface(wlType *data);
    WlSurface(WlSurface &&other) noexcept = delete;
    WlSurface &operator=(WlSurface &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void attach(WlBuffer *buffer, int32_t x, int32_t y);
    void damage(int32_t x, int32_t y, int32_t width, int32_t height);
    WlCallback *frame();
    void setOpaqueRegion(WlRegion *region);
    void setInputRegion(WlRegion *region);
    void commit();
    void setBufferTransform(int32_t transform);
    void setBufferScale(int32_t scale);
    void damageBuffer(int32_t x, int32_t y, int32_t width, int32_t height);
    auto &enter() { return enterSignal_; }
    auto &leave() { return leaveSignal_; }

private:
    static void destructor(wl_surface *);
    static const struct wl_surface_listener listener;
    fcitx::Signal<void(WlOutput *)> enterSignal_;
    fcitx::Signal<void(WlOutput *)> leaveSignal_;
    uint32_t version_;
    std::unique_ptr<wl_surface, decltype(&destructor)> data_;
};
}
}
#endif
