#include "wl_seat.h"
#include <cassert>
#include "wl_keyboard.h"
#include "wl_pointer.h"
#include "wl_touch.h"

namespace fcitx::wayland {
const struct wl_seat_listener WlSeat::listener = {
    .capabilities =
        [](void *data, wl_seat *wldata, uint32_t capabilities) {
            auto *obj = static_cast<WlSeat *>(data);
            assert(*obj == wldata);
            {
                obj->capabilities()(capabilities);
            }
        },
    .name =
        [](void *data, wl_seat *wldata, const char *name) {
            auto *obj = static_cast<WlSeat *>(data);
            assert(*obj == wldata);
            {
                obj->name()(name);
            }
        },
};

WlSeat::WlSeat(wl_seat *data)
    : version_(wl_seat_get_version(data)), data_(data) {
    wl_seat_set_user_data(*this, this);
    wl_seat_add_listener(*this, &WlSeat::listener, this);
}

void WlSeat::destructor(wl_seat *data) {
    const auto version = wl_seat_get_version(data);
    if (version >= 5) {
        wl_seat_release(data);
        return;
    }
    wl_seat_destroy(data);
}
WlPointer *WlSeat::getPointer() {
    return new WlPointer(wl_seat_get_pointer(*this));
}
WlKeyboard *WlSeat::getKeyboard() {
    return new WlKeyboard(wl_seat_get_keyboard(*this));
}
WlTouch *WlSeat::getTouch() { return new WlTouch(wl_seat_get_touch(*this)); }

} // namespace fcitx::wayland
