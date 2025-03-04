#ifndef WL_SUBSURFACE_H_
#define WL_SUBSURFACE_H_
#include <cstdint>
#include <wayland-client.h>
#include <wayland-util.h>
#include "fcitx-utils/misc.h"
namespace fcitx::wayland {

class WlSurface;

class WlSubsurface final {
public:
    static constexpr const char *interface = "wl_subsurface";
    static constexpr const wl_interface *const wlInterface =
        &wl_subsurface_interface;
    static constexpr const uint32_t version = 1;
    using wlType = wl_subsurface;
    operator wl_subsurface *() { return data_.get(); }
    WlSubsurface(wlType *data);
    WlSubsurface(WlSubsurface &&other) noexcept = delete;
    WlSubsurface &operator=(WlSubsurface &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    void setPosition(int32_t x, int32_t y);
    void placeAbove(WlSurface *sibling);
    void placeBelow(WlSurface *sibling);
    void setSync();
    void setDesync();

private:
    static void destructor(wl_subsurface *);

    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<wl_subsurface, &destructor> data_;
};
static inline wl_subsurface *rawPointer(WlSubsurface *p) {
    return p ? static_cast<wl_subsurface *>(*p) : nullptr;
}

} // namespace fcitx::wayland

#endif // WL_SUBSURFACE_H_
