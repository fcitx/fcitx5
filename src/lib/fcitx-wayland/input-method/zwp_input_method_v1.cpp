#include "zwp_input_method_v1.h"
#include <cassert>
#include "zwp_input_method_context_v1.h"
namespace fcitx::wayland {
const struct zwp_input_method_v1_listener ZwpInputMethodV1::listener = {
    [](void *data, zwp_input_method_v1 *wldata,
       zwp_input_method_context_v1 *id) {
        auto *obj = static_cast<ZwpInputMethodV1 *>(data);
        assert(*obj == wldata);
        {
            auto *id_ = new ZwpInputMethodContextV1(id);
            return obj->activate()(id_);
        }
    },
    [](void *data, zwp_input_method_v1 *wldata,
       zwp_input_method_context_v1 *context) {
        auto *obj = static_cast<ZwpInputMethodV1 *>(data);
        assert(*obj == wldata);
        {
            if (!context) {
                return;
            }
            auto *context_ = static_cast<ZwpInputMethodContextV1 *>(
                zwp_input_method_context_v1_get_user_data(context));
            return obj->deactivate()(context_);
        }
    },
};
ZwpInputMethodV1::ZwpInputMethodV1(zwp_input_method_v1 *data)
    : version_(zwp_input_method_v1_get_version(data)), data_(data) {
    zwp_input_method_v1_set_user_data(*this, this);
    zwp_input_method_v1_add_listener(*this, &ZwpInputMethodV1::listener, this);
}
void ZwpInputMethodV1::destructor(zwp_input_method_v1 *data) {
    { return zwp_input_method_v1_destroy(data); }
}
} // namespace fcitx::wayland
