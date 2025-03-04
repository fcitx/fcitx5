#ifndef WP_VIEWPORTER_H_
#define WP_VIEWPORTER_H_
#include <cstdint>
#include <wayland-client.h>
#include <wayland-util.h>
#include "fcitx-utils/misc.h"
#include "wayland-viewporter-client-protocol.h" // IWYU pragma: export
namespace fcitx::wayland {

class WlSurface;
class WpViewport;

class WpViewporter final {
public:
    static constexpr const char *interface = "wp_viewporter";
    static constexpr const wl_interface *const wlInterface =
        &wp_viewporter_interface;
    static constexpr const uint32_t version = 1;
    using wlType = wp_viewporter;
    operator wp_viewporter *() { return data_.get(); }
    WpViewporter(wlType *data);
    WpViewporter(WpViewporter &&other) noexcept = delete;
    WpViewporter &operator=(WpViewporter &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    WpViewport *getViewport(WlSurface *surface);

private:
    static void destructor(wp_viewporter *);

    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<wp_viewporter, &destructor> data_;
};
static inline wp_viewporter *rawPointer(WpViewporter *p) {
    return p ? static_cast<wp_viewporter *>(*p) : nullptr;
}

} // namespace fcitx::wayland

#endif // WP_VIEWPORTER_H_
