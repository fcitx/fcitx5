#ifndef WL_DATA_DEVICE_MANAGER_H_
#define WL_DATA_DEVICE_MANAGER_H_
#include <cstdint>
#include <wayland-client.h>
#include <wayland-util.h>
#include "fcitx-utils/misc.h"
namespace fcitx::wayland {

class WlDataDevice;
class WlDataSource;
class WlSeat;

class WlDataDeviceManager final {
public:
    static constexpr const char *interface = "wl_data_device_manager";
    static constexpr const wl_interface *const wlInterface =
        &wl_data_device_manager_interface;
    static constexpr const uint32_t version = 3;
    using wlType = wl_data_device_manager;
    operator wl_data_device_manager *() { return data_.get(); }
    WlDataDeviceManager(wlType *data);
    WlDataDeviceManager(WlDataDeviceManager &&other) noexcept = delete;
    WlDataDeviceManager &
    operator=(WlDataDeviceManager &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    WlDataSource *createDataSource();
    WlDataDevice *getDataDevice(WlSeat *seat);

private:
    static void destructor(wl_data_device_manager *);

    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<wl_data_device_manager, &destructor> data_;
};
static inline wl_data_device_manager *rawPointer(WlDataDeviceManager *p) {
    return p ? static_cast<wl_data_device_manager *>(*p) : nullptr;
}

} // namespace fcitx::wayland

#endif // WL_DATA_DEVICE_MANAGER_H_
