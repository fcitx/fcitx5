#ifndef WP_FRACTIONAL_SCALE_MANAGER_V1_H_
#define WP_FRACTIONAL_SCALE_MANAGER_V1_H_
#include <cstdint>
#include <wayland-client.h>
#include <wayland-util.h>
#include "fcitx-utils/misc.h"
#include "wayland-fractional-scale-v1-client-protocol.h" // IWYU pragma: export
namespace fcitx::wayland {

class WlSurface;
class WpFractionalScaleV1;

class WpFractionalScaleManagerV1 final {
public:
    static constexpr const char *interface = "wp_fractional_scale_manager_v1";
    static constexpr const wl_interface *const wlInterface =
        &wp_fractional_scale_manager_v1_interface;
    static constexpr const uint32_t version = 1;
    using wlType = wp_fractional_scale_manager_v1;
    operator wp_fractional_scale_manager_v1 *() { return data_.get(); }
    WpFractionalScaleManagerV1(wlType *data);
    WpFractionalScaleManagerV1(WpFractionalScaleManagerV1 &&other) noexcept =
        delete;
    WpFractionalScaleManagerV1 &
    operator=(WpFractionalScaleManagerV1 &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    WpFractionalScaleV1 *getFractionalScale(WlSurface *surface);

private:
    static void destructor(wp_fractional_scale_manager_v1 *);

    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<wp_fractional_scale_manager_v1, &destructor> data_;
};
static inline wp_fractional_scale_manager_v1 *
rawPointer(WpFractionalScaleManagerV1 *p) {
    return p ? static_cast<wp_fractional_scale_manager_v1 *>(*p) : nullptr;
}

} // namespace fcitx::wayland

#endif // WP_FRACTIONAL_SCALE_MANAGER_V1_H_
