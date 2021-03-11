#ifndef ZWP_INPUT_METHOD_KEYBOARD_GRAB_V3
#define ZWP_INPUT_METHOD_KEYBOARD_GRAB_V3
#include <memory>
#include <wayland-client.h>
#include "fcitx-utils/signals.h"
#include "wayland-input-method-unstable-v3-client-protocol.h"
namespace fcitx::wayland {
class ZwpInputMethodKeyboardGrabV3 final {
public:
    static constexpr const char *interface =
        "zwp_input_method_keyboard_grab_v3";
    static constexpr const wl_interface *const wlInterface =
        &zwp_input_method_keyboard_grab_v3_interface;
    static constexpr const uint32_t version = 1;
    typedef zwp_input_method_keyboard_grab_v3 wlType;
    operator zwp_input_method_keyboard_grab_v3 *() { return data_.get(); }
    ZwpInputMethodKeyboardGrabV3(wlType *data);
    ZwpInputMethodKeyboardGrabV3(
        ZwpInputMethodKeyboardGrabV3 &&other) noexcept = delete;
    ZwpInputMethodKeyboardGrabV3 &
    operator=(ZwpInputMethodKeyboardGrabV3 &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    auto &keymap() { return keymapSignal_; }
    auto &key() { return keySignal_; }
    auto &modifiers() { return modifiersSignal_; }
    auto &repeatInfo() { return repeatInfoSignal_; }

private:
    static void destructor(zwp_input_method_keyboard_grab_v3 *);
    static const struct zwp_input_method_keyboard_grab_v3_listener listener;
    fcitx::Signal<void(uint32_t, int32_t, uint32_t)> keymapSignal_;
    fcitx::Signal<void(uint32_t, uint32_t, uint32_t, uint32_t)> keySignal_;
    fcitx::Signal<void(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)>
        modifiersSignal_;
    fcitx::Signal<void(int32_t, int32_t)> repeatInfoSignal_;
    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<zwp_input_method_keyboard_grab_v3, &destructor> data_;
};
static inline zwp_input_method_keyboard_grab_v3 *
rawPointer(ZwpInputMethodKeyboardGrabV3 *p) {
    return p ? static_cast<zwp_input_method_keyboard_grab_v3 *>(*p) : nullptr;
}
} // namespace fcitx::wayland
#endif
