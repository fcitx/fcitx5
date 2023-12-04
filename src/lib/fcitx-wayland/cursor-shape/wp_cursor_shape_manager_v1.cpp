#include "wp_cursor_shape_manager_v1.h"
#include <cassert>
#include "wl_pointer.h"
#include "wp_cursor_shape_device_v1.h"
#include "zwp_tablet_tool_v2.h"
namespace fcitx::wayland {
WpCursorShapeManagerV1::WpCursorShapeManagerV1(wp_cursor_shape_manager_v1 *data)
    : version_(wp_cursor_shape_manager_v1_get_version(data)), data_(data) {
    wp_cursor_shape_manager_v1_set_user_data(*this, this);
}
void WpCursorShapeManagerV1::destructor(wp_cursor_shape_manager_v1 *data) {
    auto version = wp_cursor_shape_manager_v1_get_version(data);
    if (version >= 1) {
        return wp_cursor_shape_manager_v1_destroy(data);
    }
}
WpCursorShapeDeviceV1 *WpCursorShapeManagerV1::getPointer(WlPointer *pointer) {
    return new WpCursorShapeDeviceV1(
        wp_cursor_shape_manager_v1_get_pointer(*this, rawPointer(pointer)));
}
WpCursorShapeDeviceV1 *
WpCursorShapeManagerV1::getTabletToolV2(ZwpTabletToolV2 *tabletTool) {
    return new WpCursorShapeDeviceV1(
        wp_cursor_shape_manager_v1_get_tablet_tool_v2(*this,
                                                      rawPointer(tabletTool)));
}
} // namespace fcitx::wayland
