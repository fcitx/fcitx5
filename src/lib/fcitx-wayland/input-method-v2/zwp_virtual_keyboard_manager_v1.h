#ifndef ZWP_VIRTUAL_KEYBOARD_MANAGER_V1
#define ZWP_VIRTUAL_KEYBOARD_MANAGER_V1
#include <memory>
#include <wayland-client.h>
#include "fcitx-utils/signals.h"
#include "wayland-virtual-keyboard-unstable-v1-client-protocol.h"
namespace fcitx::wayland {
class WlSeat;
class ZwpVirtualKeyboardV1;
class ZwpVirtualKeyboardManagerV1 final {
public:
    static constexpr const char *interface = "zwp_virtual_keyboard_manager_v1";
    static constexpr const wl_interface *const wlInterface =
        &zwp_virtual_keyboard_manager_v1_interface;
    static constexpr const uint32_t version = 1;
    typedef zwp_virtual_keyboard_manager_v1 wlType;
    operator zwp_virtual_keyboard_manager_v1 *() { return data_.get(); }
    ZwpVirtualKeyboardManagerV1(wlType *data);
    ZwpVirtualKeyboardManagerV1(ZwpVirtualKeyboardManagerV1 &&other) noexcept =
        delete;
    ZwpVirtualKeyboardManagerV1 &
    operator=(ZwpVirtualKeyboardManagerV1 &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    ZwpVirtualKeyboardV1 *createVirtualKeyboard(WlSeat *seat);

private:
    static void destructor(zwp_virtual_keyboard_manager_v1 *);
    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<zwp_virtual_keyboard_manager_v1, &destructor> data_;
};
static inline zwp_virtual_keyboard_manager_v1 *
rawPointer(ZwpVirtualKeyboardManagerV1 *p) {
    return p ? static_cast<zwp_virtual_keyboard_manager_v1 *>(*p) : nullptr;
}
} // namespace fcitx::wayland
#endif
