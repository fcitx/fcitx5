#include "ext_data_control_manager_v1.h"
#include <cassert>
#include "ext_data_control_device_v1.h"
#include "ext_data_control_source_v1.h"
#include "wl_seat.h"
namespace fcitx::wayland {
ExtDataControlManagerV1::ExtDataControlManagerV1(
    ext_data_control_manager_v1 *data)
    : version_(ext_data_control_manager_v1_get_version(data)), data_(data) {
    ext_data_control_manager_v1_set_user_data(*this, this);
}
void ExtDataControlManagerV1::destructor(ext_data_control_manager_v1 *data) {
    auto version = ext_data_control_manager_v1_get_version(data);
    if (version >= 1) {
        return ext_data_control_manager_v1_destroy(data);
    }
}
ExtDataControlSourceV1 *ExtDataControlManagerV1::createDataSource() {
    return new ExtDataControlSourceV1(
        ext_data_control_manager_v1_create_data_source(*this));
}
ExtDataControlDeviceV1 *ExtDataControlManagerV1::getDataDevice(WlSeat *seat) {
    return new ExtDataControlDeviceV1(
        ext_data_control_manager_v1_get_data_device(*this, rawPointer(seat)));
}
} // namespace fcitx::wayland
