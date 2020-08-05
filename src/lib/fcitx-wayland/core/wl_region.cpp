#include "wl_region.h"
#include <cassert>
namespace fcitx::wayland {
WlRegion::WlRegion(wl_region *data)
    : version_(wl_region_get_version(data)), data_(data) {
    wl_region_set_user_data(*this, this);
}
void WlRegion::destructor(wl_region *data) {
    auto version = wl_region_get_version(data);
    if (version >= 1) {
        return wl_region_destroy(data);
    }
}
void WlRegion::add(int32_t x, int32_t y, int32_t width, int32_t height) {
    return wl_region_add(*this, x, y, width, height);
}
void WlRegion::subtract(int32_t x, int32_t y, int32_t width, int32_t height) {
    return wl_region_subtract(*this, x, y, width, height);
}
} // namespace fcitx::wayland
