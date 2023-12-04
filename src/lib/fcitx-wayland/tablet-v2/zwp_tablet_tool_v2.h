#ifndef ZWP_TABLET_TOOL_V2
#define ZWP_TABLET_TOOL_V2
#include <memory>
#include <wayland-client.h>
#include "fcitx-utils/signals.h"
#include "wayland-tablet-client-protocol.h"
namespace fcitx::wayland {
class WlSurface;
class ZwpTabletV2;
class ZwpTabletToolV2 final {
public:
    static constexpr const char *interface = "zwp_tablet_tool_v2";
    static constexpr const wl_interface *const wlInterface =
        &zwp_tablet_tool_v2_interface;
    static constexpr const uint32_t version = 1;
    typedef zwp_tablet_tool_v2 wlType;
    operator zwp_tablet_tool_v2 *() { return data_.get(); }
    ZwpTabletToolV2(wlType *data);
    ZwpTabletToolV2(ZwpTabletToolV2 &&other) noexcept = delete;
    ZwpTabletToolV2 &operator=(ZwpTabletToolV2 &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    void setCursor(uint32_t serial, WlSurface *surface, int32_t hotspotX,
                   int32_t hotspotY);
    auto &type() { return typeSignal_; }
    auto &hardwareSerial() { return hardwareSerialSignal_; }
    auto &hardwareIdWacom() { return hardwareIdWacomSignal_; }
    auto &capability() { return capabilitySignal_; }
    auto &done() { return doneSignal_; }
    auto &removed() { return removedSignal_; }
    auto &proximityIn() { return proximityInSignal_; }
    auto &proximityOut() { return proximityOutSignal_; }
    auto &down() { return downSignal_; }
    auto &up() { return upSignal_; }
    auto &motion() { return motionSignal_; }
    auto &pressure() { return pressureSignal_; }
    auto &distance() { return distanceSignal_; }
    auto &tilt() { return tiltSignal_; }
    auto &rotation() { return rotationSignal_; }
    auto &slider() { return sliderSignal_; }
    auto &wheel() { return wheelSignal_; }
    auto &button() { return buttonSignal_; }
    auto &frame() { return frameSignal_; }

private:
    static void destructor(zwp_tablet_tool_v2 *);
    static const struct zwp_tablet_tool_v2_listener listener;
    fcitx::Signal<void(uint32_t)> typeSignal_;
    fcitx::Signal<void(uint32_t, uint32_t)> hardwareSerialSignal_;
    fcitx::Signal<void(uint32_t, uint32_t)> hardwareIdWacomSignal_;
    fcitx::Signal<void(uint32_t)> capabilitySignal_;
    fcitx::Signal<void()> doneSignal_;
    fcitx::Signal<void()> removedSignal_;
    fcitx::Signal<void(uint32_t, ZwpTabletV2 *, WlSurface *)>
        proximityInSignal_;
    fcitx::Signal<void()> proximityOutSignal_;
    fcitx::Signal<void(uint32_t)> downSignal_;
    fcitx::Signal<void()> upSignal_;
    fcitx::Signal<void(wl_fixed_t, wl_fixed_t)> motionSignal_;
    fcitx::Signal<void(uint32_t)> pressureSignal_;
    fcitx::Signal<void(uint32_t)> distanceSignal_;
    fcitx::Signal<void(wl_fixed_t, wl_fixed_t)> tiltSignal_;
    fcitx::Signal<void(wl_fixed_t)> rotationSignal_;
    fcitx::Signal<void(int32_t)> sliderSignal_;
    fcitx::Signal<void(wl_fixed_t, int32_t)> wheelSignal_;
    fcitx::Signal<void(uint32_t, uint32_t, uint32_t)> buttonSignal_;
    fcitx::Signal<void(uint32_t)> frameSignal_;
    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<zwp_tablet_tool_v2, &destructor> data_;
};
static inline zwp_tablet_tool_v2 *rawPointer(ZwpTabletToolV2 *p) {
    return p ? static_cast<zwp_tablet_tool_v2 *>(*p) : nullptr;
}
} // namespace fcitx::wayland
#endif
