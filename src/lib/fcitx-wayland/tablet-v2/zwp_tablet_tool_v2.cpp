#include "zwp_tablet_tool_v2.h"
#include <cassert>
#include "wl_surface.h"
#include "zwp_tablet_v2.h"
namespace fcitx::wayland {
const struct zwp_tablet_tool_v2_listener ZwpTabletToolV2::listener = {
    [](void *data, zwp_tablet_tool_v2 *wldata, uint32_t toolType) {
        auto *obj = static_cast<ZwpTabletToolV2 *>(data);
        assert(*obj == wldata);
        {
            return obj->type()(toolType);
        }
    },
    [](void *data, zwp_tablet_tool_v2 *wldata, uint32_t hardwareSerialHi,
       uint32_t hardwareSerialLo) {
        auto *obj = static_cast<ZwpTabletToolV2 *>(data);
        assert(*obj == wldata);
        {
            return obj->hardwareSerial()(hardwareSerialHi, hardwareSerialLo);
        }
    },
    [](void *data, zwp_tablet_tool_v2 *wldata, uint32_t hardwareIdHi,
       uint32_t hardwareIdLo) {
        auto *obj = static_cast<ZwpTabletToolV2 *>(data);
        assert(*obj == wldata);
        {
            return obj->hardwareIdWacom()(hardwareIdHi, hardwareIdLo);
        }
    },
    [](void *data, zwp_tablet_tool_v2 *wldata, uint32_t capability) {
        auto *obj = static_cast<ZwpTabletToolV2 *>(data);
        assert(*obj == wldata);
        {
            return obj->capability()(capability);
        }
    },
    [](void *data, zwp_tablet_tool_v2 *wldata) {
        auto *obj = static_cast<ZwpTabletToolV2 *>(data);
        assert(*obj == wldata);
        {
            return obj->done()();
        }
    },
    [](void *data, zwp_tablet_tool_v2 *wldata) {
        auto *obj = static_cast<ZwpTabletToolV2 *>(data);
        assert(*obj == wldata);
        {
            return obj->removed()();
        }
    },
    [](void *data, zwp_tablet_tool_v2 *wldata, uint32_t serial,
       zwp_tablet_v2 *tablet, wl_surface *surface) {
        auto *obj = static_cast<ZwpTabletToolV2 *>(data);
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
            return obj->proximityIn()(serial, tablet_, surface_);
        }
    },
    [](void *data, zwp_tablet_tool_v2 *wldata) {
        auto *obj = static_cast<ZwpTabletToolV2 *>(data);
        assert(*obj == wldata);
        {
            return obj->proximityOut()();
        }
    },
    [](void *data, zwp_tablet_tool_v2 *wldata, uint32_t serial) {
        auto *obj = static_cast<ZwpTabletToolV2 *>(data);
        assert(*obj == wldata);
        {
            return obj->down()(serial);
        }
    },
    [](void *data, zwp_tablet_tool_v2 *wldata) {
        auto *obj = static_cast<ZwpTabletToolV2 *>(data);
        assert(*obj == wldata);
        {
            return obj->up()();
        }
    },
    [](void *data, zwp_tablet_tool_v2 *wldata, wl_fixed_t x, wl_fixed_t y) {
        auto *obj = static_cast<ZwpTabletToolV2 *>(data);
        assert(*obj == wldata);
        {
            return obj->motion()(x, y);
        }
    },
    [](void *data, zwp_tablet_tool_v2 *wldata, uint32_t pressure) {
        auto *obj = static_cast<ZwpTabletToolV2 *>(data);
        assert(*obj == wldata);
        {
            return obj->pressure()(pressure);
        }
    },
    [](void *data, zwp_tablet_tool_v2 *wldata, uint32_t distance) {
        auto *obj = static_cast<ZwpTabletToolV2 *>(data);
        assert(*obj == wldata);
        {
            return obj->distance()(distance);
        }
    },
    [](void *data, zwp_tablet_tool_v2 *wldata, wl_fixed_t tiltX,
       wl_fixed_t tiltY) {
        auto *obj = static_cast<ZwpTabletToolV2 *>(data);
        assert(*obj == wldata);
        {
            return obj->tilt()(tiltX, tiltY);
        }
    },
    [](void *data, zwp_tablet_tool_v2 *wldata, wl_fixed_t degrees) {
        auto *obj = static_cast<ZwpTabletToolV2 *>(data);
        assert(*obj == wldata);
        {
            return obj->rotation()(degrees);
        }
    },
    [](void *data, zwp_tablet_tool_v2 *wldata, int32_t position) {
        auto *obj = static_cast<ZwpTabletToolV2 *>(data);
        assert(*obj == wldata);
        {
            return obj->slider()(position);
        }
    },
    [](void *data, zwp_tablet_tool_v2 *wldata, wl_fixed_t degrees,
       int32_t clicks) {
        auto *obj = static_cast<ZwpTabletToolV2 *>(data);
        assert(*obj == wldata);
        {
            return obj->wheel()(degrees, clicks);
        }
    },
    [](void *data, zwp_tablet_tool_v2 *wldata, uint32_t serial, uint32_t button,
       uint32_t state) {
        auto *obj = static_cast<ZwpTabletToolV2 *>(data);
        assert(*obj == wldata);
        {
            return obj->button()(serial, button, state);
        }
    },
    [](void *data, zwp_tablet_tool_v2 *wldata, uint32_t time) {
        auto *obj = static_cast<ZwpTabletToolV2 *>(data);
        assert(*obj == wldata);
        {
            return obj->frame()(time);
        }
    },
};
ZwpTabletToolV2::ZwpTabletToolV2(zwp_tablet_tool_v2 *data)
    : version_(zwp_tablet_tool_v2_get_version(data)), data_(data) {
    zwp_tablet_tool_v2_set_user_data(*this, this);
    zwp_tablet_tool_v2_add_listener(*this, &ZwpTabletToolV2::listener, this);
}
void ZwpTabletToolV2::destructor(zwp_tablet_tool_v2 *data) {
    auto version = zwp_tablet_tool_v2_get_version(data);
    if (version >= 1) {
        return zwp_tablet_tool_v2_destroy(data);
    }
}
void ZwpTabletToolV2::setCursor(uint32_t serial, WlSurface *surface,
                                int32_t hotspotX, int32_t hotspotY) {
    return zwp_tablet_tool_v2_set_cursor(*this, serial, rawPointer(surface),
                                         hotspotX, hotspotY);
}
} // namespace fcitx::wayland
