#include "wl_output.h"
#include <cassert>

namespace fcitx::wayland {
const struct wl_output_listener WlOutput::listener = {
#if defined(WL_OUTPUT_GEOMETRY_SINCE_VERSION)
    .geometry =
        [](void *data, wl_output *wldata, int32_t x, int32_t y,
           int32_t physicalWidth, int32_t physicalHeight, int32_t subpixel,
           const char *make, const char *model, int32_t transform) {
            auto *obj = static_cast<WlOutput *>(data);
            assert(*obj == wldata);
            {
                obj->geometry()(x, y, physicalWidth, physicalHeight, subpixel,
                                make, model, transform);
            }
        },
#endif
#if defined(WL_OUTPUT_MODE_SINCE_VERSION)
    .mode =
        [](void *data, wl_output *wldata, uint32_t flags, int32_t width,
           int32_t height, int32_t refresh) {
            auto *obj = static_cast<WlOutput *>(data);
            assert(*obj == wldata);
            {
                obj->mode()(flags, width, height, refresh);
            }
        },
#endif
#if defined(WL_OUTPUT_DONE_SINCE_VERSION)
    .done =
        [](void *data, wl_output *wldata) {
            auto *obj = static_cast<WlOutput *>(data);
            assert(*obj == wldata);
            {
                obj->done()();
            }
        },
#endif
#if defined(WL_OUTPUT_SCALE_SINCE_VERSION)
    .scale =
        [](void *data, wl_output *wldata, int32_t factor) {
            auto *obj = static_cast<WlOutput *>(data);
            assert(*obj == wldata);
            {
                obj->scale()(factor);
            }
        },
#endif
#if defined(WL_OUTPUT_NAME_SINCE_VERSION)
    .name =
        [](void *data, wl_output *wldata, const char *name) {
            auto *obj = static_cast<WlOutput *>(data);
            assert(*obj == wldata);
            {
                obj->name()(name);
            }
        },
#endif
#if defined(WL_OUTPUT_DESCRIPTION_SINCE_VERSION)
    .description =
        [](void *data, wl_output *wldata, const char *description) {
            auto *obj = static_cast<WlOutput *>(data);
            assert(*obj == wldata);
            {
                obj->description()(description);
            }
        },
#endif
};

WlOutput::WlOutput(wl_output *data)
    : version_(wl_output_get_version(data)), data_(data) {
    wl_output_set_user_data(*this, this);
    wl_output_add_listener(*this, &WlOutput::listener, this);
}

void WlOutput::destructor(wl_output *data) {
    const auto version = wl_output_get_version(data);
#if defined(WL_OUTPUT_RELEASE_SINCE_VERSION)
    if (version >= 3) {
        wl_output_release(data);
        return;
    }
#endif
    wl_output_destroy(data);
}

} // namespace fcitx::wayland
