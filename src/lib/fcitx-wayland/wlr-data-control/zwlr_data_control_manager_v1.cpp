#include "zwlr_data_control_manager_v1.h"
#include "wayland-wlr-data-control-unstable-v1-client-protocol.h"
#include "wl_seat.h"
#include "zwlr_data_control_device_v1.h"
#include "zwlr_data_control_source_v1.h"

namespace fcitx::wayland {

ZwlrDataControlManagerV1::ZwlrDataControlManagerV1(
    zwlr_data_control_manager_v1 *data)
    : version_(zwlr_data_control_manager_v1_get_version(data)), data_(data) {
    zwlr_data_control_manager_v1_set_user_data(*this, this);
}

void ZwlrDataControlManagerV1::destructor(zwlr_data_control_manager_v1 *data) {
    const auto version = zwlr_data_control_manager_v1_get_version(data);
    if (version >= 1) {
        zwlr_data_control_manager_v1_destroy(data);
        return;
    }
}
ZwlrDataControlSourceV1 *ZwlrDataControlManagerV1::createDataSource() {
    return new ZwlrDataControlSourceV1(
        zwlr_data_control_manager_v1_create_data_source(*this));
}
ZwlrDataControlDeviceV1 *ZwlrDataControlManagerV1::getDataDevice(WlSeat *seat) {
    return new ZwlrDataControlDeviceV1(
        zwlr_data_control_manager_v1_get_data_device(*this, rawPointer(seat)));
}

} // namespace fcitx::wayland
