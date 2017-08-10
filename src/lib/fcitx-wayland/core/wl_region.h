#ifndef WL_REGION
#define WL_REGION
#include "fcitx-utils/signals.h"
#include <memory>
#include <wayland-client.h>
namespace fcitx {
namespace wayland {
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
    void add(int32_t x, int32_t y, int32_t width, int32_t height);
    void subtract(int32_t x, int32_t y, int32_t width, int32_t height);

private:
    static void destructor(wl_region *);
    uint32_t version_;
    std::unique_ptr<wl_region, decltype(&destructor)> data_;
};
}
}
#endif
