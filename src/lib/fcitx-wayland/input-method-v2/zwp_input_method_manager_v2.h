#ifndef ZWP_INPUT_METHOD_MANAGER_V2_H_
#define ZWP_INPUT_METHOD_MANAGER_V2_H_
#include <cstdint>
#include <wayland-client.h>
#include <wayland-util.h>
#include "fcitx-utils/misc.h"
#include "wayland-input-method-unstable-v2-client-protocol.h" // IWYU pragma: export
namespace fcitx::wayland {

class WlSeat;
class ZwpInputMethodV2;

class ZwpInputMethodManagerV2 final {
public:
    static constexpr const char *interface = "zwp_input_method_manager_v2";
    static constexpr const wl_interface *const wlInterface =
        &zwp_input_method_manager_v2_interface;
    static constexpr const uint32_t version = 1;
    using wlType = zwp_input_method_manager_v2;
    operator zwp_input_method_manager_v2 *() { return data_.get(); }
    ZwpInputMethodManagerV2(wlType *data);
    ZwpInputMethodManagerV2(ZwpInputMethodManagerV2 &&other) noexcept = delete;
    ZwpInputMethodManagerV2 &
    operator=(ZwpInputMethodManagerV2 &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    ZwpInputMethodV2 *getInputMethod(WlSeat *seat);

private:
    static void destructor(zwp_input_method_manager_v2 *);

    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<zwp_input_method_manager_v2, &destructor> data_;
};
static inline zwp_input_method_manager_v2 *
rawPointer(ZwpInputMethodManagerV2 *p) {
    return p ? static_cast<zwp_input_method_manager_v2 *>(*p) : nullptr;
}

} // namespace fcitx::wayland

#endif // ZWP_INPUT_METHOD_MANAGER_V2_H_
