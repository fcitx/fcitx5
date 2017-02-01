#ifndef WL_SUBCOMPOSITOR
#define WL_SUBCOMPOSITOR
#include <wayland-client.h>
#include <memory>
#include "fcitx-utils/signals.h"
namespace fcitx {
namespace wayland {
class WlSubsurface;
class WlSurface;
class WlSubcompositor {
public:
    static constexpr const char *interface = "wl_subcompositor";
    static constexpr const wl_interface *const wlInterface = &wl_subcompositor_interface;
    static constexpr const uint32_t version = 1;
    typedef wl_subcompositor wlType;
    operator wl_subcompositor *() { return data_.get(); }
    WlSubcompositor(wlType *data);
    WlSubcompositor(WlSubcompositor &&other) : data_(std::move(other.data_)) {}
    WlSubcompositor &operator=(WlSubcompositor &&other) {
        data_ = std::move(other.data_);
        return *this;
    }
    auto actualVersion() const { return version_; }
    WlSubsurface *getSubsurface(WlSurface *surface, WlSurface *parent);
private:
    static void destructor(wl_subcompositor *);
    uint32_t version_;
    std::unique_ptr<wl_subcompositor, decltype(&destructor)> data_;
};
}
}
#endif
