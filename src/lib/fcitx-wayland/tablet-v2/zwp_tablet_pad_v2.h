#ifndef ZWP_TABLET_PAD_V2
#define ZWP_TABLET_PAD_V2
#include <wayland-client.h>
#include "fcitx-utils/signals.h"
#include "wayland-tablet-client-protocol.h"
namespace fcitx::wayland {
class WlSurface;
class ZwpTabletPadGroupV2;
class ZwpTabletV2;
class ZwpTabletPadV2 final {
public:
    static constexpr const char *interface = "zwp_tablet_pad_v2";
    static constexpr const wl_interface *const wlInterface =
        &zwp_tablet_pad_v2_interface;
    static constexpr const uint32_t version = 1;
    typedef zwp_tablet_pad_v2 wlType;
    operator zwp_tablet_pad_v2 *() { return data_.get(); }
    ZwpTabletPadV2(wlType *data);
    ZwpTabletPadV2(ZwpTabletPadV2 &&other) noexcept = delete;
    ZwpTabletPadV2 &operator=(ZwpTabletPadV2 &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    void setFeedback(uint32_t button, const char *description, uint32_t serial);
    auto &group() { return groupSignal_; }
    auto &path() { return pathSignal_; }
    auto &buttons() { return buttonsSignal_; }
    auto &done() { return doneSignal_; }
    auto &button() { return buttonSignal_; }
    auto &enter() { return enterSignal_; }
    auto &leave() { return leaveSignal_; }
    auto &removed() { return removedSignal_; }

private:
    static void destructor(zwp_tablet_pad_v2 *);
    static const struct zwp_tablet_pad_v2_listener listener;
    fcitx::Signal<void(ZwpTabletPadGroupV2 *)> groupSignal_;
    fcitx::Signal<void(const char *)> pathSignal_;
    fcitx::Signal<void(uint32_t)> buttonsSignal_;
    fcitx::Signal<void()> doneSignal_;
    fcitx::Signal<void(uint32_t, uint32_t, uint32_t)> buttonSignal_;
    fcitx::Signal<void(uint32_t, ZwpTabletV2 *, WlSurface *)> enterSignal_;
    fcitx::Signal<void(uint32_t, WlSurface *)> leaveSignal_;
    fcitx::Signal<void()> removedSignal_;
    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<zwp_tablet_pad_v2, &destructor> data_;
};
static inline zwp_tablet_pad_v2 *rawPointer(ZwpTabletPadV2 *p) {
    return p ? static_cast<zwp_tablet_pad_v2 *>(*p) : nullptr;
}
} // namespace fcitx::wayland
#endif
