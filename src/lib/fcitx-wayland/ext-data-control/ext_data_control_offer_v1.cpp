#include "ext_data_control_offer_v1.h"
#include <cassert>
namespace fcitx::wayland {
const struct ext_data_control_offer_v1_listener
    ExtDataControlOfferV1::listener = {
        [](void *data, ext_data_control_offer_v1 *wldata,
           const char *mimeType) {
            auto *obj = static_cast<ExtDataControlOfferV1 *>(data);
            assert(*obj == wldata);
            {
                return obj->offer()(mimeType);
            }
        },
};
ExtDataControlOfferV1::ExtDataControlOfferV1(ext_data_control_offer_v1 *data)
    : version_(ext_data_control_offer_v1_get_version(data)), data_(data) {
    ext_data_control_offer_v1_set_user_data(*this, this);
    ext_data_control_offer_v1_add_listener(
        *this, &ExtDataControlOfferV1::listener, this);
}
void ExtDataControlOfferV1::destructor(ext_data_control_offer_v1 *data) {
    auto version = ext_data_control_offer_v1_get_version(data);
    if (version >= 1) {
        return ext_data_control_offer_v1_destroy(data);
    }
}
void ExtDataControlOfferV1::receive(const char *mimeType, int32_t fd) {
    return ext_data_control_offer_v1_receive(*this, mimeType, fd);
}
} // namespace fcitx::wayland
