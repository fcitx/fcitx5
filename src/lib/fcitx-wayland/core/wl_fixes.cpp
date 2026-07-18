#include "wl_fixes.h"
#include "wl_registry.h"

namespace fcitx::wayland {

WlFixes::WlFixes(wl_fixes *data)
    : version_(wl_fixes_get_version(data)), data_(data) {
    wl_fixes_set_user_data(*this, this);
}

void WlFixes::destructor(wl_fixes *data) { wl_fixes_destroy(data); }
#if defined(WL_FIXES_DESTROY_REGISTRY_SINCE_VERSION)
void WlFixes::destroyRegistry(WlRegistry *registry) {
    wl_fixes_destroy_registry(*this, rawPointer(registry));
}
#endif
#if defined(WL_FIXES_ACK_GLOBAL_REMOVE_SINCE_VERSION)
void WlFixes::ackGlobalRemove(WlRegistry *registry, uint32_t name) {
    wl_fixes_ack_global_remove(*this, rawPointer(registry), name);
}
#endif

} // namespace fcitx::wayland
