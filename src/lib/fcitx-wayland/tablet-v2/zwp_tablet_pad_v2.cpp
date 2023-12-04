#include "zwp_tablet_pad_v2.h"
#include <cassert>
#include "wl_surface.h"
#include "zwp_tablet_pad_group_v2.h"
#include "zwp_tablet_v2.h"
namespace fcitx::wayland {
const struct zwp_tablet_pad_v2_listener ZwpTabletPadV2::listener = {
    [](void *data, zwp_tablet_pad_v2 *wldata,
       zwp_tablet_pad_group_v2 *padGroup) {
        auto *obj = static_cast<ZwpTabletPadV2 *>(data);
        assert(*obj == wldata);
        {
            auto *padGroup_ = new ZwpTabletPadGroupV2(padGroup);
            return obj->group()(padGroup_);
        }
    },
    [](void *data, zwp_tablet_pad_v2 *wldata, const char *path) {
        auto *obj = static_cast<ZwpTabletPadV2 *>(data);
        assert(*obj == wldata);
        { return obj->path()(path); }
    },
    [](void *data, zwp_tablet_pad_v2 *wldata, uint32_t buttons) {
        auto *obj = static_cast<ZwpTabletPadV2 *>(data);
        assert(*obj == wldata);
        { return obj->buttons()(buttons); }
    },
    [](void *data, zwp_tablet_pad_v2 *wldata) {
        auto *obj = static_cast<ZwpTabletPadV2 *>(data);
        assert(*obj == wldata);
        { return obj->done()(); }
    },
    [](void *data, zwp_tablet_pad_v2 *wldata, uint32_t time, uint32_t button,
       uint32_t state) {
        auto *obj = static_cast<ZwpTabletPadV2 *>(data);
        assert(*obj == wldata);
        { return obj->button()(time, button, state); }
    },
    [](void *data, zwp_tablet_pad_v2 *wldata, uint32_t serial,
       zwp_tablet_v2 *tablet, wl_surface *surface) {
        auto *obj = static_cast<ZwpTabletPadV2 *>(data);
        assert(*obj == wldata);
        {
            if (!tablet) {
                return;
            }
            auto *tablet_ =
                static_cast<ZwpTabletV2 *>(zwp_tablet_v2_get_user_data(tablet));
            if (!surface) {
                return;
            }
            auto *surface_ =
                static_cast<WlSurface *>(wl_surface_get_user_data(surface));
            return obj->enter()(serial, tablet_, surface_);
        }
    },
    [](void *data, zwp_tablet_pad_v2 *wldata, uint32_t serial,
       wl_surface *surface) {
        auto *obj = static_cast<ZwpTabletPadV2 *>(data);
        assert(*obj == wldata);
        {
            if (!surface) {
                return;
            }
            auto *surface_ =
                static_cast<WlSurface *>(wl_surface_get_user_data(surface));
            return obj->leave()(serial, surface_);
        }
    },
    [](void *data, zwp_tablet_pad_v2 *wldata) {
        auto *obj = static_cast<ZwpTabletPadV2 *>(data);
        assert(*obj == wldata);
        { return obj->removed()(); }
    },
};
ZwpTabletPadV2::ZwpTabletPadV2(zwp_tablet_pad_v2 *data)
    : version_(zwp_tablet_pad_v2_get_version(data)), data_(data) {
    zwp_tablet_pad_v2_set_user_data(*this, this);
    zwp_tablet_pad_v2_add_listener(*this, &ZwpTabletPadV2::listener, this);
}
void ZwpTabletPadV2::destructor(zwp_tablet_pad_v2 *data) {
    auto version = zwp_tablet_pad_v2_get_version(data);
    if (version >= 1) {
        return zwp_tablet_pad_v2_destroy(data);
    }
}
void ZwpTabletPadV2::setFeedback(uint32_t button, const char *description,
                                 uint32_t serial) {
    return zwp_tablet_pad_v2_set_feedback(*this, button, description, serial);
}
} // namespace fcitx::wayland
