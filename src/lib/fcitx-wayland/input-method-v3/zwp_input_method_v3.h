#ifndef ZWP_INPUT_METHOD_V3
#define ZWP_INPUT_METHOD_V3
#include <memory>
#include <wayland-client.h>
#include "fcitx-utils/signals.h"
#include "wayland-input-method-unstable-v3-client-protocol.h"
namespace fcitx::wayland {
class WlSurface;
class ZwpInputMethodKeyboardGrabV3;
class ZwpInputPopupSurfaceV3;
class ZwpInputMethodV3 final {
public:
    static constexpr const char *interface = "zwp_input_method_v3";
    static constexpr const wl_interface *const wlInterface =
        &zwp_input_method_v3_interface;
    static constexpr const uint32_t version = 1;
    typedef zwp_input_method_v3 wlType;
    operator zwp_input_method_v3 *() { return data_.get(); }
    ZwpInputMethodV3(wlType *data);
    ZwpInputMethodV3(ZwpInputMethodV3 &&other) noexcept = delete;
    ZwpInputMethodV3 &operator=(ZwpInputMethodV3 &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    ZwpInputPopupSurfaceV3 *getInputPopupSurface(WlSurface *surface);
    ZwpInputMethodKeyboardGrabV3 *grabKeyboard();
    auto &inputContext() { return inputContextSignal_; }
    auto &inputContextRemove() { return inputContextRemoveSignal_; }
    auto &unavailable() { return unavailableSignal_; }

private:
    static void destructor(zwp_input_method_v3 *);
    static const struct zwp_input_method_v3_listener listener;
    fcitx::Signal<void(uint32_t, uint32_t)> inputContextSignal_;
    fcitx::Signal<void()> inputContextRemoveSignal_;
    fcitx::Signal<void()> unavailableSignal_;
    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<zwp_input_method_v3, &destructor> data_;
};
static inline zwp_input_method_v3 *rawPointer(ZwpInputMethodV3 *p) {
    return p ? static_cast<zwp_input_method_v3 *>(*p) : nullptr;
}
} // namespace fcitx::wayland
#endif
