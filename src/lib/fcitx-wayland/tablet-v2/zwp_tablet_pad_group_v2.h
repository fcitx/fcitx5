#ifndef ZWP_TABLET_PAD_GROUP_V2
#define ZWP_TABLET_PAD_GROUP_V2
#include <wayland-client.h>
#include "fcitx-utils/signals.h"
#include "wayland-tablet-client-protocol.h"
namespace fcitx::wayland {
class ZwpTabletPadRingV2;
class ZwpTabletPadStripV2;
class ZwpTabletPadGroupV2 final {
public:
    static constexpr const char *interface = "zwp_tablet_pad_group_v2";
    static constexpr const wl_interface *const wlInterface =
        &zwp_tablet_pad_group_v2_interface;
    static constexpr const uint32_t version = 1;
    typedef zwp_tablet_pad_group_v2 wlType;
    operator zwp_tablet_pad_group_v2 *() { return data_.get(); }
    ZwpTabletPadGroupV2(wlType *data);
    ZwpTabletPadGroupV2(ZwpTabletPadGroupV2 &&other) noexcept = delete;
    ZwpTabletPadGroupV2 &
    operator=(ZwpTabletPadGroupV2 &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    auto &buttons() { return buttonsSignal_; }
    auto &ring() { return ringSignal_; }
    auto &strip() { return stripSignal_; }
    auto &modes() { return modesSignal_; }
    auto &done() { return doneSignal_; }
    auto &modeSwitch() { return modeSwitchSignal_; }

private:
    static void destructor(zwp_tablet_pad_group_v2 *);
    static const struct zwp_tablet_pad_group_v2_listener listener;
    fcitx::Signal<void(wl_array *)> buttonsSignal_;
    fcitx::Signal<void(ZwpTabletPadRingV2 *)> ringSignal_;
    fcitx::Signal<void(ZwpTabletPadStripV2 *)> stripSignal_;
    fcitx::Signal<void(uint32_t)> modesSignal_;
    fcitx::Signal<void()> doneSignal_;
    fcitx::Signal<void(uint32_t, uint32_t, uint32_t)> modeSwitchSignal_;
    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<zwp_tablet_pad_group_v2, &destructor> data_;
};
static inline zwp_tablet_pad_group_v2 *rawPointer(ZwpTabletPadGroupV2 *p) {
    return p ? static_cast<zwp_tablet_pad_group_v2 *>(*p) : nullptr;
}
} // namespace fcitx::wayland
#endif
