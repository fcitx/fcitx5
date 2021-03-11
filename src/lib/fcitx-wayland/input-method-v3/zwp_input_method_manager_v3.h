#ifndef ZWP_INPUT_METHOD_MANAGER_V3
#define ZWP_INPUT_METHOD_MANAGER_V3
#include <memory>
#include <wayland-client.h>
#include "fcitx-utils/signals.h"
#include "wayland-input-method-unstable-v3-client-protocol.h"
namespace fcitx::wayland {
class WlSeat;
class ZwpInputMethodV3;
class ZwpInputMethodManagerV3 final {
public:
    static constexpr const char *interface = "zwp_input_method_manager_v3";
    static constexpr const wl_interface *const wlInterface =
        &zwp_input_method_manager_v3_interface;
    static constexpr const uint32_t version = 1;
    typedef zwp_input_method_manager_v3 wlType;
    operator zwp_input_method_manager_v3 *() { return data_.get(); }
    ZwpInputMethodManagerV3(wlType *data);
    ZwpInputMethodManagerV3(ZwpInputMethodManagerV3 &&other) noexcept = delete;
    ZwpInputMethodManagerV3 &
    operator=(ZwpInputMethodManagerV3 &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    ZwpInputMethodV3 *getInputMethod(WlSeat *seat);

private:
    static void destructor(zwp_input_method_manager_v3 *);
    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<zwp_input_method_manager_v3, &destructor> data_;
};
static inline zwp_input_method_manager_v3 *
rawPointer(ZwpInputMethodManagerV3 *p) {
    return p ? static_cast<zwp_input_method_manager_v3 *>(*p) : nullptr;
}
} // namespace fcitx::wayland
#endif
