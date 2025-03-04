#ifndef WL_SURFACE_H_
#define WL_SURFACE_H_
#include <cstdint>
#include <wayland-client.h>
#include <wayland-util.h>
#include "fcitx-utils/misc.h"
#include "fcitx-utils/signals.h"
namespace fcitx::wayland {

class WlBuffer;
class WlCallback;
class WlOutput;
class WlRegion;

class WlSurface final {
public:
    static constexpr const char *interface = "wl_surface";
    static constexpr const wl_interface *const wlInterface =
        &wl_surface_interface;
    static constexpr const uint32_t version = 6;
    using wlType = wl_surface;
    operator wl_surface *() { return data_.get(); }
    WlSurface(wlType *data);
    WlSurface(WlSurface &&other) noexcept = delete;
    WlSurface &operator=(WlSurface &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    void attach(WlBuffer *buffer, int32_t x, int32_t y);
    void damage(int32_t x, int32_t y, int32_t width, int32_t height);
    WlCallback *frame();
    void setOpaqueRegion(WlRegion *region);
    void setInputRegion(WlRegion *region);
    void commit();
    void setBufferTransform(int32_t transform);
    void setBufferScale(int32_t scale);
    void damageBuffer(int32_t x, int32_t y, int32_t width, int32_t height);
    void offset(int32_t x, int32_t y);

    auto &enter() { return enterSignal_; }
    auto &leave() { return leaveSignal_; }
    auto &preferredBufferScale() { return preferredBufferScaleSignal_; }
    auto &preferredBufferTransform() { return preferredBufferTransformSignal_; }

private:
    static void destructor(wl_surface *);
    static const struct wl_surface_listener listener;
    fcitx::Signal<void(WlOutput *)> enterSignal_;
    fcitx::Signal<void(WlOutput *)> leaveSignal_;
    fcitx::Signal<void(int32_t)> preferredBufferScaleSignal_;
    fcitx::Signal<void(uint32_t)> preferredBufferTransformSignal_;

    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<wl_surface, &destructor> data_;
};
static inline wl_surface *rawPointer(WlSurface *p) {
    return p ? static_cast<wl_surface *>(*p) : nullptr;
}

} // namespace fcitx::wayland

#endif // WL_SURFACE_H_
