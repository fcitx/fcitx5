#ifndef WL_CALLBACK
#define WL_CALLBACK
#include <memory>
#include <wayland-client.h>
#include "fcitx-utils/signals.h"
namespace fcitx {
namespace wayland {
class WlCallback final {
public:
    static constexpr const char *interface = "wl_callback";
    static constexpr const wl_interface *const wlInterface =
        &wl_callback_interface;
    static constexpr const uint32_t version = 1;
    typedef wl_callback wlType;
    operator wl_callback *() { return data_.get(); }
    WlCallback(wlType *data);
    WlCallback(WlCallback &&other) noexcept = delete;
    WlCallback &operator=(WlCallback &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    auto &done() { return doneSignal_; }

private:
    static void destructor(wl_callback *);
    static const struct wl_callback_listener listener;
    fcitx::Signal<void(uint32_t)> doneSignal_;
    uint32_t version_;
    void *userData_ = nullptr;
    std::unique_ptr<wl_callback, decltype(&destructor)> data_;
};
static inline wl_callback *rawPointer(WlCallback *p) {
    return p ? static_cast<wl_callback *>(*p) : nullptr;
}
} // namespace wayland
} // namespace fcitx
#endif
