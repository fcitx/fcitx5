#include "wl_data_offer.h"
#include <cassert>

namespace fcitx::wayland {
const struct wl_data_offer_listener WlDataOffer::listener = {
#if defined(WL_DATA_OFFER_OFFER_SINCE_VERSION)
    .offer =
        [](void *data, wl_data_offer *wldata, const char *mimeType) {
            auto *obj = static_cast<WlDataOffer *>(data);
            assert(*obj == wldata);
            {
                obj->offer()(mimeType);
            }
        },
#endif
#if defined(WL_DATA_OFFER_SOURCE_ACTIONS_SINCE_VERSION)
    .source_actions =
        [](void *data, wl_data_offer *wldata, uint32_t sourceActions) {
            auto *obj = static_cast<WlDataOffer *>(data);
            assert(*obj == wldata);
            {
                obj->sourceActions()(sourceActions);
            }
        },
#endif
#if defined(WL_DATA_OFFER_ACTION_SINCE_VERSION)
    .action =
        [](void *data, wl_data_offer *wldata, uint32_t dndAction) {
            auto *obj = static_cast<WlDataOffer *>(data);
            assert(*obj == wldata);
            {
                obj->action()(dndAction);
            }
        },
#endif
};

WlDataOffer::WlDataOffer(wl_data_offer *data)
    : version_(wl_data_offer_get_version(data)), data_(data) {
    wl_data_offer_set_user_data(*this, this);
    wl_data_offer_add_listener(*this, &WlDataOffer::listener, this);
}

void WlDataOffer::destructor(wl_data_offer *data) {
    wl_data_offer_destroy(data);
}
#if defined(WL_DATA_OFFER_ACCEPT_SINCE_VERSION)
void WlDataOffer::accept(uint32_t serial, const char *mimeType) {
    wl_data_offer_accept(*this, serial, mimeType);
}
#endif
#if defined(WL_DATA_OFFER_RECEIVE_SINCE_VERSION)
void WlDataOffer::receive(const char *mimeType, int32_t fd) {
    wl_data_offer_receive(*this, mimeType, fd);
}
#endif
#if defined(WL_DATA_OFFER_FINISH_SINCE_VERSION)
void WlDataOffer::finish() { wl_data_offer_finish(*this); }
#endif
#if defined(WL_DATA_OFFER_SET_ACTIONS_SINCE_VERSION)
void WlDataOffer::setActions(uint32_t dndActions, uint32_t preferredAction) {
    wl_data_offer_set_actions(*this, dndActions, preferredAction);
}
#endif

} // namespace fcitx::wayland
