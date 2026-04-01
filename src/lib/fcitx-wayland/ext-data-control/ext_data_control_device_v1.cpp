#include "ext_data_control_device_v1.h"
#include <cassert>
#include "ext_data_control_offer_v1.h"
#include "ext_data_control_source_v1.h"
namespace fcitx::wayland {
const struct ext_data_control_device_v1_listener
    ExtDataControlDeviceV1::listener = {
        [](void *data, ext_data_control_device_v1 *wldata,
           ext_data_control_offer_v1 *id) {
            auto *obj = static_cast<ExtDataControlDeviceV1 *>(data);
            assert(*obj == wldata);
            {
                auto *id_ = new ExtDataControlOfferV1(id);
                return obj->dataOffer()(id_);
            }
        },
        [](void *data, ext_data_control_device_v1 *wldata,
           ext_data_control_offer_v1 *id) {
            auto *obj = static_cast<ExtDataControlDeviceV1 *>(data);
            assert(*obj == wldata);
            {
                auto *id_ =
                    id ? static_cast<ExtDataControlOfferV1 *>(
                             ext_data_control_offer_v1_get_user_data(id))
                       : nullptr;
                return obj->selection()(id_);
            }
        },
        [](void *data, ext_data_control_device_v1 *wldata) {
            auto *obj = static_cast<ExtDataControlDeviceV1 *>(data);
            assert(*obj == wldata);
            {
                return obj->finished()();
            }
        },
        [](void *data, ext_data_control_device_v1 *wldata,
           ext_data_control_offer_v1 *id) {
            auto *obj = static_cast<ExtDataControlDeviceV1 *>(data);
            assert(*obj == wldata);
            {
                auto *id_ =
                    id ? static_cast<ExtDataControlOfferV1 *>(
                             ext_data_control_offer_v1_get_user_data(id))
                       : nullptr;
                return obj->primarySelection()(id_);
            }
        },
};
ExtDataControlDeviceV1::ExtDataControlDeviceV1(ext_data_control_device_v1 *data)
    : version_(ext_data_control_device_v1_get_version(data)), data_(data) {
    ext_data_control_device_v1_set_user_data(*this, this);
    ext_data_control_device_v1_add_listener(
        *this, &ExtDataControlDeviceV1::listener, this);
}
void ExtDataControlDeviceV1::destructor(ext_data_control_device_v1 *data) {
    auto version = ext_data_control_device_v1_get_version(data);
    if (version >= 1) {
        return ext_data_control_device_v1_destroy(data);
    }
}
void ExtDataControlDeviceV1::setSelection(ExtDataControlSourceV1 *source) {
    return ext_data_control_device_v1_set_selection(*this, rawPointer(source));
}
void ExtDataControlDeviceV1::setPrimarySelection(
    ExtDataControlSourceV1 *source) {
    return ext_data_control_device_v1_set_primary_selection(*this,
                                                            rawPointer(source));
}
} // namespace fcitx::wayland
