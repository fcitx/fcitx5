#include "zwp_tablet_pad_group_v2.h"
#include <cassert>
#include "wayland-tablet-unstable-v2-client-protocol.h"
#include "zwp_tablet_pad_ring_v2.h"
#include "zwp_tablet_pad_strip_v2.h"

namespace fcitx::wayland {
const struct zwp_tablet_pad_group_v2_listener ZwpTabletPadGroupV2::listener = {
    .buttons =
        [](void *data, zwp_tablet_pad_group_v2 *wldata, wl_array *buttons) {
            auto *obj = static_cast<ZwpTabletPadGroupV2 *>(data);
            assert(*obj == wldata);
            {
                obj->buttons()(buttons);
            }
        },
    .ring =
        [](void *data, zwp_tablet_pad_group_v2 *wldata,
           zwp_tablet_pad_ring_v2 *ring) {
            auto *obj = static_cast<ZwpTabletPadGroupV2 *>(data);
            assert(*obj == wldata);
            {
                auto *ring_ = new ZwpTabletPadRingV2(ring);
                obj->ring()(ring_);
            }
        },
    .strip =
        [](void *data, zwp_tablet_pad_group_v2 *wldata,
           zwp_tablet_pad_strip_v2 *strip) {
            auto *obj = static_cast<ZwpTabletPadGroupV2 *>(data);
            assert(*obj == wldata);
            {
                auto *strip_ = new ZwpTabletPadStripV2(strip);
                obj->strip()(strip_);
            }
        },
    .modes =
        [](void *data, zwp_tablet_pad_group_v2 *wldata, uint32_t modes) {
            auto *obj = static_cast<ZwpTabletPadGroupV2 *>(data);
            assert(*obj == wldata);
            {
                obj->modes()(modes);
            }
        },
    .done =
        [](void *data, zwp_tablet_pad_group_v2 *wldata) {
            auto *obj = static_cast<ZwpTabletPadGroupV2 *>(data);
            assert(*obj == wldata);
            {
                obj->done()();
            }
        },
    .mode_switch =
        [](void *data, zwp_tablet_pad_group_v2 *wldata, uint32_t time,
           uint32_t serial, uint32_t mode) {
            auto *obj = static_cast<ZwpTabletPadGroupV2 *>(data);
            assert(*obj == wldata);
            {
                obj->modeSwitch()(time, serial, mode);
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
    const auto version = zwp_tablet_pad_group_v2_get_version(data);
    if (version >= 1) {
        zwp_tablet_pad_group_v2_destroy(data);
        return;
    }
}

} // namespace fcitx::wayland
