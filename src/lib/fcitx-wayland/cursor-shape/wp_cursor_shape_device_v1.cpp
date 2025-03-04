#include "wp_cursor_shape_device_v1.h"
#include "wayland-cursor-shape-v1-client-protocol.h"

namespace fcitx::wayland {

WpCursorShapeDeviceV1::WpCursorShapeDeviceV1(wp_cursor_shape_device_v1 *data)
    : version_(wp_cursor_shape_device_v1_get_version(data)), data_(data) {
    wp_cursor_shape_device_v1_set_user_data(*this, this);
}

void WpCursorShapeDeviceV1::destructor(wp_cursor_shape_device_v1 *data) {
    const auto version = wp_cursor_shape_device_v1_get_version(data);
    if (version >= 1) {
        wp_cursor_shape_device_v1_destroy(data);
        return;
    }
}
void WpCursorShapeDeviceV1::setShape(uint32_t serial, uint32_t shape) {
    wp_cursor_shape_device_v1_set_shape(*this, serial, shape);
}

} // namespace fcitx::wayland
