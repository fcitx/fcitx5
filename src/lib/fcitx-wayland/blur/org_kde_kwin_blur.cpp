#include "org_kde_kwin_blur.h"
#include "wayland-blur-client-protocol.h"
#include "wl_region.h"

namespace fcitx::wayland {

OrgKdeKwinBlur::OrgKdeKwinBlur(org_kde_kwin_blur *data)
    : version_(org_kde_kwin_blur_get_version(data)), data_(data) {
    org_kde_kwin_blur_set_user_data(*this, this);
}

void OrgKdeKwinBlur::destructor(org_kde_kwin_blur *data) {
    const auto version = org_kde_kwin_blur_get_version(data);
#if defined(ORG_KDE_KWIN_BLUR_RELEASE_SINCE_VERSION)
    if (version >= 1) {
        org_kde_kwin_blur_release(data);
        return;
    }
#endif
    org_kde_kwin_blur_destroy(data);
}
void OrgKdeKwinBlur::commit() { org_kde_kwin_blur_commit(*this); }
void OrgKdeKwinBlur::setRegion(WlRegion *region) {
    org_kde_kwin_blur_set_region(*this, rawPointer(region));
}

} // namespace fcitx::wayland
