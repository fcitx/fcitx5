#include "wl_keyboard.h"
#include <cassert>
#include "wl_surface.h"
namespace fcitx {
namespace wayland {
constexpr const char *WlKeyboard::interface;
constexpr const wl_interface *const WlKeyboard::wlInterface;
const uint32_t WlKeyboard::version;
const struct wl_keyboard_listener WlKeyboard::listener = {
    [](void *data, wl_keyboard *wldata, uint32_t format, int32_t fd,
       uint32_t size) {
        auto obj = static_cast<WlKeyboard *>(data);
        assert(*obj == wldata);
        { return obj->keymap()(format, fd, size); }
    },
    [](void *data, wl_keyboard *wldata, uint32_t serial, wl_surface *surface,
       wl_array *keys) {
        auto obj = static_cast<WlKeyboard *>(data);
        assert(*obj == wldata);
        {
            auto surface_ =
                static_cast<WlSurface *>(wl_surface_get_user_data(surface));
            return obj->enter()(serial, surface_, keys);
        }
    },
    [](void *data, wl_keyboard *wldata, uint32_t serial, wl_surface *surface) {
        auto obj = static_cast<WlKeyboard *>(data);
        assert(*obj == wldata);
        {
            auto surface_ =
                static_cast<WlSurface *>(wl_surface_get_user_data(surface));
            return obj->leave()(serial, surface_);
        }
    },
    [](void *data, wl_keyboard *wldata, uint32_t serial, uint32_t time,
       uint32_t key, uint32_t state) {
        auto obj = static_cast<WlKeyboard *>(data);
        assert(*obj == wldata);
        { return obj->key()(serial, time, key, state); }
    },
    [](void *data, wl_keyboard *wldata, uint32_t serial, uint32_t modsDepressed,
       uint32_t modsLatched, uint32_t modsLocked, uint32_t group) {
        auto obj = static_cast<WlKeyboard *>(data);
        assert(*obj == wldata);
        {
            return obj->modifiers()(serial, modsDepressed, modsLatched,
                                    modsLocked, group);
        }
    },
    [](void *data, wl_keyboard *wldata, int32_t rate, int32_t delay) {
        auto obj = static_cast<WlKeyboard *>(data);
        assert(*obj == wldata);
        { return obj->repeatInfo()(rate, delay); }
    },
};
WlKeyboard::WlKeyboard(wl_keyboard *data)
    : version_(wl_keyboard_get_version(data)), data_(data) {
    wl_keyboard_set_user_data(*this, this);
    wl_keyboard_add_listener(*this, &WlKeyboard::listener, this);
}
void WlKeyboard::destructor(wl_keyboard *data) {
    auto version = wl_keyboard_get_version(data);
    if (version >= 3) {
        return wl_keyboard_release(data);
    } else {
        return wl_keyboard_destroy(data);
    }
}
} // namespace wayland
} // namespace fcitx
