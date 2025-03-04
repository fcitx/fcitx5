#include "wl_registry.h"
#include <cassert>

namespace fcitx::wayland {
const struct wl_registry_listener WlRegistry::listener = {
    .global =
        [](void *data, wl_registry *wldata, uint32_t name,
           const char *interface, uint32_t version) {
            auto *obj = static_cast<WlRegistry *>(data);
            assert(*obj == wldata);
            {
                obj->global()(name, interface, version);
            }
        },
    .global_remove =
        [](void *data, wl_registry *wldata, uint32_t name) {
            auto *obj = static_cast<WlRegistry *>(data);
            assert(*obj == wldata);
            {
                obj->globalRemove()(name);
            }
        },
};

WlRegistry::WlRegistry(wl_registry *data)
    : version_(wl_registry_get_version(data)), data_(data) {
    wl_registry_set_user_data(*this, this);
    wl_registry_add_listener(*this, &WlRegistry::listener, this);
}

void WlRegistry::destructor(wl_registry *data) { wl_registry_destroy(data); }

} // namespace fcitx::wayland
