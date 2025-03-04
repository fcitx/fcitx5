#ifndef WL_BUFFER_H_
#define WL_BUFFER_H_
#include <cstdint>
#include <wayland-client.h>
#include <wayland-util.h>
#include "fcitx-utils/misc.h"
#include "fcitx-utils/signals.h"
namespace fcitx::wayland {

class WlBuffer final {
public:
    static constexpr const char *interface = "wl_buffer";
    static constexpr const wl_interface *const wlInterface =
        &wl_buffer_interface;
    static constexpr const uint32_t version = 1;
    using wlType = wl_buffer;
    operator wl_buffer *() { return data_.get(); }
    WlBuffer(wlType *data);
    WlBuffer(WlBuffer &&other) noexcept = delete;
    WlBuffer &operator=(WlBuffer &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }

    auto &release() { return releaseSignal_; }

private:
    static void destructor(wl_buffer *);
    static const struct wl_buffer_listener listener;
    fcitx::Signal<void()> releaseSignal_;

    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<wl_buffer, &destructor> data_;
};
static inline wl_buffer *rawPointer(WlBuffer *p) {
    return p ? static_cast<wl_buffer *>(*p) : nullptr;
}

} // namespace fcitx::wayland

#endif // WL_BUFFER_H_
