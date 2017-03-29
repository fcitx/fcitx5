#include "zwp_input_method_context_v1.h"
#include "wl_keyboard.h"
#include <cassert>
namespace fcitx {
namespace wayland {
constexpr const char *ZwpInputMethodContextV1::interface;
constexpr const wl_interface *const ZwpInputMethodContextV1::wlInterface;
const uint32_t ZwpInputMethodContextV1::version;
const struct zwp_input_method_context_v1_listener
    ZwpInputMethodContextV1::listener = {
        [](void *data, zwp_input_method_context_v1 *wldata, const char *text,
           uint32_t cursor, uint32_t anchor) {
            auto obj = static_cast<ZwpInputMethodContextV1 *>(data);
            assert(*obj == wldata);
            { return obj->surroundingText()(text, cursor, anchor); }
        },
        [](void *data, zwp_input_method_context_v1 *wldata) {
            auto obj = static_cast<ZwpInputMethodContextV1 *>(data);
            assert(*obj == wldata);
            { return obj->reset()(); }
        },
        [](void *data, zwp_input_method_context_v1 *wldata, uint32_t hint,
           uint32_t purpose) {
            auto obj = static_cast<ZwpInputMethodContextV1 *>(data);
            assert(*obj == wldata);
            { return obj->contentType()(hint, purpose); }
        },
        [](void *data, zwp_input_method_context_v1 *wldata, uint32_t button,
           uint32_t index) {
            auto obj = static_cast<ZwpInputMethodContextV1 *>(data);
            assert(*obj == wldata);
            { return obj->invokeAction()(button, index); }
        },
        [](void *data, zwp_input_method_context_v1 *wldata, uint32_t serial) {
            auto obj = static_cast<ZwpInputMethodContextV1 *>(data);
            assert(*obj == wldata);
            { return obj->commitState()(serial); }
        },
        [](void *data, zwp_input_method_context_v1 *wldata,
           const char *language) {
            auto obj = static_cast<ZwpInputMethodContextV1 *>(data);
            assert(*obj == wldata);
            { return obj->preferredLanguage()(language); }
        },
};
ZwpInputMethodContextV1::ZwpInputMethodContextV1(
    zwp_input_method_context_v1 *data)
    : version_(zwp_input_method_context_v1_get_version(data)),
      data_(data, &ZwpInputMethodContextV1::destructor) {
    zwp_input_method_context_v1_set_user_data(*this, this);
    zwp_input_method_context_v1_add_listener(
        *this, &ZwpInputMethodContextV1::listener, this);
}
void ZwpInputMethodContextV1::destructor(zwp_input_method_context_v1 *data) {
    auto version = zwp_input_method_context_v1_get_version(data);
    if (version >= 1) {
        return zwp_input_method_context_v1_destroy(data);
    }
}
void ZwpInputMethodContextV1::commitString(uint32_t serial, const char *text) {
    return zwp_input_method_context_v1_commit_string(*this, serial, text);
}
void ZwpInputMethodContextV1::preeditString(uint32_t serial, const char *text,
                                            const char *commit) {
    return zwp_input_method_context_v1_preedit_string(*this, serial, text,
                                                      commit);
}
void ZwpInputMethodContextV1::preeditStyling(uint32_t index, uint32_t length,
                                             uint32_t style) {
    return zwp_input_method_context_v1_preedit_styling(*this, index, length,
                                                       style);
}
void ZwpInputMethodContextV1::preeditCursor(int32_t index) {
    return zwp_input_method_context_v1_preedit_cursor(*this, index);
}
void ZwpInputMethodContextV1::deleteSurroundingText(int32_t index,
                                                    uint32_t length) {
    return zwp_input_method_context_v1_delete_surrounding_text(*this, index,
                                                               length);
}
void ZwpInputMethodContextV1::cursorPosition(int32_t index, int32_t anchor) {
    return zwp_input_method_context_v1_cursor_position(*this, index, anchor);
}
void ZwpInputMethodContextV1::modifiersMap(wl_array *map) {
    return zwp_input_method_context_v1_modifiers_map(*this, map);
}
void ZwpInputMethodContextV1::keysym(uint32_t serial, uint32_t time,
                                     uint32_t sym, uint32_t state,
                                     uint32_t modifiers) {
    return zwp_input_method_context_v1_keysym(*this, serial, time, sym, state,
                                              modifiers);
}
WlKeyboard *ZwpInputMethodContextV1::grabKeyboard() {
    return new WlKeyboard(zwp_input_method_context_v1_grab_keyboard(*this));
}
void ZwpInputMethodContextV1::key(uint32_t serial, uint32_t time, uint32_t key,
                                  uint32_t state) {
    return zwp_input_method_context_v1_key(*this, serial, time, key, state);
}
void ZwpInputMethodContextV1::modifiers(uint32_t serial, uint32_t modsDepressed,
                                        uint32_t modsLatched,
                                        uint32_t modsLocked, uint32_t group) {
    return zwp_input_method_context_v1_modifiers(
        *this, serial, modsDepressed, modsLatched, modsLocked, group);
}
void ZwpInputMethodContextV1::language(uint32_t serial, const char *language) {
    return zwp_input_method_context_v1_language(*this, serial, language);
}
void ZwpInputMethodContextV1::textDirection(uint32_t serial,
                                            uint32_t direction) {
    return zwp_input_method_context_v1_text_direction(*this, serial, direction);
}
}
}
