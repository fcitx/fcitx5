#include "zwp_tablet_tool_v2.h"
#include <cassert>
#include "wayland-tablet-unstable-v2-client-protocol.h"
#include "wl_surface.h"
#include "zwp_tablet_v2.h"

namespace fcitx::wayland {
const struct zwp_tablet_tool_v2_listener ZwpTabletToolV2::listener = {
    .type =
        [](void *data, zwp_tablet_tool_v2 *wldata, uint32_t toolType) {
            auto *obj = static_cast<ZwpTabletToolV2 *>(data);
            assert(*obj == wldata);
            {
                obj->type()(toolType);
            }
        },
    .hardware_serial =
        [](void *data, zwp_tablet_tool_v2 *wldata, uint32_t hardwareSerialHi,
           uint32_t hardwareSerialLo) {
            auto *obj = static_cast<ZwpTabletToolV2 *>(data);
            assert(*obj == wldata);
            {
                obj->hardwareSerial()(hardwareSerialHi, hardwareSerialLo);
            }
        },
    .hardware_id_wacom =
        [](void *data, zwp_tablet_tool_v2 *wldata, uint32_t hardwareIdHi,
           uint32_t hardwareIdLo) {
            auto *obj = static_cast<ZwpTabletToolV2 *>(data);
            assert(*obj == wldata);
            {
                obj->hardwareIdWacom()(hardwareIdHi, hardwareIdLo);
            }
        },
    .capability =
        [](void *data, zwp_tablet_tool_v2 *wldata, uint32_t capability) {
            auto *obj = static_cast<ZwpTabletToolV2 *>(data);
            assert(*obj == wldata);
            {
                obj->capability()(capability);
            }
        },
    .done =
        [](void *data, zwp_tablet_tool_v2 *wldata) {
            auto *obj = static_cast<ZwpTabletToolV2 *>(data);
            assert(*obj == wldata);
            {
                obj->done()();
            }
        },
    .removed =
        [](void *data, zwp_tablet_tool_v2 *wldata) {
            auto *obj = static_cast<ZwpTabletToolV2 *>(data);
            assert(*obj == wldata);
            {
                obj->removed()();
            }
        },
    .proximity_in =
        [](void *data, zwp_tablet_tool_v2 *wldata, uint32_t serial,
           zwp_tablet_v2 *tablet, wl_surface *surface) {
            auto *obj = static_cast<ZwpTabletToolV2 *>(data);
            assert(*obj == wldata);
            {
                if (!tablet) {
                    return;
                }
                auto *tablet_ = static_cast<ZwpTabletV2 *>(
                    zwp_tablet_v2_get_user_data(tablet));
                if (!surface) {
                    return;
                }
                auto *surface_ =
                    static_cast<WlSurface *>(wl_surface_get_user_data(surface));
                obj->proximityIn()(serial, tablet_, surface_);
            }
        },
    .proximity_out =
        [](void *data, zwp_tablet_tool_v2 *wldata) {
            auto *obj = static_cast<ZwpTabletToolV2 *>(data);
            assert(*obj == wldata);
            {
                obj->proximityOut()();
            }
        },
    .down =
        [](void *data, zwp_tablet_tool_v2 *wldata, uint32_t serial) {
            auto *obj = static_cast<ZwpTabletToolV2 *>(data);
            assert(*obj == wldata);
            {
                obj->down()(serial);
            }
        },
    .up =
        [](void *data, zwp_tablet_tool_v2 *wldata) {
            auto *obj = static_cast<ZwpTabletToolV2 *>(data);
            assert(*obj == wldata);
            {
                obj->up()();
            }
        },
    .motion =
        [](void *data, zwp_tablet_tool_v2 *wldata, wl_fixed_t x, wl_fixed_t y) {
            auto *obj = static_cast<ZwpTabletToolV2 *>(data);
            assert(*obj == wldata);
            {
                obj->motion()(x, y);
            }
        },
    .pressure =
        [](void *data, zwp_tablet_tool_v2 *wldata, uint32_t pressure) {
            auto *obj = static_cast<ZwpTabletToolV2 *>(data);
            assert(*obj == wldata);
            {
                obj->pressure()(pressure);
            }
        },
    .distance =
        [](void *data, zwp_tablet_tool_v2 *wldata, uint32_t distance) {
            auto *obj = static_cast<ZwpTabletToolV2 *>(data);
            assert(*obj == wldata);
            {
                obj->distance()(distance);
            }
        },
    .tilt =
        [](void *data, zwp_tablet_tool_v2 *wldata, wl_fixed_t tiltX,
           wl_fixed_t tiltY) {
            auto *obj = static_cast<ZwpTabletToolV2 *>(data);
            assert(*obj == wldata);
            {
                obj->tilt()(tiltX, tiltY);
            }
        },
    .rotation =
        [](void *data, zwp_tablet_tool_v2 *wldata, wl_fixed_t degrees) {
            auto *obj = static_cast<ZwpTabletToolV2 *>(data);
            assert(*obj == wldata);
            {
                obj->rotation()(degrees);
            }
        },
    .slider =
        [](void *data, zwp_tablet_tool_v2 *wldata, int32_t position) {
            auto *obj = static_cast<ZwpTabletToolV2 *>(data);
            assert(*obj == wldata);
            {
                obj->slider()(position);
            }
        },
    .wheel =
        [](void *data, zwp_tablet_tool_v2 *wldata, wl_fixed_t degrees,
           int32_t clicks) {
            auto *obj = static_cast<ZwpTabletToolV2 *>(data);
            assert(*obj == wldata);
            {
                obj->wheel()(degrees, clicks);
            }
        },
    .button =
        [](void *data, zwp_tablet_tool_v2 *wldata, uint32_t serial,
           uint32_t button, uint32_t state) {
            auto *obj = static_cast<ZwpTabletToolV2 *>(data);
            assert(*obj == wldata);
            {
                obj->button()(serial, button, state);
            }
        },
    .frame =
        [](void *data, zwp_tablet_tool_v2 *wldata, uint32_t time) {
            auto *obj = static_cast<ZwpTabletToolV2 *>(data);
            assert(*obj == wldata);
            {
                obj->frame()(time);
            }
        },
};

ZwpTabletToolV2::ZwpTabletToolV2(zwp_tablet_tool_v2 *data)
    : version_(zwp_tablet_tool_v2_get_version(data)), data_(data) {
    zwp_tablet_tool_v2_set_user_data(*this, this);
    zwp_tablet_tool_v2_add_listener(*this, &ZwpTabletToolV2::listener, this);
}

void ZwpTabletToolV2::destructor(zwp_tablet_tool_v2 *data) {
    const auto version = zwp_tablet_tool_v2_get_version(data);
    if (version >= 1) {
        zwp_tablet_tool_v2_destroy(data);
        return;
    }
}
void ZwpTabletToolV2::setCursor(uint32_t serial, WlSurface *surface,
                                int32_t hotspotX, int32_t hotspotY) {
    zwp_tablet_tool_v2_set_cursor(*this, serial, rawPointer(surface), hotspotX,
                                  hotspotY);
}

} // namespace fcitx::wayland
