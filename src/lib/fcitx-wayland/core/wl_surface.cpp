#include "wl_surface.h"
#include "wl_buffer.h"
#include "wl_callback.h"
#include "wl_output.h"
#include "wl_region.h"
#include <cassert>
namespace fcitx {
namespace wayland {
constexpr const char *WlSurface::interface;
constexpr const wl_interface *const WlSurface::wlInterface;
const uint32_t WlSurface::version;
const struct wl_surface_listener WlSurface::listener = {
    [](void *data, wl_surface *wldata, wl_output *output) {
        auto obj = static_cast<WlSurface *>(data);
        assert(*obj == wldata);
        {
            auto output_ =
                static_cast<WlOutput *>(wl_output_get_user_data(output));
            return obj->enter()(output_);
        }
    },
    [](void *data, wl_surface *wldata, wl_output *output) {
        auto obj = static_cast<WlSurface *>(data);
        assert(*obj == wldata);
        {
            auto output_ =
                static_cast<WlOutput *>(wl_output_get_user_data(output));
            return obj->leave()(output_);
        }
    },
};
WlSurface::WlSurface(wl_surface *data)
    : version_(wl_surface_get_version(data)),
      data_(data, &WlSurface::destructor) {
    wl_surface_set_user_data(*this, this);
    wl_surface_add_listener(*this, &WlSurface::listener, this);
}
void WlSurface::destructor(wl_surface *data) {
    auto version = wl_surface_get_version(data);
    if (version >= 1) {
        return wl_surface_destroy(data);
    }
}
void WlSurface::attach(WlBuffer *buffer, int32_t x, int32_t y) {
    return wl_surface_attach(*this, *buffer, x, y);
}
void WlSurface::damage(int32_t x, int32_t y, int32_t width, int32_t height) {
    return wl_surface_damage(*this, x, y, width, height);
}
WlCallback *WlSurface::frame() {
    return new WlCallback(wl_surface_frame(*this));
}
void WlSurface::setOpaqueRegion(WlRegion *region) {
    return wl_surface_set_opaque_region(*this, *region);
}
void WlSurface::setInputRegion(WlRegion *region) {
    return wl_surface_set_input_region(*this, *region);
}
void WlSurface::commit() { return wl_surface_commit(*this); }
void WlSurface::setBufferTransform(int32_t transform) {
    return wl_surface_set_buffer_transform(*this, transform);
}
void WlSurface::setBufferScale(int32_t scale) {
    return wl_surface_set_buffer_scale(*this, scale);
}
void WlSurface::damageBuffer(int32_t x, int32_t y, int32_t width,
                             int32_t height) {
    return wl_surface_damage_buffer(*this, x, y, width, height);
}
}
}
