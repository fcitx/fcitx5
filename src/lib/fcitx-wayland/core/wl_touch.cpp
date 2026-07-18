#include "wl_touch.h"
#include <cassert>
#include "wl_surface.h"

namespace fcitx::wayland {
const struct wl_touch_listener WlTouch::listener = {
#if defined(WL_TOUCH_DOWN_SINCE_VERSION)
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
#endif
#if defined(WL_TOUCH_UP_SINCE_VERSION)
    .up =
        [](void *data, wl_touch *wldata, uint32_t serial, uint32_t time,
           int32_t id) {
            auto *obj = static_cast<WlTouch *>(data);
            assert(*obj == wldata);
            {
                obj->up()(serial, time, id);
            }
        },
#endif
#if defined(WL_TOUCH_MOTION_SINCE_VERSION)
    .motion =
        [](void *data, wl_touch *wldata, uint32_t time, int32_t id,
           wl_fixed_t x, wl_fixed_t y) {
            auto *obj = static_cast<WlTouch *>(data);
            assert(*obj == wldata);
            {
                obj->motion()(time, id, x, y);
            }
        },
#endif
#if defined(WL_TOUCH_FRAME_SINCE_VERSION)
    .frame =
        [](void *data, wl_touch *wldata) {
            auto *obj = static_cast<WlTouch *>(data);
            assert(*obj == wldata);
            {
                obj->frame()();
            }
        },
#endif
#if defined(WL_TOUCH_CANCEL_SINCE_VERSION)
    .cancel =
        [](void *data, wl_touch *wldata) {
            auto *obj = static_cast<WlTouch *>(data);
            assert(*obj == wldata);
            {
                obj->cancel()();
            }
        },
#endif
#if defined(WL_TOUCH_SHAPE_SINCE_VERSION)
    .shape =
        [](void *data, wl_touch *wldata, int32_t id, wl_fixed_t major,
           wl_fixed_t minor) {
            auto *obj = static_cast<WlTouch *>(data);
            assert(*obj == wldata);
            {
                obj->shape()(id, major, minor);
            }
        },
#endif
#if defined(WL_TOUCH_ORIENTATION_SINCE_VERSION)
    .orientation =
        [](void *data, wl_touch *wldata, int32_t id, wl_fixed_t orientation) {
            auto *obj = static_cast<WlTouch *>(data);
            assert(*obj == wldata);
            {
                obj->orientation()(id, orientation);
            }
        },
#endif
};

WlTouch::WlTouch(wl_touch *data)
    : version_(wl_touch_get_version(data)), data_(data) {
    wl_touch_set_user_data(*this, this);
    wl_touch_add_listener(*this, &WlTouch::listener, this);
}

void WlTouch::destructor(wl_touch *data) {
    const auto version = wl_touch_get_version(data);
#if defined(WL_TOUCH_RELEASE_SINCE_VERSION)
    if (version >= 3) {
        wl_touch_release(data);
        return;
    }
#endif
    wl_touch_destroy(data);
}

} // namespace fcitx::wayland
