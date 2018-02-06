#ifndef WL_KEYBOARD
#define WL_KEYBOARD
#include "fcitx-utils/signals.h"
#include <memory>
#include <wayland-client.h>
namespace fcitx {
namespace wayland {
class WlSurface;
class WlKeyboard final {
public:
    static constexpr const char *interface = "wl_keyboard";
    static constexpr const wl_interface *const wlInterface =
        &wl_keyboard_interface;
    static constexpr const uint32_t version = 5;
    typedef wl_keyboard wlType;
    operator wl_keyboard *() { return data_.get(); }
    WlKeyboard(wlType *data);
    WlKeyboard(WlKeyboard &&other) noexcept = delete;
    WlKeyboard &operator=(WlKeyboard &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
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
    std::unique_ptr<wl_keyboard, decltype(&destructor)> data_;
};
static inline wl_keyboard *rawPointer(WlKeyboard *p) {
    return p ? static_cast<wl_keyboard *>(*p) : nullptr;
}
} // namespace wayland
} // namespace fcitx
#endif
