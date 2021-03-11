#include "zwp_input_method_v3.h"
#include <cassert>
#include "wl_surface.h"
#include "zwp_input_method_keyboard_grab_v3.h"
#include "zwp_input_popup_surface_v3.h"
namespace fcitx::wayland {
const struct zwp_input_method_v3_listener ZwpInputMethodV3::listener = {
    [](void *data, zwp_input_method_v3 *wldata, uint32_t name,
       uint32_t version) {
        auto *obj = static_cast<ZwpInputMethodV3 *>(data);
        assert(*obj == wldata);
        { return obj->inputContext()(name, version); }
    },
    [](void *data, zwp_input_method_v3 *wldata) {
        auto *obj = static_cast<ZwpInputMethodV3 *>(data);
        assert(*obj == wldata);
        { return obj->inputContextRemove()(); }
    },
    [](void *data, zwp_input_method_v3 *wldata) {
        auto *obj = static_cast<ZwpInputMethodV3 *>(data);
        assert(*obj == wldata);
        { return obj->unavailable()(); }
    },
};
ZwpInputMethodV3::ZwpInputMethodV3(zwp_input_method_v3 *data)
    : version_(zwp_input_method_v3_get_version(data)), data_(data) {
    zwp_input_method_v3_set_user_data(*this, this);
    zwp_input_method_v3_add_listener(*this, &ZwpInputMethodV3::listener, this);
}
void ZwpInputMethodV3::destructor(zwp_input_method_v3 *data) {
    { return zwp_input_method_v3_destroy(data); }
}
ZwpInputPopupSurfaceV3 *
ZwpInputMethodV3::getInputPopupSurface(WlSurface *surface) {
    return new ZwpInputPopupSurfaceV3(
        zwp_input_method_v3_get_input_popup_surface(*this,
                                                    rawPointer(surface)));
}
ZwpInputMethodKeyboardGrabV3 *ZwpInputMethodV3::grabKeyboard() {
    return new ZwpInputMethodKeyboardGrabV3(
        zwp_input_method_v3_grab_keyboard(*this));
}
} // namespace fcitx::wayland
