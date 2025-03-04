#ifndef WP_CURSOR_SHAPE_DEVICE_V1_H_
#define WP_CURSOR_SHAPE_DEVICE_V1_H_
#include <cstdint>
#include <wayland-client.h>
#include <wayland-util.h>
#include "fcitx-utils/misc.h"
#include "wayland-cursor-shape-v1-client-protocol.h" // IWYU pragma: export
namespace fcitx::wayland {

class WpCursorShapeDeviceV1 final {
public:
    static constexpr const char *interface = "wp_cursor_shape_device_v1";
    static constexpr const wl_interface *const wlInterface =
        &wp_cursor_shape_device_v1_interface;
    static constexpr const uint32_t version = 1;
    using wlType = wp_cursor_shape_device_v1;
    operator wp_cursor_shape_device_v1 *() { return data_.get(); }
    WpCursorShapeDeviceV1(wlType *data);
    WpCursorShapeDeviceV1(WpCursorShapeDeviceV1 &&other) noexcept = delete;
    WpCursorShapeDeviceV1 &
    operator=(WpCursorShapeDeviceV1 &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    void setShape(uint32_t serial, uint32_t shape);

private:
    static void destructor(wp_cursor_shape_device_v1 *);

    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<wp_cursor_shape_device_v1, &destructor> data_;
};
static inline wp_cursor_shape_device_v1 *rawPointer(WpCursorShapeDeviceV1 *p) {
    return p ? static_cast<wp_cursor_shape_device_v1 *>(*p) : nullptr;
}

} // namespace fcitx::wayland

#endif // WP_CURSOR_SHAPE_DEVICE_V1_H_
