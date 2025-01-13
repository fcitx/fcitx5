#include "zwlr_data_control_source_v1.h"
#include <cassert>
namespace fcitx::wayland {
const struct zwlr_data_control_source_v1_listener
    ZwlrDataControlSourceV1::listener = {
        [](void *data, zwlr_data_control_source_v1 *wldata,
           const char *mimeType, int32_t fd) {
            auto *obj = static_cast<ZwlrDataControlSourceV1 *>(data);
            assert(*obj == wldata);
            {
                return obj->send()(mimeType, fd);
            }
        },
        [](void *data, zwlr_data_control_source_v1 *wldata) {
            auto *obj = static_cast<ZwlrDataControlSourceV1 *>(data);
            assert(*obj == wldata);
            {
                return obj->cancelled()();
            }
        },
};
ZwlrDataControlSourceV1::ZwlrDataControlSourceV1(
    zwlr_data_control_source_v1 *data)
    : version_(zwlr_data_control_source_v1_get_version(data)), data_(data) {
    zwlr_data_control_source_v1_set_user_data(*this, this);
    zwlr_data_control_source_v1_add_listener(
        *this, &ZwlrDataControlSourceV1::listener, this);
}
void ZwlrDataControlSourceV1::destructor(zwlr_data_control_source_v1 *data) {
    auto version = zwlr_data_control_source_v1_get_version(data);
    if (version >= 1) {
        return zwlr_data_control_source_v1_destroy(data);
    }
}
void ZwlrDataControlSourceV1::offer(const char *mimeType) {
    return zwlr_data_control_source_v1_offer(*this, mimeType);
}
} // namespace fcitx::wayland
