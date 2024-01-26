#ifndef WL_SUBCOMPOSITOR
#define WL_SUBCOMPOSITOR
#include <wayland-client.h>
#include "fcitx-utils/signals.h"
namespace fcitx::wayland {
class WlSubsurface;
class WlSurface;
class WlSubcompositor final {
public:
    static constexpr const char *interface = "wl_subcompositor";
    static constexpr const wl_interface *const wlInterface =
        &wl_subcompositor_interface;
    static constexpr const uint32_t version = 1;
    typedef wl_subcompositor wlType;
    operator wl_subcompositor *() { return data_.get(); }
    WlSubcompositor(wlType *data);
    WlSubcompositor(WlSubcompositor &&other) noexcept = delete;
    WlSubcompositor &operator=(WlSubcompositor &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    WlSubsurface *getSubsurface(WlSurface *surface, WlSurface *parent);

private:
    static void destructor(wl_subcompositor *);
    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<wl_subcompositor, &destructor> data_;
};
static inline wl_subcompositor *rawPointer(WlSubcompositor *p) {
    return p ? static_cast<wl_subcompositor *>(*p) : nullptr;
}
} // namespace fcitx::wayland
#endif
