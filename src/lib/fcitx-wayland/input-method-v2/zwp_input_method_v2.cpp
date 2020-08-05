#include "zwp_input_method_v2.h"
#include <cassert>
#include "wl_surface.h"
#include "zwp_input_method_keyboard_grab_v2.h"
#include "zwp_input_popup_surface_v2.h"
namespace fcitx::wayland {

const struct zwp_input_method_v2_listener ZwpInputMethodV2::listener = {
    [](void *data, zwp_input_method_v2 *wldata) {
        auto *obj = static_cast<ZwpInputMethodV2 *>(data);
        assert(*obj == wldata);
        { return obj->activate()(); }
    },
    [](void *data, zwp_input_method_v2 *wldata) {
        auto *obj = static_cast<ZwpInputMethodV2 *>(data);
        assert(*obj == wldata);
        { return obj->deactivate()(); }
    },
    [](void *data, zwp_input_method_v2 *wldata, const char *text,
       uint32_t cursor, uint32_t anchor) {
        auto *obj = static_cast<ZwpInputMethodV2 *>(data);
        assert(*obj == wldata);
        { return obj->surroundingText()(text, cursor, anchor); }
    },
    [](void *data, zwp_input_method_v2 *wldata, uint32_t cause) {
        auto *obj = static_cast<ZwpInputMethodV2 *>(data);
        assert(*obj == wldata);
        { return obj->textChangeCause()(cause); }
    },
    [](void *data, zwp_input_method_v2 *wldata, uint32_t hint,
       uint32_t purpose) {
        auto *obj = static_cast<ZwpInputMethodV2 *>(data);
        assert(*obj == wldata);
        { return obj->contentType()(hint, purpose); }
    },
    [](void *data, zwp_input_method_v2 *wldata) {
        auto *obj = static_cast<ZwpInputMethodV2 *>(data);
        assert(*obj == wldata);
        { return obj->done()(); }
    },
    [](void *data, zwp_input_method_v2 *wldata) {
        auto *obj = static_cast<ZwpInputMethodV2 *>(data);
        assert(*obj == wldata);
        { return obj->unavailable()(); }
    },
};
ZwpInputMethodV2::ZwpInputMethodV2(zwp_input_method_v2 *data)
    : version_(zwp_input_method_v2_get_version(data)), data_(data) {
    zwp_input_method_v2_set_user_data(*this, this);
    zwp_input_method_v2_add_listener(*this, &ZwpInputMethodV2::listener, this);
}
void ZwpInputMethodV2::destructor(zwp_input_method_v2 *data) {
    auto version = zwp_input_method_v2_get_version(data);
    if (version >= 1) {
        return zwp_input_method_v2_destroy(data);
    }
}
void ZwpInputMethodV2::commitString(const char *text) {
    return zwp_input_method_v2_commit_string(*this, text);
}
void ZwpInputMethodV2::setPreeditString(const char *text, int32_t cursorBegin,
                                        int32_t cursorEnd) {
    return zwp_input_method_v2_set_preedit_string(*this, text, cursorBegin,
                                                  cursorEnd);
}
void ZwpInputMethodV2::deleteSurroundingText(uint32_t beforeLength,
                                             uint32_t afterLength) {
    return zwp_input_method_v2_delete_surrounding_text(*this, beforeLength,
                                                       afterLength);
}
void ZwpInputMethodV2::commit(uint32_t serial) {
    return zwp_input_method_v2_commit(*this, serial);
}
ZwpInputPopupSurfaceV2 *
ZwpInputMethodV2::getInputPopupSurface(WlSurface *surface) {
    return new ZwpInputPopupSurfaceV2(
        zwp_input_method_v2_get_input_popup_surface(*this,
                                                    rawPointer(surface)));
}
ZwpInputMethodKeyboardGrabV2 *ZwpInputMethodV2::grabKeyboard() {
    return new ZwpInputMethodKeyboardGrabV2(
        zwp_input_method_v2_grab_keyboard(*this));
}
} // namespace fcitx::wayland
