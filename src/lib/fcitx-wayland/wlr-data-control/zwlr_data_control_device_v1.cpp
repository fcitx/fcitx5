#include "zwlr_data_control_device_v1.h"
#include <cassert>
#include "zwlr_data_control_offer_v1.h"
#include "zwlr_data_control_source_v1.h"
namespace fcitx::wayland {
const struct zwlr_data_control_device_v1_listener
    ZwlrDataControlDeviceV1::listener = {
        [](void *data, zwlr_data_control_device_v1 *wldata,
           zwlr_data_control_offer_v1 *id) {
            auto *obj = static_cast<ZwlrDataControlDeviceV1 *>(data);
            assert(*obj == wldata);
            {
                auto *id_ = new ZwlrDataControlOfferV1(id);
                return obj->dataOffer()(id_);
            }
        },
        [](void *data, zwlr_data_control_device_v1 *wldata,
           zwlr_data_control_offer_v1 *id) {
            auto *obj = static_cast<ZwlrDataControlDeviceV1 *>(data);
            assert(*obj == wldata);
            {
                auto *id_ =
                    id ? static_cast<ZwlrDataControlOfferV1 *>(
                             zwlr_data_control_offer_v1_get_user_data(id))
                       : nullptr;
                return obj->selection()(id_);
            }
        },
        [](void *data, zwlr_data_control_device_v1 *wldata) {
            auto *obj = static_cast<ZwlrDataControlDeviceV1 *>(data);
            assert(*obj == wldata);
            {
                return obj->finished()();
            }
        },
        [](void *data, zwlr_data_control_device_v1 *wldata,
           zwlr_data_control_offer_v1 *id) {
            auto *obj = static_cast<ZwlrDataControlDeviceV1 *>(data);
            assert(*obj == wldata);
            {
                auto *id_ =
                    id ? static_cast<ZwlrDataControlOfferV1 *>(
                             zwlr_data_control_offer_v1_get_user_data(id))
                       : nullptr;
                return obj->primarySelection()(id_);
            }
        },
};
ZwlrDataControlDeviceV1::ZwlrDataControlDeviceV1(
    zwlr_data_control_device_v1 *data)
    : version_(zwlr_data_control_device_v1_get_version(data)), data_(data) {
    zwlr_data_control_device_v1_set_user_data(*this, this);
    zwlr_data_control_device_v1_add_listener(
        *this, &ZwlrDataControlDeviceV1::listener, this);
}
void ZwlrDataControlDeviceV1::destructor(zwlr_data_control_device_v1 *data) {
    auto version = zwlr_data_control_device_v1_get_version(data);
    if (version >= 1) {
        return zwlr_data_control_device_v1_destroy(data);
    }
}
void ZwlrDataControlDeviceV1::setSelection(ZwlrDataControlSourceV1 *source) {
    return zwlr_data_control_device_v1_set_selection(*this, rawPointer(source));
}
void ZwlrDataControlDeviceV1::setPrimarySelection(
    ZwlrDataControlSourceV1 *source) {
    return zwlr_data_control_device_v1_set_primary_selection(
        *this, rawPointer(source));
}
} // namespace fcitx::wayland
