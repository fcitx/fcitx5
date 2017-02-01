#ifndef ZWP_INPUT_METHOD_V1
#define ZWP_INPUT_METHOD_V1
#include <wayland-client.h>
#include <memory>
#include "wayland-input-method-unstable-v1-client-protocol.h"
#include "fcitx-utils/signals.h"
namespace fcitx {
namespace wayland {
class ZwpInputMethodContextV1;
class ZwpInputMethodV1 {
public:
    static constexpr const char *interface = "zwp_input_method_v1";
    static constexpr const wl_interface *const wlInterface = &zwp_input_method_v1_interface;
    static constexpr const uint32_t version = 1;
    typedef zwp_input_method_v1 wlType;
    operator zwp_input_method_v1 *() { return data_.get(); }
    ZwpInputMethodV1(wlType *data);
    ZwpInputMethodV1(ZwpInputMethodV1 &&other) : data_(std::move(other.data_)) {}
    ZwpInputMethodV1 &operator=(ZwpInputMethodV1 &&other) {
        data_ = std::move(other.data_);
        return *this;
    }
    auto actualVersion() const { return version_; }
    auto &activate() { return activateSignal_; }
    auto &deactivate() { return deactivateSignal_; }
private:
    static void destructor(zwp_input_method_v1 *);
    static const struct zwp_input_method_v1_listener listener;
    fcitx::Signal<void(ZwpInputMethodContextV1 *)> activateSignal_;
    fcitx::Signal<void(ZwpInputMethodContextV1 *)> deactivateSignal_;
    uint32_t version_;
    std::unique_ptr<zwp_input_method_v1, decltype(&destructor)> data_;
};
}
}
#endif
