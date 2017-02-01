#include "wl_pointer.h"
#include "wl_surface.h"
#include <cassert>
namespace fcitx {
namespace wayland {
constexpr const char *WlPointer::interface;
constexpr const wl_interface *const WlPointer::wlInterface;
const uint32_t WlPointer::version;
const struct wl_pointer_listener WlPointer::listener = {
    [](void *data, wl_pointer *wldata, uint32_t serial, wl_surface *surface, wl_fixed_t surfaceX, wl_fixed_t surfaceY) {
        auto obj = static_cast<WlPointer *>(data);
        assert(*obj == wldata);
        {
            auto surface_ = static_cast<WlSurface *>(wl_surface_get_user_data(surface));
            return obj->enter()(serial, surface_, surfaceX, surfaceY);
        }
    },
    [](void *data, wl_pointer *wldata, uint32_t serial, wl_surface *surface) {
        auto obj = static_cast<WlPointer *>(data);
        assert(*obj == wldata);
        {
            auto surface_ = static_cast<WlSurface *>(wl_surface_get_user_data(surface));
            return obj->leave()(serial, surface_);
        }
    },
    [](void *data, wl_pointer *wldata, uint32_t time, wl_fixed_t surfaceX, wl_fixed_t surfaceY) {
        auto obj = static_cast<WlPointer *>(data);
        assert(*obj == wldata);
        {
            return obj->motion()(time, surfaceX, surfaceY);
        }
    },
    [](void *data, wl_pointer *wldata, uint32_t serial, uint32_t time, uint32_t button, uint32_t state) {
        auto obj = static_cast<WlPointer *>(data);
        assert(*obj == wldata);
        {
            return obj->button()(serial, time, button, state);
        }
    },
    [](void *data, wl_pointer *wldata, uint32_t time, uint32_t axis, wl_fixed_t value) {
        auto obj = static_cast<WlPointer *>(data);
        assert(*obj == wldata);
        {
            return obj->axis()(time, axis, value);
        }
    },
    [](void *data, wl_pointer *wldata) {
        auto obj = static_cast<WlPointer *>(data);
        assert(*obj == wldata);
        {
            return obj->frame()();
        }
    },
    [](void *data, wl_pointer *wldata, uint32_t axisSource) {
        auto obj = static_cast<WlPointer *>(data);
        assert(*obj == wldata);
        {
            return obj->axisSource()(axisSource);
        }
    },
    [](void *data, wl_pointer *wldata, uint32_t time, uint32_t axis) {
        auto obj = static_cast<WlPointer *>(data);
        assert(*obj == wldata);
        {
            return obj->axisStop()(time, axis);
        }
    },
    [](void *data, wl_pointer *wldata, uint32_t axis, int32_t discrete) {
        auto obj = static_cast<WlPointer *>(data);
        assert(*obj == wldata);
        {
            return obj->axisDiscrete()(axis, discrete);
        }
    },
};
WlPointer::WlPointer(wl_pointer *data) : version_(wl_pointer_get_version(data)), data_(data, &WlPointer::destructor) {
    wl_pointer_set_user_data(*this, this);
    wl_pointer_add_listener(*this, &WlPointer::listener, this);
}
void WlPointer::destructor(wl_pointer *data) {
    auto version = wl_pointer_get_version(data);
    if (version >= 3) {
        return wl_pointer_release(data);
    } else {
        return wl_pointer_destroy(data);
    }
}
void WlPointer::setCursor(uint32_t serial, WlSurface *surface, int32_t hotspotX, int32_t hotspotY) {
    return wl_pointer_set_cursor(*this, serial, *surface, hotspotX, hotspotY);
}
}
}
