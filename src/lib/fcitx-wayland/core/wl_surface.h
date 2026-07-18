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
    static constexpr const uint32_t version = 7;
    using wlType = wl_surface;
    operator wl_surface *() { return data_.get(); }
    WlSurface(wlType *data);
    WlSurface(WlSurface &&other) noexcept = delete;
    WlSurface &operator=(WlSurface &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
#if defined(WL_SURFACE_ATTACH_SINCE_VERSION)
    void attach(WlBuffer *buffer, int32_t x, int32_t y);
#endif
#if defined(WL_SURFACE_DAMAGE_SINCE_VERSION)
    void damage(int32_t x, int32_t y, int32_t width, int32_t height);
#endif
#if defined(WL_SURFACE_FRAME_SINCE_VERSION)
    WlCallback *frame();
#endif
#if defined(WL_SURFACE_SET_OPAQUE_REGION_SINCE_VERSION)
    void setOpaqueRegion(WlRegion *region);
#endif
#if defined(WL_SURFACE_SET_INPUT_REGION_SINCE_VERSION)
    void setInputRegion(WlRegion *region);
#endif
#if defined(WL_SURFACE_COMMIT_SINCE_VERSION)
    void commit();
#endif
#if defined(WL_SURFACE_SET_BUFFER_TRANSFORM_SINCE_VERSION)
    void setBufferTransform(int32_t transform);
#endif
#if defined(WL_SURFACE_SET_BUFFER_SCALE_SINCE_VERSION)
    void setBufferScale(int32_t scale);
#endif
#if defined(WL_SURFACE_DAMAGE_BUFFER_SINCE_VERSION)
    void damageBuffer(int32_t x, int32_t y, int32_t width, int32_t height);
#endif
#if defined(WL_SURFACE_OFFSET_SINCE_VERSION)
    void offset(int32_t x, int32_t y);
#endif
#if defined(WL_SURFACE_GET_RELEASE_SINCE_VERSION)
    WlCallback *getRelease();
#endif

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
