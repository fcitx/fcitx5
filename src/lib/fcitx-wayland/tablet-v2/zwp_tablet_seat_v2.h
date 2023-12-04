#ifndef ZWP_TABLET_SEAT_V2
#define ZWP_TABLET_SEAT_V2
#include <memory>
#include <wayland-client.h>
#include "fcitx-utils/signals.h"
#include "wayland-tablet-client-protocol.h"
namespace fcitx::wayland {
class ZwpTabletPadV2;
class ZwpTabletToolV2;
class ZwpTabletV2;
class ZwpTabletSeatV2 final {
public:
    static constexpr const char *interface = "zwp_tablet_seat_v2";
    static constexpr const wl_interface *const wlInterface =
        &zwp_tablet_seat_v2_interface;
    static constexpr const uint32_t version = 1;
    typedef zwp_tablet_seat_v2 wlType;
    operator zwp_tablet_seat_v2 *() { return data_.get(); }
    ZwpTabletSeatV2(wlType *data);
    ZwpTabletSeatV2(ZwpTabletSeatV2 &&other) noexcept = delete;
    ZwpTabletSeatV2 &operator=(ZwpTabletSeatV2 &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    auto &tabletAdded() { return tabletAddedSignal_; }
    auto &toolAdded() { return toolAddedSignal_; }
    auto &padAdded() { return padAddedSignal_; }

private:
    static void destructor(zwp_tablet_seat_v2 *);
    static const struct zwp_tablet_seat_v2_listener listener;
    fcitx::Signal<void(ZwpTabletV2 *)> tabletAddedSignal_;
    fcitx::Signal<void(ZwpTabletToolV2 *)> toolAddedSignal_;
    fcitx::Signal<void(ZwpTabletPadV2 *)> padAddedSignal_;
    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<zwp_tablet_seat_v2, &destructor> data_;
};
static inline zwp_tablet_seat_v2 *rawPointer(ZwpTabletSeatV2 *p) {
    return p ? static_cast<zwp_tablet_seat_v2 *>(*p) : nullptr;
}
} // namespace fcitx::wayland
#endif
