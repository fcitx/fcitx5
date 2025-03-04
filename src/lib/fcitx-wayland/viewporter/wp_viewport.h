#ifndef WP_VIEWPORT_H_
#define WP_VIEWPORT_H_
#include <cstdint>
#include <wayland-client.h>
#include <wayland-util.h>
#include "fcitx-utils/misc.h"
#include "wayland-viewporter-client-protocol.h" // IWYU pragma: export
namespace fcitx::wayland {

class WpViewport final {
public:
    static constexpr const char *interface = "wp_viewport";
    static constexpr const wl_interface *const wlInterface =
        &wp_viewport_interface;
    static constexpr const uint32_t version = 1;
    using wlType = wp_viewport;
    operator wp_viewport *() { return data_.get(); }
    WpViewport(wlType *data);
    WpViewport(WpViewport &&other) noexcept = delete;
    WpViewport &operator=(WpViewport &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    void setSource(wl_fixed_t x, wl_fixed_t y, wl_fixed_t width,
                   wl_fixed_t height);
    void setDestination(int32_t width, int32_t height);

private:
    static void destructor(wp_viewport *);

    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<wp_viewport, &destructor> data_;
};
static inline wp_viewport *rawPointer(WpViewport *p) {
    return p ? static_cast<wp_viewport *>(*p) : nullptr;
}

} // namespace fcitx::wayland

#endif // WP_VIEWPORT_H_
