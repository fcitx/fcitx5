#include "wl_region.h"
#include <cassert>
namespace fcitx {
namespace wayland {
constexpr const char *WlRegion::interface;
constexpr const wl_interface *const WlRegion::wlInterface;
const uint32_t WlRegion::version;
WlRegion::WlRegion(wl_region *data)
    : version_(wl_region_get_version(data)),
      data_(data, &WlRegion::destructor) {
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
}
}
