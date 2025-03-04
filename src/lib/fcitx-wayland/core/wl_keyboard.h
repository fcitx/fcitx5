#ifndef WL_KEYBOARD_H_
#define WL_KEYBOARD_H_
#include <cstdint>
#include <wayland-client.h>
#include <wayland-util.h>
#include "fcitx-utils/misc.h"
#include "fcitx-utils/signals.h"
namespace fcitx::wayland {

class WlSurface;

class WlKeyboard final {
public:
    static constexpr const char *interface = "wl_keyboard";
    static constexpr const wl_interface *const wlInterface =
        &wl_keyboard_interface;
    static constexpr const uint32_t version = 9;
    using wlType = wl_keyboard;
    operator wl_keyboard *() { return data_.get(); }
    WlKeyboard(wlType *data);
    WlKeyboard(WlKeyboard &&other) noexcept = delete;
    WlKeyboard &operator=(WlKeyboard &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }

    auto &keymap() { return keymapSignal_; }
    auto &enter() { return enterSignal_; }
    auto &leave() { return leaveSignal_; }
    auto &key() { return keySignal_; }
    auto &modifiers() { return modifiersSignal_; }
    auto &repeatInfo() { return repeatInfoSignal_; }

private:
    static void destructor(wl_keyboard *);
    static const struct wl_keyboard_listener listener;
    fcitx::Signal<void(uint32_t, int32_t, uint32_t)> keymapSignal_;
    fcitx::Signal<void(uint32_t, WlSurface *, wl_array *)> enterSignal_;
    fcitx::Signal<void(uint32_t, WlSurface *)> leaveSignal_;
    fcitx::Signal<void(uint32_t, uint32_t, uint32_t, uint32_t)> keySignal_;
    fcitx::Signal<void(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)>
        modifiersSignal_;
    fcitx::Signal<void(int32_t, int32_t)> repeatInfoSignal_;

    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<wl_keyboard, &destructor> data_;
};
static inline wl_keyboard *rawPointer(WlKeyboard *p) {
    return p ? static_cast<wl_keyboard *>(*p) : nullptr;
}

} // namespace fcitx::wayland

#endif // WL_KEYBOARD_H_
