#ifndef ZWP_VIRTUAL_KEYBOARD_V1
#define ZWP_VIRTUAL_KEYBOARD_V1
#include <memory>
#include <wayland-client.h>
#include "fcitx-utils/signals.h"
#include "wayland-virtual-keyboard-unstable-v1-client-protocol.h"
namespace fcitx {
namespace wayland {
class ZwpVirtualKeyboardV1 final {
public:
    static constexpr const char *interface = "zwp_virtual_keyboard_v1";
    static constexpr const wl_interface *const wlInterface =
        &zwp_virtual_keyboard_v1_interface;
    static constexpr const uint32_t version = 1;
    typedef zwp_virtual_keyboard_v1 wlType;
    operator zwp_virtual_keyboard_v1 *() { return data_.get(); }
    ZwpVirtualKeyboardV1(wlType *data);
    ZwpVirtualKeyboardV1(ZwpVirtualKeyboardV1 &&other) noexcept = delete;
    ZwpVirtualKeyboardV1 &
    operator=(ZwpVirtualKeyboardV1 &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    void keymap(uint32_t format, int32_t fd, uint32_t size);
    void key(uint32_t time, uint32_t key, uint32_t state);
    void modifiers(uint32_t modsDepressed, uint32_t modsLatched,
                   uint32_t modsLocked, uint32_t group);

private:
    static void destructor(zwp_virtual_keyboard_v1 *);
    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<zwp_virtual_keyboard_v1, &destructor> data_;
};
static inline zwp_virtual_keyboard_v1 *rawPointer(ZwpVirtualKeyboardV1 *p) {
    return p ? static_cast<zwp_virtual_keyboard_v1 *>(*p) : nullptr;
}
} // namespace wayland
} // namespace fcitx
#endif
