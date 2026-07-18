#include "wl_surface.h"
#include <cassert>
#include "wl_buffer.h"
#include "wl_callback.h"
#include "wl_output.h"
#include "wl_region.h"

namespace fcitx::wayland {
const struct wl_surface_listener WlSurface::listener = {
#if defined(WL_SURFACE_ENTER_SINCE_VERSION)
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
#endif
#if defined(WL_SURFACE_LEAVE_SINCE_VERSION)
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
#endif
#if defined(WL_SURFACE_PREFERRED_BUFFER_SCALE_SINCE_VERSION)
    .preferred_buffer_scale =
        [](void *data, wl_surface *wldata, int32_t factor) {
            auto *obj = static_cast<WlSurface *>(data);
            assert(*obj == wldata);
            {
                obj->preferredBufferScale()(factor);
            }
        },
#endif
#if defined(WL_SURFACE_PREFERRED_BUFFER_TRANSFORM_SINCE_VERSION)
    .preferred_buffer_transform =
        [](void *data, wl_surface *wldata, uint32_t transform) {
            auto *obj = static_cast<WlSurface *>(data);
            assert(*obj == wldata);
            {
                obj->preferredBufferTransform()(transform);
            }
        },
#endif
};

WlSurface::WlSurface(wl_surface *data)
    : version_(wl_surface_get_version(data)), data_(data) {
    wl_surface_set_user_data(*this, this);
    wl_surface_add_listener(*this, &WlSurface::listener, this);
}

void WlSurface::destructor(wl_surface *data) { wl_surface_destroy(data); }
#if defined(WL_SURFACE_ATTACH_SINCE_VERSION)
void WlSurface::attach(WlBuffer *buffer, int32_t x, int32_t y) {
    wl_surface_attach(*this, rawPointer(buffer), x, y);
}
#endif
#if defined(WL_SURFACE_DAMAGE_SINCE_VERSION)
void WlSurface::damage(int32_t x, int32_t y, int32_t width, int32_t height) {
    wl_surface_damage(*this, x, y, width, height);
}
#endif
#if defined(WL_SURFACE_FRAME_SINCE_VERSION)
WlCallback *WlSurface::frame() {
    return new WlCallback(wl_surface_frame(*this));
}
#endif
#if defined(WL_SURFACE_SET_OPAQUE_REGION_SINCE_VERSION)
void WlSurface::setOpaqueRegion(WlRegion *region) {
    wl_surface_set_opaque_region(*this, rawPointer(region));
}
#endif
#if defined(WL_SURFACE_SET_INPUT_REGION_SINCE_VERSION)
void WlSurface::setInputRegion(WlRegion *region) {
    wl_surface_set_input_region(*this, rawPointer(region));
}
#endif
#if defined(WL_SURFACE_COMMIT_SINCE_VERSION)
void WlSurface::commit() { wl_surface_commit(*this); }
#endif
#if defined(WL_SURFACE_SET_BUFFER_TRANSFORM_SINCE_VERSION)
void WlSurface::setBufferTransform(int32_t transform) {
    wl_surface_set_buffer_transform(*this, transform);
}
#endif
#if defined(WL_SURFACE_SET_BUFFER_SCALE_SINCE_VERSION)
void WlSurface::setBufferScale(int32_t scale) {
    wl_surface_set_buffer_scale(*this, scale);
}
#endif
#if defined(WL_SURFACE_DAMAGE_BUFFER_SINCE_VERSION)
void WlSurface::damageBuffer(int32_t x, int32_t y, int32_t width,
                             int32_t height) {
    wl_surface_damage_buffer(*this, x, y, width, height);
}
#endif
#if defined(WL_SURFACE_OFFSET_SINCE_VERSION)
void WlSurface::offset(int32_t x, int32_t y) { wl_surface_offset(*this, x, y); }
#endif
#if defined(WL_SURFACE_GET_RELEASE_SINCE_VERSION)
WlCallback *WlSurface::getRelease() {
    return new WlCallback(wl_surface_get_release(*this));
}
#endif

} // namespace fcitx::wayland
