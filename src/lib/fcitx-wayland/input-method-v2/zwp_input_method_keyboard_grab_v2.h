#ifndef ZWP_INPUT_METHOD_KEYBOARD_GRAB_V2
#define ZWP_INPUT_METHOD_KEYBOARD_GRAB_V2
#include <memory>
#include <wayland-client.h>
#include "fcitx-utils/signals.h"
#include "wayland-input-method-unstable-v2-client-protocol.h"
namespace fcitx::wayland {
class ZwpInputMethodKeyboardGrabV2 final {
public:
    static constexpr const char *interface =
        "zwp_input_method_keyboard_grab_v2";
    static constexpr const wl_interface *const wlInterface =
        &zwp_input_method_keyboard_grab_v2_interface;
    static constexpr const uint32_t version = 1;
    typedef zwp_input_method_keyboard_grab_v2 wlType;
    operator zwp_input_method_keyboard_grab_v2 *() { return data_.get(); }
    ZwpInputMethodKeyboardGrabV2(wlType *data);
    ZwpInputMethodKeyboardGrabV2(
        ZwpInputMethodKeyboardGrabV2 &&other) noexcept = delete;
    ZwpInputMethodKeyboardGrabV2 &
    operator=(ZwpInputMethodKeyboardGrabV2 &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    auto &keymap() { return keymapSignal_; }
    auto &key() { return keySignal_; }
    auto &modifiers() { return modifiersSignal_; }
    auto &repeatInfo() { return repeatInfoSignal_; }

private:
    static void destructor(zwp_input_method_keyboard_grab_v2 *);
    static const struct zwp_input_method_keyboard_grab_v2_listener listener;
    fcitx::Signal<void(uint32_t, int32_t, uint32_t)> keymapSignal_;
    fcitx::Signal<void(uint32_t, uint32_t, uint32_t, uint32_t)> keySignal_;
    fcitx::Signal<void(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)>
        modifiersSignal_;
    fcitx::Signal<void(int32_t, int32_t)> repeatInfoSignal_;
    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<zwp_input_method_keyboard_grab_v2, &destructor> data_;
};
static inline zwp_input_method_keyboard_grab_v2 *
rawPointer(ZwpInputMethodKeyboardGrabV2 *p) {
    return p ? static_cast<zwp_input_method_keyboard_grab_v2 *>(*p) : nullptr;
}
} // namespace fcitx::wayland
#endif
