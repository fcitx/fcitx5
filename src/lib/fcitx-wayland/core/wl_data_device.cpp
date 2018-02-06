#include "wl_data_device.h"
#include "wl_data_offer.h"
#include "wl_data_source.h"
#include "wl_surface.h"
#include <cassert>
namespace fcitx {
namespace wayland {
constexpr const char *WlDataDevice::interface;
constexpr const wl_interface *const WlDataDevice::wlInterface;
const uint32_t WlDataDevice::version;
const struct wl_data_device_listener WlDataDevice::listener = {
    [](void *data, wl_data_device *wldata, wl_data_offer *id) {
        auto obj = static_cast<WlDataDevice *>(data);
        assert(*obj == wldata);
        {
            auto id_ = new WlDataOffer(id);
            return obj->dataOffer()(id_);
        }
    },
    [](void *data, wl_data_device *wldata, uint32_t serial, wl_surface *surface,
       wl_fixed_t x, wl_fixed_t y, wl_data_offer *id) {
        auto obj = static_cast<WlDataDevice *>(data);
        assert(*obj == wldata);
        {
            auto surface_ =
                static_cast<WlSurface *>(wl_surface_get_user_data(surface));
            auto id_ =
                static_cast<WlDataOffer *>(wl_data_offer_get_user_data(id));
            return obj->enter()(serial, surface_, x, y, id_);
        }
    },
    [](void *data, wl_data_device *wldata) {
        auto obj = static_cast<WlDataDevice *>(data);
        assert(*obj == wldata);
        { return obj->leave()(); }
    },
    [](void *data, wl_data_device *wldata, uint32_t time, wl_fixed_t x,
       wl_fixed_t y) {
        auto obj = static_cast<WlDataDevice *>(data);
        assert(*obj == wldata);
        { return obj->motion()(time, x, y); }
    },
    [](void *data, wl_data_device *wldata) {
        auto obj = static_cast<WlDataDevice *>(data);
        assert(*obj == wldata);
        { return obj->drop()(); }
    },
    [](void *data, wl_data_device *wldata, wl_data_offer *id) {
        auto obj = static_cast<WlDataDevice *>(data);
        assert(*obj == wldata);
        {
            auto id_ =
                static_cast<WlDataOffer *>(wl_data_offer_get_user_data(id));
            return obj->selection()(id_);
        }
    },
};
WlDataDevice::WlDataDevice(wl_data_device *data)
    : version_(wl_data_device_get_version(data)),
      data_(data, &WlDataDevice::destructor) {
    wl_data_device_set_user_data(*this, this);
    wl_data_device_add_listener(*this, &WlDataDevice::listener, this);
}
void WlDataDevice::destructor(wl_data_device *data) {
    auto version = wl_data_device_get_version(data);
    if (version >= 2) {
        return wl_data_device_release(data);
    } else {
        return wl_data_device_destroy(data);
    }
}
void WlDataDevice::startDrag(WlDataSource *source, WlSurface *origin,
                             WlSurface *icon, uint32_t serial) {
    return wl_data_device_start_drag(*this, rawPointer(source),
                                     rawPointer(origin), rawPointer(icon),
                                     serial);
}
void WlDataDevice::setSelection(WlDataSource *source, uint32_t serial) {
    return wl_data_device_set_selection(*this, rawPointer(source), serial);
}
} // namespace wayland
} // namespace fcitx
