#ifndef EXT_DATA_CONTROL_DEVICE_V1
#define EXT_DATA_CONTROL_DEVICE_V1
#include <memory>
#include <wayland-client.h>
#include "fcitx-utils/signals.h"
#include "wayland-ext-data-control-unstable-v1-client-protocol.h"
namespace fcitx::wayland {
class ExtDataControlOfferV1;
class ExtDataControlSourceV1;
class ExtDataControlDeviceV1 final {
public:
    static constexpr const char *interface = "ext_data_control_device_v1";
    static constexpr const wl_interface *const wlInterface =
        &ext_data_control_device_v1_interface;
    static constexpr const uint32_t version = 1;
    typedef ext_data_control_device_v1 wlType;
    operator ext_data_control_device_v1 *() { return data_.get(); }
    ExtDataControlDeviceV1(wlType *data);
    ExtDataControlDeviceV1(ExtDataControlDeviceV1 &&other) noexcept = delete;
    ExtDataControlDeviceV1 &
    operator=(ExtDataControlDeviceV1 &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    void setSelection(ExtDataControlSourceV1 *source);
    void setPrimarySelection(ExtDataControlSourceV1 *source);
    auto &dataOffer() { return dataOfferSignal_; }
    auto &selection() { return selectionSignal_; }
    auto &finished() { return finishedSignal_; }
    auto &primarySelection() { return primarySelectionSignal_; }

private:
    static void destructor(ext_data_control_device_v1 *);
    static const struct ext_data_control_device_v1_listener listener;
    fcitx::Signal<void(ExtDataControlOfferV1 *)> dataOfferSignal_;
    fcitx::Signal<void(ExtDataControlOfferV1 *)> selectionSignal_;
    fcitx::Signal<void()> finishedSignal_;
    fcitx::Signal<void(ExtDataControlOfferV1 *)> primarySelectionSignal_;
    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<ext_data_control_device_v1, &destructor> data_;
};
static inline ext_data_control_device_v1 *
rawPointer(ExtDataControlDeviceV1 *p) {
    return p ? static_cast<ext_data_control_device_v1 *>(*p) : nullptr;
}
} // namespace fcitx::wayland
#endif
