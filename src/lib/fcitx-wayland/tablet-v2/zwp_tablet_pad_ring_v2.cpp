#include "zwp_tablet_pad_ring_v2.h"
#include <cassert>
namespace fcitx::wayland {
const struct zwp_tablet_pad_ring_v2_listener ZwpTabletPadRingV2::listener = {
    [](void *data, zwp_tablet_pad_ring_v2 *wldata, uint32_t source) {
        auto *obj = static_cast<ZwpTabletPadRingV2 *>(data);
        assert(*obj == wldata);
        { return obj->source()(source); }
    },
    [](void *data, zwp_tablet_pad_ring_v2 *wldata, wl_fixed_t degrees) {
        auto *obj = static_cast<ZwpTabletPadRingV2 *>(data);
        assert(*obj == wldata);
        { return obj->angle()(degrees); }
    },
    [](void *data, zwp_tablet_pad_ring_v2 *wldata) {
        auto *obj = static_cast<ZwpTabletPadRingV2 *>(data);
        assert(*obj == wldata);
        { return obj->stop()(); }
    },
    [](void *data, zwp_tablet_pad_ring_v2 *wldata, uint32_t time) {
        auto *obj = static_cast<ZwpTabletPadRingV2 *>(data);
        assert(*obj == wldata);
        { return obj->frame()(time); }
    },
};
ZwpTabletPadRingV2::ZwpTabletPadRingV2(zwp_tablet_pad_ring_v2 *data)
    : version_(zwp_tablet_pad_ring_v2_get_version(data)), data_(data) {
    zwp_tablet_pad_ring_v2_set_user_data(*this, this);
    zwp_tablet_pad_ring_v2_add_listener(*this, &ZwpTabletPadRingV2::listener,
                                        this);
}
void ZwpTabletPadRingV2::destructor(zwp_tablet_pad_ring_v2 *data) {
    auto version = zwp_tablet_pad_ring_v2_get_version(data);
    if (version >= 1) {
        return zwp_tablet_pad_ring_v2_destroy(data);
    }
}
void ZwpTabletPadRingV2::setFeedback(const char *description, uint32_t serial) {
    return zwp_tablet_pad_ring_v2_set_feedback(*this, description, serial);
}
} // namespace fcitx::wayland
