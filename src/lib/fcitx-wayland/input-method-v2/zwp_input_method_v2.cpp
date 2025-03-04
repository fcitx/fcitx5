#include "zwp_input_method_v2.h"
#include <cassert>
#include "wayland-input-method-unstable-v2-client-protocol.h"
#include "wl_surface.h"
#include "zwp_input_method_keyboard_grab_v2.h"
#include "zwp_input_popup_surface_v2.h"

namespace fcitx::wayland {
const struct zwp_input_method_v2_listener ZwpInputMethodV2::listener = {
    .activate =
        [](void *data, zwp_input_method_v2 *wldata) {
            auto *obj = static_cast<ZwpInputMethodV2 *>(data);
            assert(*obj == wldata);
            {
                obj->activate()();
            }
        },
    .deactivate =
        [](void *data, zwp_input_method_v2 *wldata) {
            auto *obj = static_cast<ZwpInputMethodV2 *>(data);
            assert(*obj == wldata);
            {
                obj->deactivate()();
            }
        },
    .surrounding_text =
        [](void *data, zwp_input_method_v2 *wldata, const char *text,
           uint32_t cursor, uint32_t anchor) {
            auto *obj = static_cast<ZwpInputMethodV2 *>(data);
            assert(*obj == wldata);
            {
                obj->surroundingText()(text, cursor, anchor);
            }
        },
    .text_change_cause =
        [](void *data, zwp_input_method_v2 *wldata, uint32_t cause) {
            auto *obj = static_cast<ZwpInputMethodV2 *>(data);
            assert(*obj == wldata);
            {
                obj->textChangeCause()(cause);
            }
        },
    .content_type =
        [](void *data, zwp_input_method_v2 *wldata, uint32_t hint,
           uint32_t purpose) {
            auto *obj = static_cast<ZwpInputMethodV2 *>(data);
            assert(*obj == wldata);
            {
                obj->contentType()(hint, purpose);
            }
        },
    .done =
        [](void *data, zwp_input_method_v2 *wldata) {
            auto *obj = static_cast<ZwpInputMethodV2 *>(data);
            assert(*obj == wldata);
            {
                obj->done()();
            }
        },
    .unavailable =
        [](void *data, zwp_input_method_v2 *wldata) {
            auto *obj = static_cast<ZwpInputMethodV2 *>(data);
            assert(*obj == wldata);
            {
                obj->unavailable()();
            }
        },
};

ZwpInputMethodV2::ZwpInputMethodV2(zwp_input_method_v2 *data)
    : version_(zwp_input_method_v2_get_version(data)), data_(data) {
    zwp_input_method_v2_set_user_data(*this, this);
    zwp_input_method_v2_add_listener(*this, &ZwpInputMethodV2::listener, this);
}

void ZwpInputMethodV2::destructor(zwp_input_method_v2 *data) {
    const auto version = zwp_input_method_v2_get_version(data);
    if (version >= 1) {
        zwp_input_method_v2_destroy(data);
        return;
    }
}
void ZwpInputMethodV2::commitString(const char *text) {
    zwp_input_method_v2_commit_string(*this, text);
}
void ZwpInputMethodV2::setPreeditString(const char *text, int32_t cursorBegin,
                                        int32_t cursorEnd) {
    zwp_input_method_v2_set_preedit_string(*this, text, cursorBegin, cursorEnd);
}
void ZwpInputMethodV2::deleteSurroundingText(uint32_t beforeLength,
                                             uint32_t afterLength) {
    zwp_input_method_v2_delete_surrounding_text(*this, beforeLength,
                                                afterLength);
}
void ZwpInputMethodV2::commit(uint32_t serial) {
    zwp_input_method_v2_commit(*this, serial);
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
