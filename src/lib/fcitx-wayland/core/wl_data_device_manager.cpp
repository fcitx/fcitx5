#include "wl_data_device_manager.h"
#include "wl_data_device.h"
#include "wl_data_source.h"
#include "wl_seat.h"

namespace fcitx::wayland {

WlDataDeviceManager::WlDataDeviceManager(wl_data_device_manager *data)
    : version_(wl_data_device_manager_get_version(data)), data_(data) {
    wl_data_device_manager_set_user_data(*this, this);
}

void WlDataDeviceManager::destructor(wl_data_device_manager *data) {
    const auto version = wl_data_device_manager_get_version(data);
#if defined(WL_DATA_DEVICE_MANAGER_RELEASE_SINCE_VERSION)
    if (version >= 4) {
        wl_data_device_manager_release(data);
        return;
    }
#endif
    wl_data_device_manager_destroy(data);
}
WlDataSource *WlDataDeviceManager::createDataSource() {
    return new WlDataSource(wl_data_device_manager_create_data_source(*this));
}
WlDataDevice *WlDataDeviceManager::getDataDevice(WlSeat *seat) {
    return new WlDataDevice(
        wl_data_device_manager_get_data_device(*this, rawPointer(seat)));
}

} // namespace fcitx::wayland
