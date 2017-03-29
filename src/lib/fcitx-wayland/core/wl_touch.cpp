#include "wl_touch.h"
#include "wl_surface.h"
#include <cassert>
namespace fcitx {
namespace wayland {
constexpr const char *WlTouch::interface;
constexpr const wl_interface *const WlTouch::wlInterface;
const uint32_t WlTouch::version;
const struct wl_touch_listener WlTouch::listener = {
    [](void *data, wl_touch *wldata, uint32_t serial, uint32_t time,
       wl_surface *surface, int32_t id, wl_fixed_t x, wl_fixed_t y) {
        auto obj = static_cast<WlTouch *>(data);
        assert(*obj == wldata);
        {
            auto surface_ =
                static_cast<WlSurface *>(wl_surface_get_user_data(surface));
            return obj->down()(serial, time, surface_, id, x, y);
        }
    },
    [](void *data, wl_touch *wldata, uint32_t serial, uint32_t time,
       int32_t id) {
        auto obj = static_cast<WlTouch *>(data);
        assert(*obj == wldata);
        { return obj->up()(serial, time, id); }
    },
    [](void *data, wl_touch *wldata, uint32_t time, int32_t id, wl_fixed_t x,
       wl_fixed_t y) {
        auto obj = static_cast<WlTouch *>(data);
        assert(*obj == wldata);
        { return obj->motion()(time, id, x, y); }
    },
    [](void *data, wl_touch *wldata) {
        auto obj = static_cast<WlTouch *>(data);
        assert(*obj == wldata);
        { return obj->frame()(); }
    },
    [](void *data, wl_touch *wldata) {
        auto obj = static_cast<WlTouch *>(data);
        assert(*obj == wldata);
        { return obj->cancel()(); }
    },
};
WlTouch::WlTouch(wl_touch *data)
    : version_(wl_touch_get_version(data)), data_(data, &WlTouch::destructor) {
    wl_touch_set_user_data(*this, this);
    wl_touch_add_listener(*this, &WlTouch::listener, this);
}
void WlTouch::destructor(wl_touch *data) {
    auto version = wl_touch_get_version(data);
    if (version >= 3) {
        return wl_touch_release(data);
    } else {
        return wl_touch_destroy(data);
    }
}
}
}
