#include "zwlr_data_control_source_v1.h"
#include <cassert>
#include "wayland-wlr-data-control-unstable-v1-client-protocol.h"

namespace fcitx::wayland {
const struct zwlr_data_control_source_v1_listener
    ZwlrDataControlSourceV1::listener = {
        .send =
            [](void *data, zwlr_data_control_source_v1 *wldata,
               const char *mimeType, int32_t fd) {
                auto *obj = static_cast<ZwlrDataControlSourceV1 *>(data);
                assert(*obj == wldata);
                {
                    obj->send()(mimeType, fd);
                }
            },
        .cancelled =
            [](void *data, zwlr_data_control_source_v1 *wldata) {
                auto *obj = static_cast<ZwlrDataControlSourceV1 *>(data);
                assert(*obj == wldata);
                {
                    obj->cancelled()();
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
    const auto version = zwlr_data_control_source_v1_get_version(data);
    if (version >= 1) {
        zwlr_data_control_source_v1_destroy(data);
        return;
    }
}
void ZwlrDataControlSourceV1::offer(const char *mimeType) {
    zwlr_data_control_source_v1_offer(*this, mimeType);
}

} // namespace fcitx::wayland
