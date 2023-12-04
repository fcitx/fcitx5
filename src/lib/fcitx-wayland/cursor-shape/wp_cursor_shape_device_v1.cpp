#include "wp_cursor_shape_device_v1.h"
#include <cassert>
namespace fcitx::wayland {
WpCursorShapeDeviceV1::WpCursorShapeDeviceV1(wp_cursor_shape_device_v1 *data)
    : version_(wp_cursor_shape_device_v1_get_version(data)), data_(data) {
    wp_cursor_shape_device_v1_set_user_data(*this, this);
}
void WpCursorShapeDeviceV1::destructor(wp_cursor_shape_device_v1 *data) {
    auto version = wp_cursor_shape_device_v1_get_version(data);
    if (version >= 1) {
        return wp_cursor_shape_device_v1_destroy(data);
    }
}
void WpCursorShapeDeviceV1::setShape(uint32_t serial, uint32_t shape) {
    return wp_cursor_shape_device_v1_set_shape(*this, serial, shape);
}
} // namespace fcitx::wayland
