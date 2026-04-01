#ifndef EXT_DATA_CONTROL_MANAGER_V1
#define EXT_DATA_CONTROL_MANAGER_V1
#include <memory>
#include <wayland-client.h>
#include "fcitx-utils/signals.h"
#include "wayland-ext-data-control-unstable-v1-client-protocol.h"
namespace fcitx::wayland {
class ExtDataControlDeviceV1;
class ExtDataControlSourceV1;
class WlSeat;
class ExtDataControlManagerV1 final {
public:
    static constexpr const char *interface = "ext_data_control_manager_v1";
    static constexpr const wl_interface *const wlInterface =
        &ext_data_control_manager_v1_interface;
    static constexpr const uint32_t version = 1;
    typedef ext_data_control_manager_v1 wlType;
    operator ext_data_control_manager_v1 *() { return data_.get(); }
    ExtDataControlManagerV1(wlType *data);
    ExtDataControlManagerV1(ExtDataControlManagerV1 &&other) noexcept = delete;
    ExtDataControlManagerV1 &
    operator=(ExtDataControlManagerV1 &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    ExtDataControlSourceV1 *createDataSource();
    ExtDataControlDeviceV1 *getDataDevice(WlSeat *seat);

private:
    static void destructor(ext_data_control_manager_v1 *);
    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<ext_data_control_manager_v1, &destructor> data_;
};
static inline ext_data_control_manager_v1 *
rawPointer(ExtDataControlManagerV1 *p) {
    return p ? static_cast<ext_data_control_manager_v1 *>(*p) : nullptr;
}
} // namespace fcitx::wayland
#endif
