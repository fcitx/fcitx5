#include "wl_output.h"
#include <cassert>

namespace fcitx::wayland {
const struct wl_output_listener WlOutput::listener = {
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
    .mode =
        [](void *data, wl_output *wldata, uint32_t flags, int32_t width,
           int32_t height, int32_t refresh) {
            auto *obj = static_cast<WlOutput *>(data);
            assert(*obj == wldata);
            {
                obj->mode()(flags, width, height, refresh);
            }
        },
    .done =
        [](void *data, wl_output *wldata) {
            auto *obj = static_cast<WlOutput *>(data);
            assert(*obj == wldata);
            {
                obj->done()();
            }
        },
    .scale =
        [](void *data, wl_output *wldata, int32_t factor) {
            auto *obj = static_cast<WlOutput *>(data);
            assert(*obj == wldata);
            {
                obj->scale()(factor);
            }
        },
    .name =
        [](void *data, wl_output *wldata, const char *name) {
            auto *obj = static_cast<WlOutput *>(data);
            assert(*obj == wldata);
            {
                obj->name()(name);
            }
        },
    .description =
        [](void *data, wl_output *wldata, const char *description) {
            auto *obj = static_cast<WlOutput *>(data);
            assert(*obj == wldata);
            {
                obj->description()(description);
            }
        },
};

WlOutput::WlOutput(wl_output *data)
    : version_(wl_output_get_version(data)), data_(data) {
    wl_output_set_user_data(*this, this);
    wl_output_add_listener(*this, &WlOutput::listener, this);
}

void WlOutput::destructor(wl_output *data) {
    const auto version = wl_output_get_version(data);
    if (version >= 3) {
        wl_output_release(data);
        return;
    }
    wl_output_destroy(data);
}
} // namespace fcitx::wayland
