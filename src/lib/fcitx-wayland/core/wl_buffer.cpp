#include "wl_buffer.h"
#include <cassert>
namespace fcitx {
namespace wayland {
constexpr const char *WlBuffer::interface;
constexpr const wl_interface *const WlBuffer::wlInterface;
const uint32_t WlBuffer::version;
const struct wl_buffer_listener WlBuffer::listener = {
    [](void *data, wl_buffer *wldata) {
        auto obj = static_cast<WlBuffer *>(data);
        assert(*obj == wldata);
        { return obj->release()(); }
    },
};
WlBuffer::WlBuffer(wl_buffer *data)
    : version_(wl_buffer_get_version(data)),
      data_(data, &WlBuffer::destructor) {
    wl_buffer_set_user_data(*this, this);
    wl_buffer_add_listener(*this, &WlBuffer::listener, this);
}
void WlBuffer::destructor(wl_buffer *data) {
    auto version = wl_buffer_get_version(data);
    if (version >= 1) {
        return wl_buffer_destroy(data);
    }
}
} // namespace wayland
} // namespace fcitx
