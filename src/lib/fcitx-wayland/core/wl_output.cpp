#include "wl_output.h"
#include <cassert>
namespace fcitx::wayland {
const struct wl_output_listener WlOutput::listener = {
    [](void *data, wl_output *wldata, int32_t x, int32_t y,
       int32_t physicalWidth, int32_t physicalHeight, int32_t subpixel,
       const char *make, const char *model, int32_t transform) {
        auto *obj = static_cast<WlOutput *>(data);
        assert(*obj == wldata);
        {
            return obj->geometry()(x, y, physicalWidth, physicalHeight,
                                   subpixel, make, model, transform);
        }
    },
    [](void *data, wl_output *wldata, uint32_t flags, int32_t width,
       int32_t height, int32_t refresh) {
        auto *obj = static_cast<WlOutput *>(data);
        assert(*obj == wldata);
        { return obj->mode()(flags, width, height, refresh); }
    },
    [](void *data, wl_output *wldata) {
        auto *obj = static_cast<WlOutput *>(data);
        assert(*obj == wldata);
        { return obj->done()(); }
    },
    [](void *data, wl_output *wldata, int32_t factor) {
        auto *obj = static_cast<WlOutput *>(data);
        assert(*obj == wldata);
        { return obj->scale()(factor); }
    },
};
WlOutput::WlOutput(wl_output *data)
    : version_(wl_output_get_version(data)), data_(data) {
    wl_output_set_user_data(*this, this);
    wl_output_add_listener(*this, &WlOutput::listener, this);
}
void WlOutput::destructor(wl_output *data) {
    auto version = wl_output_get_version(data);
    if (version >= 3) {
        return wl_output_release(data);
    } else {
        return wl_output_destroy(data);
    }
}
} // namespace fcitx::wayland
