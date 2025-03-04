#ifndef ZWP_INPUT_METHOD_V1_H_
#define ZWP_INPUT_METHOD_V1_H_
#include <cstdint>
#include <wayland-client.h>
#include <wayland-util.h>
#include "fcitx-utils/misc.h"
#include "fcitx-utils/signals.h"
#include "wayland-input-method-unstable-v1-client-protocol.h" // IWYU pragma: export
namespace fcitx::wayland {

class ZwpInputMethodContextV1;

class ZwpInputMethodV1 final {
public:
    static constexpr const char *interface = "zwp_input_method_v1";
    static constexpr const wl_interface *const wlInterface =
        &zwp_input_method_v1_interface;
    static constexpr const uint32_t version = 1;
    using wlType = zwp_input_method_v1;
    operator zwp_input_method_v1 *() { return data_.get(); }
    ZwpInputMethodV1(wlType *data);
    ZwpInputMethodV1(ZwpInputMethodV1 &&other) noexcept = delete;
    ZwpInputMethodV1 &operator=(ZwpInputMethodV1 &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }

    auto &activate() { return activateSignal_; }
    auto &deactivate() { return deactivateSignal_; }

private:
    static void destructor(zwp_input_method_v1 *);
    static const struct zwp_input_method_v1_listener listener;
    fcitx::Signal<void(ZwpInputMethodContextV1 *)> activateSignal_;
    fcitx::Signal<void(ZwpInputMethodContextV1 *)> deactivateSignal_;

    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<zwp_input_method_v1, &destructor> data_;
};
static inline zwp_input_method_v1 *rawPointer(ZwpInputMethodV1 *p) {
    return p ? static_cast<zwp_input_method_v1 *>(*p) : nullptr;
}

} // namespace fcitx::wayland

#endif // ZWP_INPUT_METHOD_V1_H_
