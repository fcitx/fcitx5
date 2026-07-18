#include "wl_region.h"

namespace fcitx::wayland {

WlRegion::WlRegion(wl_region *data)
    : version_(wl_region_get_version(data)), data_(data) {
    wl_region_set_user_data(*this, this);
}

void WlRegion::destructor(wl_region *data) { wl_region_destroy(data); }
#if defined(WL_REGION_ADD_SINCE_VERSION)
void WlRegion::add(int32_t x, int32_t y, int32_t width, int32_t height) {
    wl_region_add(*this, x, y, width, height);
}
#endif
#if defined(WL_REGION_SUBTRACT_SINCE_VERSION)
void WlRegion::subtract(int32_t x, int32_t y, int32_t width, int32_t height) {
    wl_region_subtract(*this, x, y, width, height);
}
#endif

} // namespace fcitx::wayland
