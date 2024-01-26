#ifndef ZWP_TABLET_PAD_STRIP_V2
#define ZWP_TABLET_PAD_STRIP_V2
#include <wayland-client.h>
#include "fcitx-utils/signals.h"
#include "wayland-tablet-client-protocol.h"
namespace fcitx::wayland {
class ZwpTabletPadStripV2 final {
public:
    static constexpr const char *interface = "zwp_tablet_pad_strip_v2";
    static constexpr const wl_interface *const wlInterface =
        &zwp_tablet_pad_strip_v2_interface;
    static constexpr const uint32_t version = 1;
    typedef zwp_tablet_pad_strip_v2 wlType;
    operator zwp_tablet_pad_strip_v2 *() { return data_.get(); }
    ZwpTabletPadStripV2(wlType *data);
    ZwpTabletPadStripV2(ZwpTabletPadStripV2 &&other) noexcept = delete;
    ZwpTabletPadStripV2 &
    operator=(ZwpTabletPadStripV2 &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    void setFeedback(const char *description, uint32_t serial);
    auto &source() { return sourceSignal_; }
    auto &position() { return positionSignal_; }
    auto &stop() { return stopSignal_; }
    auto &frame() { return frameSignal_; }

private:
    static void destructor(zwp_tablet_pad_strip_v2 *);
    static const struct zwp_tablet_pad_strip_v2_listener listener;
    fcitx::Signal<void(uint32_t)> sourceSignal_;
    fcitx::Signal<void(uint32_t)> positionSignal_;
    fcitx::Signal<void()> stopSignal_;
    fcitx::Signal<void(uint32_t)> frameSignal_;
    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<zwp_tablet_pad_strip_v2, &destructor> data_;
};
static inline zwp_tablet_pad_strip_v2 *rawPointer(ZwpTabletPadStripV2 *p) {
    return p ? static_cast<zwp_tablet_pad_strip_v2 *>(*p) : nullptr;
}
} // namespace fcitx::wayland
#endif
