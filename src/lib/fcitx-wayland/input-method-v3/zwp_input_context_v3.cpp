#include "zwp_input_context_v3.h"
#include <cassert>
namespace fcitx::wayland {
const struct zwp_input_context_v3_listener ZwpInputContextV3::listener = {
    [](void *data, zwp_input_context_v3 *wldata, const char *text,
       uint32_t cursor, uint32_t anchor) {
        auto *obj = static_cast<ZwpInputContextV3 *>(data);
        assert(*obj == wldata);
        { return obj->surroundingText()(text, cursor, anchor); }
    },
    [](void *data, zwp_input_context_v3 *wldata, uint32_t cause) {
        auto *obj = static_cast<ZwpInputContextV3 *>(data);
        assert(*obj == wldata);
        { return obj->textChangeCause()(cause); }
    },
    [](void *data, zwp_input_context_v3 *wldata, uint32_t hint,
       uint32_t purpose) {
        auto *obj = static_cast<ZwpInputContextV3 *>(data);
        assert(*obj == wldata);
        { return obj->contentType()(hint, purpose); }
    },
};
ZwpInputContextV3::ZwpInputContextV3(zwp_input_context_v3 *data)
    : version_(zwp_input_context_v3_get_version(data)), data_(data) {
    zwp_input_context_v3_set_user_data(*this, this);
    zwp_input_context_v3_add_listener(*this, &ZwpInputContextV3::listener,
                                      this);
}
void ZwpInputContextV3::destructor(zwp_input_context_v3 *data) {
    { return zwp_input_context_v3_destroy(data); }
}
void ZwpInputContextV3::commitString(const char *text) {
    return zwp_input_context_v3_commit_string(*this, text);
}
void ZwpInputContextV3::setPreeditString(const char *text, int32_t cursorBegin,
                                         int32_t cursorEnd) {
    return zwp_input_context_v3_set_preedit_string(*this, text, cursorBegin,
                                                   cursorEnd);
}
void ZwpInputContextV3::deleteSurroundingText(uint32_t beforeLength,
                                              uint32_t afterLength) {
    return zwp_input_context_v3_delete_surrounding_text(*this, beforeLength,
                                                        afterLength);
}
void ZwpInputContextV3::commit(uint32_t serial) {
    return zwp_input_context_v3_commit(*this, serial);
}
} // namespace fcitx::wayland
