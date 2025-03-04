#include "wl_touch.h"
#include <cassert>
#include "wl_surface.h"

namespace fcitx::wayland {
const struct wl_touch_listener WlTouch::listener = {
    .down =
        [](void *data, wl_touch *wldata, uint32_t serial, uint32_t time,
           wl_surface *surface, int32_t id, wl_fixed_t x, wl_fixed_t y) {
            auto *obj = static_cast<WlTouch *>(data);
            assert(*obj == wldata);
            {
                if (!surface) {
                    return;
                }
                auto *surface_ =
                    static_cast<WlSurface *>(wl_surface_get_user_data(surface));
                obj->down()(serial, time, surface_, id, x, y);
            }
        },
    .up =
        [](void *data, wl_touch *wldata, uint32_t serial, uint32_t time,
           int32_t id) {
            auto *obj = static_cast<WlTouch *>(data);
            assert(*obj == wldata);
            {
                obj->up()(serial, time, id);
            }
        },
    .motion =
        [](void *data, wl_touch *wldata, uint32_t time, int32_t id,
           wl_fixed_t x, wl_fixed_t y) {
            auto *obj = static_cast<WlTouch *>(data);
            assert(*obj == wldata);
            {
                obj->motion()(time, id, x, y);
            }
        },
    .frame =
        [](void *data, wl_touch *wldata) {
            auto *obj = static_cast<WlTouch *>(data);
            assert(*obj == wldata);
            {
                obj->frame()();
            }
        },
    .cancel =
        [](void *data, wl_touch *wldata) {
            auto *obj = static_cast<WlTouch *>(data);
            assert(*obj == wldata);
            {
                obj->cancel()();
            }
        },
    .shape =
        [](void *data, wl_touch *wldata, int32_t id, wl_fixed_t major,
           wl_fixed_t minor) {
            auto *obj = static_cast<WlTouch *>(data);
            assert(*obj == wldata);
            {
                obj->shape()(id, major, minor);
            }
        },
    .orientation =
        [](void *data, wl_touch *wldata, int32_t id, wl_fixed_t orientation) {
            auto *obj = static_cast<WlTouch *>(data);
            assert(*obj == wldata);
            {
                obj->orientation()(id, orientation);
            }
        },
};

WlTouch::WlTouch(wl_touch *data)
    : version_(wl_touch_get_version(data)), data_(data) {
    wl_touch_set_user_data(*this, this);
    wl_touch_add_listener(*this, &WlTouch::listener, this);
}

void WlTouch::destructor(wl_touch *data) {
    const auto version = wl_touch_get_version(data);
    if (version >= 3) {
        wl_touch_release(data);
        return;
    }
    wl_touch_destroy(data);
}

} // namespace fcitx::wayland
