#include "zwlr_data_control_offer_v1.h"
#include <cassert>
namespace fcitx::wayland {
const struct zwlr_data_control_offer_v1_listener
    ZwlrDataControlOfferV1::listener = {
        [](void *data, zwlr_data_control_offer_v1 *wldata,
           const char *mimeType) {
            auto *obj = static_cast<ZwlrDataControlOfferV1 *>(data);
            assert(*obj == wldata);
            {
                return obj->offer()(mimeType);
            }
        },
};
ZwlrDataControlOfferV1::ZwlrDataControlOfferV1(zwlr_data_control_offer_v1 *data)
    : version_(zwlr_data_control_offer_v1_get_version(data)), data_(data) {
    zwlr_data_control_offer_v1_set_user_data(*this, this);
    zwlr_data_control_offer_v1_add_listener(
        *this, &ZwlrDataControlOfferV1::listener, this);
}
void ZwlrDataControlOfferV1::destructor(zwlr_data_control_offer_v1 *data) {
    auto version = zwlr_data_control_offer_v1_get_version(data);
    if (version >= 1) {
        return zwlr_data_control_offer_v1_destroy(data);
    }
}
void ZwlrDataControlOfferV1::receive(const char *mimeType, int32_t fd) {
    return zwlr_data_control_offer_v1_receive(*this, mimeType, fd);
}
} // namespace fcitx::wayland
