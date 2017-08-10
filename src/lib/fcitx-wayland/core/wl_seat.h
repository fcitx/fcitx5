#ifndef WL_SEAT
#define WL_SEAT
#include "fcitx-utils/signals.h"
#include <memory>
#include <wayland-client.h>
namespace fcitx {
namespace wayland {
class WlKeyboard;
class WlPointer;
class WlTouch;
class WlSeat final {
public:
    static constexpr const char *interface = "wl_seat";
    static constexpr const wl_interface *const wlInterface = &wl_seat_interface;
    static constexpr const uint32_t version = 5;
    typedef wl_seat wlType;
    operator wl_seat *() { return data_.get(); }
    WlSeat(wlType *data);
    WlSeat(WlSeat &&other) noexcept = delete;
    WlSeat &operator=(WlSeat &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
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
    std::unique_ptr<wl_seat, decltype(&destructor)> data_;
};
}
}
#endif
