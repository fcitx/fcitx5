#ifndef WL_REGION
#define WL_REGION
#include <wayland-client.h>
#include "fcitx-utils/signals.h"
namespace fcitx::wayland {
class WlRegion final {
public:
    static constexpr const char *interface = "wl_region";
    static constexpr const wl_interface *const wlInterface =
        &wl_region_interface;
    static constexpr const uint32_t version = 1;
    typedef wl_region wlType;
    operator wl_region *() { return data_.get(); }
    WlRegion(wlType *data);
    WlRegion(WlRegion &&other) noexcept = delete;
    WlRegion &operator=(WlRegion &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    void add(int32_t x, int32_t y, int32_t width, int32_t height);
    void subtract(int32_t x, int32_t y, int32_t width, int32_t height);

private:
    static void destructor(wl_region *);
    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<wl_region, &destructor> data_;
};
static inline wl_region *rawPointer(WlRegion *p) {
    return p ? static_cast<wl_region *>(*p) : nullptr;
}
} // namespace fcitx::wayland
#endif
