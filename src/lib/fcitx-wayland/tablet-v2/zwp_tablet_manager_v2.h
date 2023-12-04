#ifndef ZWP_TABLET_MANAGER_V2
#define ZWP_TABLET_MANAGER_V2
#include <memory>
#include <wayland-client.h>
#include "fcitx-utils/signals.h"
#include "wayland-tablet-client-protocol.h"
namespace fcitx::wayland {
class WlSeat;
class ZwpTabletSeatV2;
class ZwpTabletManagerV2 final {
public:
    static constexpr const char *interface = "zwp_tablet_manager_v2";
    static constexpr const wl_interface *const wlInterface =
        &zwp_tablet_manager_v2_interface;
    static constexpr const uint32_t version = 1;
    typedef zwp_tablet_manager_v2 wlType;
    operator zwp_tablet_manager_v2 *() { return data_.get(); }
    ZwpTabletManagerV2(wlType *data);
    ZwpTabletManagerV2(ZwpTabletManagerV2 &&other) noexcept = delete;
    ZwpTabletManagerV2 &operator=(ZwpTabletManagerV2 &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    ZwpTabletSeatV2 *getTabletSeat(WlSeat *seat);

private:
    static void destructor(zwp_tablet_manager_v2 *);
    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<zwp_tablet_manager_v2, &destructor> data_;
};
static inline zwp_tablet_manager_v2 *rawPointer(ZwpTabletManagerV2 *p) {
    return p ? static_cast<zwp_tablet_manager_v2 *>(*p) : nullptr;
}
} // namespace fcitx::wayland
#endif
