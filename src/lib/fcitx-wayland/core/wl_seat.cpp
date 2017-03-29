#include "wl_seat.h"
#include "wl_keyboard.h"
#include "wl_pointer.h"
#include "wl_touch.h"
#include <cassert>
namespace fcitx {
namespace wayland {
constexpr const char *WlSeat::interface;
constexpr const wl_interface *const WlSeat::wlInterface;
const uint32_t WlSeat::version;
const struct wl_seat_listener WlSeat::listener = {
    [](void *data, wl_seat *wldata, uint32_t capabilities) {
        auto obj = static_cast<WlSeat *>(data);
        assert(*obj == wldata);
        { return obj->capabilities()(capabilities); }
    },
    [](void *data, wl_seat *wldata, const char *name) {
        auto obj = static_cast<WlSeat *>(data);
        assert(*obj == wldata);
        { return obj->name()(name); }
    },
};
WlSeat::WlSeat(wl_seat *data)
    : version_(wl_seat_get_version(data)), data_(data, &WlSeat::destructor) {
    wl_seat_set_user_data(*this, this);
    wl_seat_add_listener(*this, &WlSeat::listener, this);
}
void WlSeat::destructor(wl_seat *data) {
    auto version = wl_seat_get_version(data);
    if (version >= 5) {
        return wl_seat_release(data);
    } else {
        return wl_seat_destroy(data);
    }
}
WlPointer *WlSeat::getPointer() {
    return new WlPointer(wl_seat_get_pointer(*this));
}
WlKeyboard *WlSeat::getKeyboard() {
    return new WlKeyboard(wl_seat_get_keyboard(*this));
}
WlTouch *WlSeat::getTouch() { return new WlTouch(wl_seat_get_touch(*this)); }
}
}
