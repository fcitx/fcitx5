#ifndef ZWP_INPUT_METHOD_V2
#define ZWP_INPUT_METHOD_V2
#include <wayland-client.h>
#include "fcitx-utils/signals.h"
#include "wayland-input-method-unstable-v2-client-protocol.h"
namespace fcitx::wayland {
class WlSurface;
class ZwpInputMethodKeyboardGrabV2;
class ZwpInputPopupSurfaceV2;
class ZwpInputMethodV2 final {
public:
    static constexpr const char *interface = "zwp_input_method_v2";
    static constexpr const wl_interface *const wlInterface =
        &zwp_input_method_v2_interface;
    static constexpr const uint32_t version = 1;
    typedef zwp_input_method_v2 wlType;
    operator zwp_input_method_v2 *() { return data_.get(); }
    ZwpInputMethodV2(wlType *data);
    ZwpInputMethodV2(ZwpInputMethodV2 &&other) noexcept = delete;
    ZwpInputMethodV2 &operator=(ZwpInputMethodV2 &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    void commitString(const char *text);
    void setPreeditString(const char *text, int32_t cursorBegin,
                          int32_t cursorEnd);
    void deleteSurroundingText(uint32_t beforeLength, uint32_t afterLength);
    void commit(uint32_t serial);
    ZwpInputPopupSurfaceV2 *getInputPopupSurface(WlSurface *surface);
    ZwpInputMethodKeyboardGrabV2 *grabKeyboard();
    auto &activate() { return activateSignal_; }
    auto &deactivate() { return deactivateSignal_; }
    auto &surroundingText() { return surroundingTextSignal_; }
    auto &textChangeCause() { return textChangeCauseSignal_; }
    auto &contentType() { return contentTypeSignal_; }
    auto &done() { return doneSignal_; }
    auto &unavailable() { return unavailableSignal_; }

private:
    static void destructor(zwp_input_method_v2 *);
    static const struct zwp_input_method_v2_listener listener;
    fcitx::Signal<void()> activateSignal_;
    fcitx::Signal<void()> deactivateSignal_;
    fcitx::Signal<void(const char *, uint32_t, uint32_t)>
        surroundingTextSignal_;
    fcitx::Signal<void(uint32_t)> textChangeCauseSignal_;
    fcitx::Signal<void(uint32_t, uint32_t)> contentTypeSignal_;
    fcitx::Signal<void()> doneSignal_;
    fcitx::Signal<void()> unavailableSignal_;
    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<zwp_input_method_v2, &destructor> data_;
};
static inline zwp_input_method_v2 *rawPointer(ZwpInputMethodV2 *p) {
    return p ? static_cast<zwp_input_method_v2 *>(*p) : nullptr;
}
} // namespace fcitx::wayland
#endif
