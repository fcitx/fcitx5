#ifndef WL_DATA_DEVICE_H_
#define WL_DATA_DEVICE_H_
#include <cstdint>
#include <wayland-client.h>
#include <wayland-util.h>
#include "fcitx-utils/misc.h"
#include "fcitx-utils/signals.h"
namespace fcitx::wayland {

class WlDataOffer;
class WlDataSource;
class WlSurface;

class WlDataDevice final {
public:
    static constexpr const char *interface = "wl_data_device";
    static constexpr const wl_interface *const wlInterface =
        &wl_data_device_interface;
    static constexpr const uint32_t version = 3;
    using wlType = wl_data_device;
    operator wl_data_device *() { return data_.get(); }
    WlDataDevice(wlType *data);
    WlDataDevice(WlDataDevice &&other) noexcept = delete;
    WlDataDevice &operator=(WlDataDevice &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    void startDrag(WlDataSource *source, WlSurface *origin, WlSurface *icon,
                   uint32_t serial);
    void setSelection(WlDataSource *source, uint32_t serial);

    auto &dataOffer() { return dataOfferSignal_; }
    auto &enter() { return enterSignal_; }
    auto &leave() { return leaveSignal_; }
    auto &motion() { return motionSignal_; }
    auto &drop() { return dropSignal_; }
    auto &selection() { return selectionSignal_; }

private:
    static void destructor(wl_data_device *);
    static const struct wl_data_device_listener listener;
    fcitx::Signal<void(WlDataOffer *)> dataOfferSignal_;
    fcitx::Signal<void(uint32_t, WlSurface *, wl_fixed_t, wl_fixed_t,
                       WlDataOffer *)>
        enterSignal_;
    fcitx::Signal<void()> leaveSignal_;
    fcitx::Signal<void(uint32_t, wl_fixed_t, wl_fixed_t)> motionSignal_;
    fcitx::Signal<void()> dropSignal_;
    fcitx::Signal<void(WlDataOffer *)> selectionSignal_;

    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<wl_data_device, &destructor> data_;
};
static inline wl_data_device *rawPointer(WlDataDevice *p) {
    return p ? static_cast<wl_data_device *>(*p) : nullptr;
}

} // namespace fcitx::wayland

#endif // WL_DATA_DEVICE_H_
