#ifndef WP_CURSOR_SHAPE_MANAGER_V1
#define WP_CURSOR_SHAPE_MANAGER_V1
#include <wayland-client.h>
#include "fcitx-utils/signals.h"
#include "wayland-cursor-shape-client-protocol.h"
namespace fcitx::wayland {
class WlPointer;
class WpCursorShapeDeviceV1;
class ZwpTabletToolV2;
class WpCursorShapeManagerV1 final {
public:
    static constexpr const char *interface = "wp_cursor_shape_manager_v1";
    static constexpr const wl_interface *const wlInterface =
        &wp_cursor_shape_manager_v1_interface;
    static constexpr const uint32_t version = 1;
    typedef wp_cursor_shape_manager_v1 wlType;
    operator wp_cursor_shape_manager_v1 *() { return data_.get(); }
    WpCursorShapeManagerV1(wlType *data);
    WpCursorShapeManagerV1(WpCursorShapeManagerV1 &&other) noexcept = delete;
    WpCursorShapeManagerV1 &
    operator=(WpCursorShapeManagerV1 &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    WpCursorShapeDeviceV1 *getPointer(WlPointer *pointer);
    WpCursorShapeDeviceV1 *getTabletToolV2(ZwpTabletToolV2 *tabletTool);

private:
    static void destructor(wp_cursor_shape_manager_v1 *);
    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<wp_cursor_shape_manager_v1, &destructor> data_;
};
static inline wp_cursor_shape_manager_v1 *
rawPointer(WpCursorShapeManagerV1 *p) {
    return p ? static_cast<wp_cursor_shape_manager_v1 *>(*p) : nullptr;
}
} // namespace fcitx::wayland
#endif
