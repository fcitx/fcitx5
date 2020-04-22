#ifndef WL_SEAT
#define WL_SEAT
#include <memory>
#include <wayland-client.h>
#include "fcitx-utils/signals.h"
namespace fcitx {
namespace wayland {
class WlKeyboard;
class WlPointer;
class WlTouch;
class WlSeat final {
public:
    static constexpr const char *interface = "wl_seat";
    static constexpr const wl_interface *const wlInterface = &wl_seat_interface;
    static constexpr const uint32_t version = 7;
    typedef wl_seat wlType;
    operator wl_seat *() { return data_.get(); }
    WlSeat(wlType *data);
    WlSeat(WlSeat &&other) noexcept = delete;
    WlSeat &operator=(WlSeat &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    WlPointer *getPointer();
    WlKeyboard *getKeyboard();
    WlTouch *getTouch();
    auto &capabilities() { return capabilitiesSignal_; }
    auto &name() { return nameSignal_; }

private:
    static void destructor(wl_seat *);
    static const struct wl_seat_listener listener;
    fcitx::Signal<void(uint32_t)> capabilitiesSignal_;
    fcitx::Signal<void(const char *)> nameSignal_;
    uint32_t version_;
    void *userData_ = nullptr;
    std::unique_ptr<wl_seat, decltype(&destructor)> data_;
};
static inline wl_seat *rawPointer(WlSeat *p) {
    return p ? static_cast<wl_seat *>(*p) : nullptr;
}
} // namespace wayland
} // namespace fcitx
#endif
