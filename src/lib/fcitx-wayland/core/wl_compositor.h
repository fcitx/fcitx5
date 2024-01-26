#ifndef WL_COMPOSITOR
#define WL_COMPOSITOR
#include <wayland-client.h>
#include "fcitx-utils/signals.h"
namespace fcitx::wayland {
class WlRegion;
class WlSurface;
class WlCompositor final {
public:
    static constexpr const char *interface = "wl_compositor";
    static constexpr const wl_interface *const wlInterface =
        &wl_compositor_interface;
    static constexpr const uint32_t version = 4;
    typedef wl_compositor wlType;
    operator wl_compositor *() { return data_.get(); }
    WlCompositor(wlType *data);
    WlCompositor(WlCompositor &&other) noexcept = delete;
    WlCompositor &operator=(WlCompositor &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    WlSurface *createSurface();
    WlRegion *createRegion();

private:
    static void destructor(wl_compositor *);
    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<wl_compositor, &destructor> data_;
};
static inline wl_compositor *rawPointer(WlCompositor *p) {
    return p ? static_cast<wl_compositor *>(*p) : nullptr;
}
} // namespace fcitx::wayland
#endif
