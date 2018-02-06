#ifndef WL_DATA_DEVICE_MANAGER
#define WL_DATA_DEVICE_MANAGER
#include "fcitx-utils/signals.h"
#include <memory>
#include <wayland-client.h>
namespace fcitx {
namespace wayland {
class WlDataDevice;
class WlDataSource;
class WlSeat;
class WlDataDeviceManager final {
public:
    static constexpr const char *interface = "wl_data_device_manager";
    static constexpr const wl_interface *const wlInterface =
        &wl_data_device_manager_interface;
    static constexpr const uint32_t version = 3;
    typedef wl_data_device_manager wlType;
    operator wl_data_device_manager *() { return data_.get(); }
    WlDataDeviceManager(wlType *data);
    WlDataDeviceManager(WlDataDeviceManager &&other) noexcept = delete;
    WlDataDeviceManager &
    operator=(WlDataDeviceManager &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    WlDataSource *createDataSource();
    WlDataDevice *getDataDevice(WlSeat *seat);

private:
    static void destructor(wl_data_device_manager *);
    uint32_t version_;
    std::unique_ptr<wl_data_device_manager, decltype(&destructor)> data_;
};
static inline wl_data_device_manager *rawPointer(WlDataDeviceManager *p) {
    return p ? static_cast<wl_data_device_manager *>(*p) : nullptr;
}
} // namespace wayland
} // namespace fcitx
#endif
