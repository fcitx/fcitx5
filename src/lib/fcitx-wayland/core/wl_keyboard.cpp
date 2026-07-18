#include "wl_keyboard.h"
#include <cassert>
#include "wl_surface.h"

namespace fcitx::wayland {
const struct wl_keyboard_listener WlKeyboard::listener = {
#if defined(WL_KEYBOARD_KEYMAP_SINCE_VERSION)
    .keymap =
        [](void *data, wl_keyboard *wldata, uint32_t format, int32_t fd,
           uint32_t size) {
            auto *obj = static_cast<WlKeyboard *>(data);
            assert(*obj == wldata);
            {
                obj->keymap()(format, fd, size);
            }
        },
#endif
#if defined(WL_KEYBOARD_ENTER_SINCE_VERSION)
    .enter =
        [](void *data, wl_keyboard *wldata, uint32_t serial,
           wl_surface *surface, wl_array *keys) {
            auto *obj = static_cast<WlKeyboard *>(data);
            assert(*obj == wldata);
            {
                if (!surface) {
                    return;
                }
                auto *surface_ =
                    static_cast<WlSurface *>(wl_surface_get_user_data(surface));
                obj->enter()(serial, surface_, keys);
            }
        },
#endif
#if defined(WL_KEYBOARD_LEAVE_SINCE_VERSION)
    .leave =
        [](void *data, wl_keyboard *wldata, uint32_t serial,
           wl_surface *surface) {
            auto *obj = static_cast<WlKeyboard *>(data);
            assert(*obj == wldata);
            {
                if (!surface) {
                    return;
                }
                auto *surface_ =
                    static_cast<WlSurface *>(wl_surface_get_user_data(surface));
                obj->leave()(serial, surface_);
            }
        },
#endif
#if defined(WL_KEYBOARD_KEY_SINCE_VERSION)
    .key =
        [](void *data, wl_keyboard *wldata, uint32_t serial, uint32_t time,
           uint32_t key, uint32_t state) {
            auto *obj = static_cast<WlKeyboard *>(data);
            assert(*obj == wldata);
            {
                obj->key()(serial, time, key, state);
            }
        },
#endif
#if defined(WL_KEYBOARD_MODIFIERS_SINCE_VERSION)
    .modifiers =
        [](void *data, wl_keyboard *wldata, uint32_t serial,
           uint32_t modsDepressed, uint32_t modsLatched, uint32_t modsLocked,
           uint32_t group) {
            auto *obj = static_cast<WlKeyboard *>(data);
            assert(*obj == wldata);
            {
                obj->modifiers()(serial, modsDepressed, modsLatched, modsLocked,
                                 group);
            }
        },
#endif
#if defined(WL_KEYBOARD_REPEAT_INFO_SINCE_VERSION)
    .repeat_info =
        [](void *data, wl_keyboard *wldata, int32_t rate, int32_t delay) {
            auto *obj = static_cast<WlKeyboard *>(data);
            assert(*obj == wldata);
            {
                obj->repeatInfo()(rate, delay);
            }
        },
#endif
};

WlKeyboard::WlKeyboard(wl_keyboard *data)
    : version_(wl_keyboard_get_version(data)), data_(data) {
    wl_keyboard_set_user_data(*this, this);
    wl_keyboard_add_listener(*this, &WlKeyboard::listener, this);
}

void WlKeyboard::destructor(wl_keyboard *data) {
    const auto version = wl_keyboard_get_version(data);
#if defined(WL_KEYBOARD_RELEASE_SINCE_VERSION)
    if (version >= 3) {
        wl_keyboard_release(data);
        return;
    }
#endif
    wl_keyboard_destroy(data);
}

} // namespace fcitx::wayland
