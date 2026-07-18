#include "wl_pointer.h"
#include <cassert>
#include "wl_surface.h"

namespace fcitx::wayland {
const struct wl_pointer_listener WlPointer::listener = {
#if defined(WL_POINTER_ENTER_SINCE_VERSION)
    .enter =
        [](void *data, wl_pointer *wldata, uint32_t serial, wl_surface *surface,
           wl_fixed_t surfaceX, wl_fixed_t surfaceY) {
            auto *obj = static_cast<WlPointer *>(data);
            assert(*obj == wldata);
            {
                if (!surface) {
                    return;
                }
                auto *surface_ =
                    static_cast<WlSurface *>(wl_surface_get_user_data(surface));
                obj->enter()(serial, surface_, surfaceX, surfaceY);
            }
        },
#endif
#if defined(WL_POINTER_LEAVE_SINCE_VERSION)
    .leave =
        [](void *data, wl_pointer *wldata, uint32_t serial,
           wl_surface *surface) {
            auto *obj = static_cast<WlPointer *>(data);
            assert(*obj == wldata);
            {
                if (!surface) {
                    return;
                }
                auto *surface_ =
                    static_cast<WlSurface *>(wl_surface_get_user_data(surface));
                obj->leave()(serial, surface_);
            }
        },
#endif
#if defined(WL_POINTER_MOTION_SINCE_VERSION)
    .motion =
        [](void *data, wl_pointer *wldata, uint32_t time, wl_fixed_t surfaceX,
           wl_fixed_t surfaceY) {
            auto *obj = static_cast<WlPointer *>(data);
            assert(*obj == wldata);
            {
                obj->motion()(time, surfaceX, surfaceY);
            }
        },
#endif
#if defined(WL_POINTER_BUTTON_SINCE_VERSION)
    .button =
        [](void *data, wl_pointer *wldata, uint32_t serial, uint32_t time,
           uint32_t button, uint32_t state) {
            auto *obj = static_cast<WlPointer *>(data);
            assert(*obj == wldata);
            {
                obj->button()(serial, time, button, state);
            }
        },
#endif
#if defined(WL_POINTER_AXIS_SINCE_VERSION)
    .axis =
        [](void *data, wl_pointer *wldata, uint32_t time, uint32_t axis,
           wl_fixed_t value) {
            auto *obj = static_cast<WlPointer *>(data);
            assert(*obj == wldata);
            {
                obj->axis()(time, axis, value);
            }
        },
#endif
#if defined(WL_POINTER_FRAME_SINCE_VERSION)
    .frame =
        [](void *data, wl_pointer *wldata) {
            auto *obj = static_cast<WlPointer *>(data);
            assert(*obj == wldata);
            {
                obj->frame()();
            }
        },
#endif
#if defined(WL_POINTER_AXIS_SOURCE_SINCE_VERSION)
    .axis_source =
        [](void *data, wl_pointer *wldata, uint32_t axisSource) {
            auto *obj = static_cast<WlPointer *>(data);
            assert(*obj == wldata);
            {
                obj->axisSource()(axisSource);
            }
        },
#endif
#if defined(WL_POINTER_AXIS_STOP_SINCE_VERSION)
    .axis_stop =
        [](void *data, wl_pointer *wldata, uint32_t time, uint32_t axis) {
            auto *obj = static_cast<WlPointer *>(data);
            assert(*obj == wldata);
            {
                obj->axisStop()(time, axis);
            }
        },
#endif
#if defined(WL_POINTER_AXIS_DISCRETE_SINCE_VERSION)
    .axis_discrete =
        [](void *data, wl_pointer *wldata, uint32_t axis, int32_t discrete) {
            auto *obj = static_cast<WlPointer *>(data);
            assert(*obj == wldata);
            {
                obj->axisDiscrete()(axis, discrete);
            }
        },
#endif
#if defined(WL_POINTER_AXIS_VALUE120_SINCE_VERSION)
    .axis_value120 =
        [](void *data, wl_pointer *wldata, uint32_t axis, int32_t value120) {
            auto *obj = static_cast<WlPointer *>(data);
            assert(*obj == wldata);
            {
                obj->axisValue120()(axis, value120);
            }
        },
#endif
#if defined(WL_POINTER_AXIS_RELATIVE_DIRECTION_SINCE_VERSION)
    .axis_relative_direction =
        [](void *data, wl_pointer *wldata, uint32_t axis, uint32_t direction) {
            auto *obj = static_cast<WlPointer *>(data);
            assert(*obj == wldata);
            {
                obj->axisRelativeDirection()(axis, direction);
            }
        },
#endif
#if defined(WL_POINTER_WARP_SINCE_VERSION)
    .warp =
        [](void *data, wl_pointer *wldata, wl_fixed_t surfaceX,
           wl_fixed_t surfaceY) {
            auto *obj = static_cast<WlPointer *>(data);
            assert(*obj == wldata);
            {
                obj->warp()(surfaceX, surfaceY);
            }
        },
#endif
};

WlPointer::WlPointer(wl_pointer *data)
    : version_(wl_pointer_get_version(data)), data_(data) {
    wl_pointer_set_user_data(*this, this);
    wl_pointer_add_listener(*this, &WlPointer::listener, this);
}

void WlPointer::destructor(wl_pointer *data) {
    const auto version = wl_pointer_get_version(data);
#if defined(WL_POINTER_RELEASE_SINCE_VERSION)
    if (version >= 3) {
        wl_pointer_release(data);
        return;
    }
#endif
    wl_pointer_destroy(data);
}
#if defined(WL_POINTER_SET_CURSOR_SINCE_VERSION)
void WlPointer::setCursor(uint32_t serial, WlSurface *surface, int32_t hotspotX,
                          int32_t hotspotY) {
    wl_pointer_set_cursor(*this, serial, rawPointer(surface), hotspotX,
                          hotspotY);
}
#endif

} // namespace fcitx::wayland
