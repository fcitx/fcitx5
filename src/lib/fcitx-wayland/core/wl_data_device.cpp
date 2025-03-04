#include "wl_data_device.h"
#include <cassert>
#include "wl_data_offer.h"
#include "wl_data_source.h"
#include "wl_surface.h"

namespace fcitx::wayland {
const struct wl_data_device_listener WlDataDevice::listener = {
    .data_offer =
        [](void *data, wl_data_device *wldata, wl_data_offer *id) {
            auto *obj = static_cast<WlDataDevice *>(data);
            assert(*obj == wldata);
            {
                auto *id_ = new WlDataOffer(id);
                obj->dataOffer()(id_);
            }
        },
    .enter =
        [](void *data, wl_data_device *wldata, uint32_t serial,
           wl_surface *surface, wl_fixed_t x, wl_fixed_t y, wl_data_offer *id) {
            auto *obj = static_cast<WlDataDevice *>(data);
            assert(*obj == wldata);
            {
                if (!surface) {
                    return;
                }
                auto *surface_ =
                    static_cast<WlSurface *>(wl_surface_get_user_data(surface));
                auto *id_ = id ? static_cast<WlDataOffer *>(
                                     wl_data_offer_get_user_data(id))
                               : nullptr;
                obj->enter()(serial, surface_, x, y, id_);
            }
        },
    .leave =
        [](void *data, wl_data_device *wldata) {
            auto *obj = static_cast<WlDataDevice *>(data);
            assert(*obj == wldata);
            {
                obj->leave()();
            }
        },
    .motion =
        [](void *data, wl_data_device *wldata, uint32_t time, wl_fixed_t x,
           wl_fixed_t y) {
            auto *obj = static_cast<WlDataDevice *>(data);
            assert(*obj == wldata);
            {
                obj->motion()(time, x, y);
            }
        },
    .drop =
        [](void *data, wl_data_device *wldata) {
            auto *obj = static_cast<WlDataDevice *>(data);
            assert(*obj == wldata);
            {
                obj->drop()();
            }
        },
    .selection =
        [](void *data, wl_data_device *wldata, wl_data_offer *id) {
            auto *obj = static_cast<WlDataDevice *>(data);
            assert(*obj == wldata);
            {
                auto *id_ = id ? static_cast<WlDataOffer *>(
                                     wl_data_offer_get_user_data(id))
                               : nullptr;
                obj->selection()(id_);
            }
        },
};

WlDataDevice::WlDataDevice(wl_data_device *data)
    : version_(wl_data_device_get_version(data)), data_(data) {
    wl_data_device_set_user_data(*this, this);
    wl_data_device_add_listener(*this, &WlDataDevice::listener, this);
}

void WlDataDevice::destructor(wl_data_device *data) {
    const auto version = wl_data_device_get_version(data);
    if (version >= 2) {
        wl_data_device_release(data);
        return;
    }
    wl_data_device_destroy(data);
}
void WlDataDevice::startDrag(WlDataSource *source, WlSurface *origin,
                             WlSurface *icon, uint32_t serial) {
    wl_data_device_start_drag(*this, rawPointer(source), rawPointer(origin),
                              rawPointer(icon), serial);
}
void WlDataDevice::setSelection(WlDataSource *source, uint32_t serial) {
    wl_data_device_set_selection(*this, rawPointer(source), serial);
}

} // namespace fcitx::wayland
