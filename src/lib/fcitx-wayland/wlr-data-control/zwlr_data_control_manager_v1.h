#ifndef ZWLR_DATA_CONTROL_MANAGER_V1
#define ZWLR_DATA_CONTROL_MANAGER_V1
#include <wayland-client.h>
#include "fcitx-utils/signals.h"
#include "wayland-wlr-data-control-unstable-v1-client-protocol.h"
namespace fcitx::wayland {
class WlSeat;
class ZwlrDataControlDeviceV1;
class ZwlrDataControlSourceV1;
class ZwlrDataControlManagerV1 final {
public:
    static constexpr const char *interface = "zwlr_data_control_manager_v1";
    static constexpr const wl_interface *const wlInterface =
        &zwlr_data_control_manager_v1_interface;
    static constexpr const uint32_t version = 2;
    typedef zwlr_data_control_manager_v1 wlType;
    operator zwlr_data_control_manager_v1 *() { return data_.get(); }
    ZwlrDataControlManagerV1(wlType *data);
    ZwlrDataControlManagerV1(ZwlrDataControlManagerV1 &&other) noexcept =
        delete;
    ZwlrDataControlManagerV1 &
    operator=(ZwlrDataControlManagerV1 &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    ZwlrDataControlSourceV1 *createDataSource();
    ZwlrDataControlDeviceV1 *getDataDevice(WlSeat *seat);

private:
    static void destructor(zwlr_data_control_manager_v1 *);
    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<zwlr_data_control_manager_v1, &destructor> data_;
};
static inline zwlr_data_control_manager_v1 *
rawPointer(ZwlrDataControlManagerV1 *p) {
    return p ? static_cast<zwlr_data_control_manager_v1 *>(*p) : nullptr;
}
} // namespace fcitx::wayland
#endif
