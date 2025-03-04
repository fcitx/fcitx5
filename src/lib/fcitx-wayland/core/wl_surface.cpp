#include "wl_surface.h"
#include <cassert>
#include "wl_buffer.h"
#include "wl_callback.h"
#include "wl_output.h"
#include "wl_region.h"

namespace fcitx::wayland {
const struct wl_surface_listener WlSurface::listener = {
    .enter =
        [](void *data, wl_surface *wldata, wl_output *output) {
            auto *obj = static_cast<WlSurface *>(data);
            assert(*obj == wldata);
            {
                if (!output) {
                    return;
                }
                auto *output_ =
                    static_cast<WlOutput *>(wl_output_get_user_data(output));
                obj->enter()(output_);
            }
        },
    .leave =
        [](void *data, wl_surface *wldata, wl_output *output) {
            auto *obj = static_cast<WlSurface *>(data);
            assert(*obj == wldata);
            {
                if (!output) {
                    return;
                }
                auto *output_ =
                    static_cast<WlOutput *>(wl_output_get_user_data(output));
                obj->leave()(output_);
            }
        },
    .preferred_buffer_scale =
        [](void *data, wl_surface *wldata, int32_t factor) {
            auto *obj = static_cast<WlSurface *>(data);
            assert(*obj == wldata);
            {
                obj->preferredBufferScale()(factor);
            }
        },
    .preferred_buffer_transform =
        [](void *data, wl_surface *wldata, uint32_t transform) {
            auto *obj = static_cast<WlSurface *>(data);
            assert(*obj == wldata);
            {
                obj->preferredBufferTransform()(transform);
            }
        },
};

WlSurface::WlSurface(wl_surface *data)
    : version_(wl_surface_get_version(data)), data_(data) {
    wl_surface_set_user_data(*this, this);
    wl_surface_add_listener(*this, &WlSurface::listener, this);
}

void WlSurface::destructor(wl_surface *data) {
    const auto version = wl_surface_get_version(data);
    if (version >= 1) {
        wl_surface_destroy(data);
        return;
    }
}
void WlSurface::attach(WlBuffer *buffer, int32_t x, int32_t y) {
    wl_surface_attach(*this, rawPointer(buffer), x, y);
}
void WlSurface::damage(int32_t x, int32_t y, int32_t width, int32_t height) {
    wl_surface_damage(*this, x, y, width, height);
}
WlCallback *WlSurface::frame() {
    return new WlCallback(wl_surface_frame(*this));
}
void WlSurface::setOpaqueRegion(WlRegion *region) {
    wl_surface_set_opaque_region(*this, rawPointer(region));
}
void WlSurface::setInputRegion(WlRegion *region) {
    wl_surface_set_input_region(*this, rawPointer(region));
}
void WlSurface::commit() { wl_surface_commit(*this); }
void WlSurface::setBufferTransform(int32_t transform) {
    wl_surface_set_buffer_transform(*this, transform);
}
void WlSurface::setBufferScale(int32_t scale) {
    wl_surface_set_buffer_scale(*this, scale);
}
void WlSurface::damageBuffer(int32_t x, int32_t y, int32_t width,
                             int32_t height) {
    wl_surface_damage_buffer(*this, x, y, width, height);
}
void WlSurface::offset(int32_t x, int32_t y) { wl_surface_offset(*this, x, y); }
} // namespace fcitx::wayland
