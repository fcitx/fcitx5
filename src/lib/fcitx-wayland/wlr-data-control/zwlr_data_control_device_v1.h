#ifndef ZWLR_DATA_CONTROL_DEVICE_V1_H_
#define ZWLR_DATA_CONTROL_DEVICE_V1_H_
#include <cstdint>
#include <wayland-client.h>
#include <wayland-util.h>
#include "fcitx-utils/misc.h"
#include "fcitx-utils/signals.h"
#include "wayland-wlr-data-control-unstable-v1-client-protocol.h" // IWYU pragma: export
namespace fcitx::wayland {

class ZwlrDataControlOfferV1;
class ZwlrDataControlSourceV1;

class ZwlrDataControlDeviceV1 final {
public:
    static constexpr const char *interface = "zwlr_data_control_device_v1";
    static constexpr const wl_interface *const wlInterface =
        &zwlr_data_control_device_v1_interface;
    static constexpr const uint32_t version = 2;
    using wlType = zwlr_data_control_device_v1;
    operator zwlr_data_control_device_v1 *() { return data_.get(); }
    ZwlrDataControlDeviceV1(wlType *data);
    ZwlrDataControlDeviceV1(ZwlrDataControlDeviceV1 &&other) noexcept = delete;
    ZwlrDataControlDeviceV1 &
    operator=(ZwlrDataControlDeviceV1 &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    void setSelection(ZwlrDataControlSourceV1 *source);
    void setPrimarySelection(ZwlrDataControlSourceV1 *source);

    auto &dataOffer() { return dataOfferSignal_; }
    auto &selection() { return selectionSignal_; }
    auto &finished() { return finishedSignal_; }
    auto &primarySelection() { return primarySelectionSignal_; }

private:
    static void destructor(zwlr_data_control_device_v1 *);
    static const struct zwlr_data_control_device_v1_listener listener;
    fcitx::Signal<void(ZwlrDataControlOfferV1 *)> dataOfferSignal_;
    fcitx::Signal<void(ZwlrDataControlOfferV1 *)> selectionSignal_;
    fcitx::Signal<void()> finishedSignal_;
    fcitx::Signal<void(ZwlrDataControlOfferV1 *)> primarySelectionSignal_;

    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<zwlr_data_control_device_v1, &destructor> data_;
};
static inline zwlr_data_control_device_v1 *
rawPointer(ZwlrDataControlDeviceV1 *p) {
    return p ? static_cast<zwlr_data_control_device_v1 *>(*p) : nullptr;
}

} // namespace fcitx::wayland

#endif // ZWLR_DATA_CONTROL_DEVICE_V1_H_
