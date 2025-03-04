#include "org_kde_kwin_blur_manager.h"
#include "org_kde_kwin_blur.h"
#include "wayland-blur-client-protocol.h"
#include "wl_surface.h"

namespace fcitx::wayland {

OrgKdeKwinBlurManager::OrgKdeKwinBlurManager(org_kde_kwin_blur_manager *data)
    : version_(org_kde_kwin_blur_manager_get_version(data)), data_(data) {
    org_kde_kwin_blur_manager_set_user_data(*this, this);
}

void OrgKdeKwinBlurManager::destructor(org_kde_kwin_blur_manager *data) {
    org_kde_kwin_blur_manager_destroy(data);
}
OrgKdeKwinBlur *OrgKdeKwinBlurManager::create(WlSurface *surface) {
    return new OrgKdeKwinBlur(
        org_kde_kwin_blur_manager_create(*this, rawPointer(surface)));
}
void OrgKdeKwinBlurManager::unset(WlSurface *surface) {
    org_kde_kwin_blur_manager_unset(*this, rawPointer(surface));
}

} // namespace fcitx::wayland
