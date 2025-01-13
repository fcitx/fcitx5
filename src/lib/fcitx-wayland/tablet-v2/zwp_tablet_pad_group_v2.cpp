#include "zwp_tablet_pad_group_v2.h"
#include <cassert>
#include "zwp_tablet_pad_ring_v2.h"
#include "zwp_tablet_pad_strip_v2.h"
namespace fcitx::wayland {
const struct zwp_tablet_pad_group_v2_listener ZwpTabletPadGroupV2::listener = {
    [](void *data, zwp_tablet_pad_group_v2 *wldata, wl_array *buttons) {
        auto *obj = static_cast<ZwpTabletPadGroupV2 *>(data);
        assert(*obj == wldata);
        {
            return obj->buttons()(buttons);
        }
    },
    [](void *data, zwp_tablet_pad_group_v2 *wldata,
       zwp_tablet_pad_ring_v2 *ring) {
        auto *obj = static_cast<ZwpTabletPadGroupV2 *>(data);
        assert(*obj == wldata);
        {
            auto *ring_ = new ZwpTabletPadRingV2(ring);
            return obj->ring()(ring_);
        }
    },
    [](void *data, zwp_tablet_pad_group_v2 *wldata,
       zwp_tablet_pad_strip_v2 *strip) {
        auto *obj = static_cast<ZwpTabletPadGroupV2 *>(data);
        assert(*obj == wldata);
        {
            auto *strip_ = new ZwpTabletPadStripV2(strip);
            return obj->strip()(strip_);
        }
    },
    [](void *data, zwp_tablet_pad_group_v2 *wldata, uint32_t modes) {
        auto *obj = static_cast<ZwpTabletPadGroupV2 *>(data);
        assert(*obj == wldata);
        {
            return obj->modes()(modes);
        }
    },
    [](void *data, zwp_tablet_pad_group_v2 *wldata) {
        auto *obj = static_cast<ZwpTabletPadGroupV2 *>(data);
        assert(*obj == wldata);
        {
            return obj->done()();
        }
    },
    [](void *data, zwp_tablet_pad_group_v2 *wldata, uint32_t time,
       uint32_t serial, uint32_t mode) {
        auto *obj = static_cast<ZwpTabletPadGroupV2 *>(data);
        assert(*obj == wldata);
        {
            return obj->modeSwitch()(time, serial, mode);
        }
    },
};
ZwpTabletPadGroupV2::ZwpTabletPadGroupV2(zwp_tablet_pad_group_v2 *data)
    : version_(zwp_tablet_pad_group_v2_get_version(data)), data_(data) {
    zwp_tablet_pad_group_v2_set_user_data(*this, this);
    zwp_tablet_pad_group_v2_add_listener(*this, &ZwpTabletPadGroupV2::listener,
                                         this);
}
void ZwpTabletPadGroupV2::destructor(zwp_tablet_pad_group_v2 *data) {
    auto version = zwp_tablet_pad_group_v2_get_version(data);
    if (version >= 1) {
        return zwp_tablet_pad_group_v2_destroy(data);
    }
}
} // namespace fcitx::wayland
