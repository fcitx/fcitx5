#include "ext_data_control_source_v1.h"
#include <cassert>
namespace fcitx::wayland {
const struct ext_data_control_source_v1_listener
    ExtDataControlSourceV1::listener = {
        [](void *data, ext_data_control_source_v1 *wldata, const char *mimeType,
           int32_t fd) {
            auto *obj = static_cast<ExtDataControlSourceV1 *>(data);
            assert(*obj == wldata);
            {
                return obj->send()(mimeType, fd);
            }
        },
        [](void *data, ext_data_control_source_v1 *wldata) {
            auto *obj = static_cast<ExtDataControlSourceV1 *>(data);
            assert(*obj == wldata);
            {
                return obj->cancelled()();
            }
        },
};
ExtDataControlSourceV1::ExtDataControlSourceV1(ext_data_control_source_v1 *data)
    : version_(ext_data_control_source_v1_get_version(data)), data_(data) {
    ext_data_control_source_v1_set_user_data(*this, this);
    ext_data_control_source_v1_add_listener(
        *this, &ExtDataControlSourceV1::listener, this);
}
void ExtDataControlSourceV1::destructor(ext_data_control_source_v1 *data) {
    auto version = ext_data_control_source_v1_get_version(data);
    if (version >= 1) {
        return ext_data_control_source_v1_destroy(data);
    }
}
void ExtDataControlSourceV1::offer(const char *mimeType) {
    return ext_data_control_source_v1_offer(*this, mimeType);
}
} // namespace fcitx::wayland
