#include "wl_data_device_manager.h"
#include "wl_data_device.h"
#include "wl_data_source.h"
#include "wl_seat.h"
#include <cassert>
namespace fcitx {
namespace wayland {
constexpr const char *WlDataDeviceManager::interface;
constexpr const wl_interface *const WlDataDeviceManager::wlInterface;
const uint32_t WlDataDeviceManager::version;
WlDataDeviceManager::WlDataDeviceManager(wl_data_device_manager *data)
    : version_(wl_data_device_manager_get_version(data)),
      data_(data, &WlDataDeviceManager::destructor) {
    wl_data_device_manager_set_user_data(*this, this);
}
void WlDataDeviceManager::destructor(wl_data_device_manager *data) {
    { return wl_data_device_manager_destroy(data); }
}
WlDataSource *WlDataDeviceManager::createDataSource() {
    return new WlDataSource(wl_data_device_manager_create_data_source(*this));
}
WlDataDevice *WlDataDeviceManager::getDataDevice(WlSeat *seat) {
    return new WlDataDevice(
        wl_data_device_manager_get_data_device(*this, *seat));
}
}
}
