#include "wl_callback.h"
#include <cassert>
namespace fcitx::wayland {
const struct wl_callback_listener WlCallback::listener = {
    [](void *data, wl_callback *wldata, uint32_t callbackData) {
        auto *obj = static_cast<WlCallback *>(data);
        assert(*obj == wldata);
        {
            return obj->done()(callbackData);
        }
    },
};
WlCallback::WlCallback(wl_callback *data)
    : version_(wl_callback_get_version(data)), data_(data) {
    wl_callback_set_user_data(*this, this);
    wl_callback_add_listener(*this, &WlCallback::listener, this);
}
void WlCallback::destructor(wl_callback *data) {
    {
        return wl_callback_destroy(data);
    }
}
} // namespace fcitx::wayland
