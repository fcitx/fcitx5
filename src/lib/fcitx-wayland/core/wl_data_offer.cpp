#include "wl_data_offer.h"
#include <cassert>
namespace fcitx::wayland {
const struct wl_data_offer_listener WlDataOffer::listener = {
    [](void *data, wl_data_offer *wldata, const char *mimeType) {
        auto *obj = static_cast<WlDataOffer *>(data);
        assert(*obj == wldata);
        {
            return obj->offer()(mimeType);
        }
    },
    [](void *data, wl_data_offer *wldata, uint32_t sourceActions) {
        auto *obj = static_cast<WlDataOffer *>(data);
        assert(*obj == wldata);
        {
            return obj->sourceActions()(sourceActions);
        }
    },
    [](void *data, wl_data_offer *wldata, uint32_t dndAction) {
        auto *obj = static_cast<WlDataOffer *>(data);
        assert(*obj == wldata);
        {
            return obj->action()(dndAction);
        }
    },
};
WlDataOffer::WlDataOffer(wl_data_offer *data)
    : version_(wl_data_offer_get_version(data)), data_(data) {
    wl_data_offer_set_user_data(*this, this);
    wl_data_offer_add_listener(*this, &WlDataOffer::listener, this);
}
void WlDataOffer::destructor(wl_data_offer *data) {
    auto version = wl_data_offer_get_version(data);
    if (version >= 1) {
        return wl_data_offer_destroy(data);
    }
}
void WlDataOffer::accept(uint32_t serial, const char *mimeType) {
    return wl_data_offer_accept(*this, serial, mimeType);
}
void WlDataOffer::receive(const char *mimeType, int32_t fd) {
    return wl_data_offer_receive(*this, mimeType, fd);
}
void WlDataOffer::finish() { return wl_data_offer_finish(*this); }
void WlDataOffer::setActions(uint32_t dndActions, uint32_t preferredAction) {
    return wl_data_offer_set_actions(*this, dndActions, preferredAction);
}
} // namespace fcitx::wayland
