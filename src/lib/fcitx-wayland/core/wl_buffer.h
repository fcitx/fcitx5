#ifndef WL_BUFFER
#define WL_BUFFER
#include "fcitx-utils/signals.h"
#include <memory>
#include <wayland-client.h>
namespace fcitx {
namespace wayland {
class WlBuffer final {
public:
    static constexpr const char *interface = "wl_buffer";
    static constexpr const wl_interface *const wlInterface =
        &wl_buffer_interface;
    static constexpr const uint32_t version = 1;
    typedef wl_buffer wlType;
    operator wl_buffer *() { return data_.get(); }
    WlBuffer(wlType *data);
    WlBuffer(WlBuffer &&other) noexcept = default;
    WlBuffer &operator=(WlBuffer &&other) noexcept = default;
    auto actualVersion() const { return version_; }
    auto &release() { return releaseSignal_; }

private:
    static void destructor(wl_buffer *);
    static const struct wl_buffer_listener listener;
    fcitx::Signal<void()> releaseSignal_;
    uint32_t version_;
    std::unique_ptr<wl_buffer, decltype(&destructor)> data_;
};
}
}
#endif
