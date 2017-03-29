#ifndef WL_COMPOSITOR
#define WL_COMPOSITOR
#include "fcitx-utils/signals.h"
#include <memory>
#include <wayland-client.h>
namespace fcitx {
namespace wayland {
class WlRegion;
class WlSurface;
class WlCompositor {
public:
    static constexpr const char *interface = "wl_compositor";
    static constexpr const wl_interface *const wlInterface =
        &wl_compositor_interface;
    static constexpr const uint32_t version = 4;
    typedef wl_compositor wlType;
    operator wl_compositor *() { return data_.get(); }
    WlCompositor(wlType *data);
    WlCompositor(WlCompositor &&other) : data_(std::move(other.data_)) {}
    WlCompositor &operator=(WlCompositor &&other) {
        data_ = std::move(other.data_);
        return *this;
    }
    auto actualVersion() const { return version_; }
    WlSurface *createSurface();
    WlRegion *createRegion();

private:
    static void destructor(wl_compositor *);
    uint32_t version_;
    std::unique_ptr<wl_compositor, decltype(&destructor)> data_;
};
}
}
#endif
