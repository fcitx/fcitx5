#ifndef ZWP_INPUT_CONTEXT_V3
#define ZWP_INPUT_CONTEXT_V3
#include <memory>
#include <wayland-client.h>
#include "fcitx-utils/signals.h"
#include "wayland-input-method-unstable-v3-client-protocol.h"
namespace fcitx::wayland {
class ZwpInputContextV3 final {
public:
    static constexpr const char *interface = "zwp_input_context_v3";
    static constexpr const wl_interface *const wlInterface =
        &zwp_input_context_v3_interface;
    static constexpr const uint32_t version = 1;
    typedef zwp_input_context_v3 wlType;
    operator zwp_input_context_v3 *() { return data_.get(); }
    ZwpInputContextV3(wlType *data);
    ZwpInputContextV3(ZwpInputContextV3 &&other) noexcept = delete;
    ZwpInputContextV3 &operator=(ZwpInputContextV3 &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    void commitString(const char *text);
    void setPreeditString(const char *text, int32_t cursorBegin,
                          int32_t cursorEnd);
    void deleteSurroundingText(uint32_t beforeLength, uint32_t afterLength);
    void commit(uint32_t serial);
    auto &surroundingText() { return surroundingTextSignal_; }
    auto &textChangeCause() { return textChangeCauseSignal_; }
    auto &contentType() { return contentTypeSignal_; }

private:
    static void destructor(zwp_input_context_v3 *);
    static const struct zwp_input_context_v3_listener listener;
    fcitx::Signal<void(const char *, uint32_t, uint32_t)>
        surroundingTextSignal_;
    fcitx::Signal<void(uint32_t)> textChangeCauseSignal_;
    fcitx::Signal<void(uint32_t, uint32_t)> contentTypeSignal_;
    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<zwp_input_context_v3, &destructor> data_;
};
static inline zwp_input_context_v3 *rawPointer(ZwpInputContextV3 *p) {
    return p ? static_cast<zwp_input_context_v3 *>(*p) : nullptr;
}
} // namespace fcitx::wayland
#endif
