#include "zwp_tablet_pad_strip_v2.h"
#include <cassert>
namespace fcitx::wayland {
const struct zwp_tablet_pad_strip_v2_listener ZwpTabletPadStripV2::listener = {
    [](void *data, zwp_tablet_pad_strip_v2 *wldata, uint32_t source) {
        auto *obj = static_cast<ZwpTabletPadStripV2 *>(data);
        assert(*obj == wldata);
        { return obj->source()(source); }
    },
    [](void *data, zwp_tablet_pad_strip_v2 *wldata, uint32_t position) {
        auto *obj = static_cast<ZwpTabletPadStripV2 *>(data);
        assert(*obj == wldata);
        { return obj->position()(position); }
    },
    [](void *data, zwp_tablet_pad_strip_v2 *wldata) {
        auto *obj = static_cast<ZwpTabletPadStripV2 *>(data);
        assert(*obj == wldata);
        { return obj->stop()(); }
    },
    [](void *data, zwp_tablet_pad_strip_v2 *wldata, uint32_t time) {
        auto *obj = static_cast<ZwpTabletPadStripV2 *>(data);
        assert(*obj == wldata);
        { return obj->frame()(time); }
    },
};
ZwpTabletPadStripV2::ZwpTabletPadStripV2(zwp_tablet_pad_strip_v2 *data)
    : version_(zwp_tablet_pad_strip_v2_get_version(data)), data_(data) {
    zwp_tablet_pad_strip_v2_set_user_data(*this, this);
    zwp_tablet_pad_strip_v2_add_listener(*this, &ZwpTabletPadStripV2::listener,
                                         this);
}
void ZwpTabletPadStripV2::destructor(zwp_tablet_pad_strip_v2 *data) {
    auto version = zwp_tablet_pad_strip_v2_get_version(data);
    if (version >= 1) {
        return zwp_tablet_pad_strip_v2_destroy(data);
    }
}
void ZwpTabletPadStripV2::setFeedback(const char *description,
                                      uint32_t serial) {
    return zwp_tablet_pad_strip_v2_set_feedback(*this, description, serial);
}
} // namespace fcitx::wayland
