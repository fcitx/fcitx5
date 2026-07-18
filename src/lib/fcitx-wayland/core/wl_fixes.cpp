#include "wl_fixes.h"
#include "wl_registry.h"

namespace fcitx::wayland {

WlFixes::WlFixes(wl_fixes *data)
    : version_(wl_fixes_get_version(data)), data_(data) {
    wl_fixes_set_user_data(*this, this);
}

void WlFixes::destructor(wl_fixes *data) { wl_fixes_destroy(data); }
void WlFixes::destroyRegistry(WlRegistry *registry) {
    wl_fixes_destroy_registry(*this, rawPointer(registry));
}
void WlFixes::ackGlobalRemove(WlRegistry *registry, uint32_t name) {
    wl_fixes_ack_global_remove(*this, rawPointer(registry), name);
}

} // namespace fcitx::wayland
