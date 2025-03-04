#include "zwp_tablet_seat_v2.h"
#include <cassert>
#include "wayland-tablet-unstable-v2-client-protocol.h"
#include "zwp_tablet_pad_v2.h"
#include "zwp_tablet_tool_v2.h"
#include "zwp_tablet_v2.h"

namespace fcitx::wayland {
const struct zwp_tablet_seat_v2_listener ZwpTabletSeatV2::listener = {
    .tablet_added =
        [](void *data, zwp_tablet_seat_v2 *wldata, zwp_tablet_v2 *id) {
            auto *obj = static_cast<ZwpTabletSeatV2 *>(data);
            assert(*obj == wldata);
            {
                auto *id_ = new ZwpTabletV2(id);
                obj->tabletAdded()(id_);
            }
        },
    .tool_added =
        [](void *data, zwp_tablet_seat_v2 *wldata, zwp_tablet_tool_v2 *id) {
            auto *obj = static_cast<ZwpTabletSeatV2 *>(data);
            assert(*obj == wldata);
            {
                auto *id_ = new ZwpTabletToolV2(id);
                obj->toolAdded()(id_);
            }
        },
    .pad_added =
        [](void *data, zwp_tablet_seat_v2 *wldata, zwp_tablet_pad_v2 *id) {
            auto *obj = static_cast<ZwpTabletSeatV2 *>(data);
            assert(*obj == wldata);
            {
                auto *id_ = new ZwpTabletPadV2(id);
                obj->padAdded()(id_);
            }
        },
};

ZwpTabletSeatV2::ZwpTabletSeatV2(zwp_tablet_seat_v2 *data)
    : version_(zwp_tablet_seat_v2_get_version(data)), data_(data) {
    zwp_tablet_seat_v2_set_user_data(*this, this);
    zwp_tablet_seat_v2_add_listener(*this, &ZwpTabletSeatV2::listener, this);
}

void ZwpTabletSeatV2::destructor(zwp_tablet_seat_v2 *data) {
    const auto version = zwp_tablet_seat_v2_get_version(data);
    if (version >= 1) {
        zwp_tablet_seat_v2_destroy(data);
        return;
    }
}

} // namespace fcitx::wayland
