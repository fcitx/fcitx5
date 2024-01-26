#ifndef ORG_KDE_KWIN_BLUR_MANAGER
#define ORG_KDE_KWIN_BLUR_MANAGER
#include <wayland-client.h>
#include "fcitx-utils/signals.h"
#include "wayland-blur-client-protocol.h"
namespace fcitx::wayland {
class OrgKdeKwinBlur;
class WlSurface;
class OrgKdeKwinBlurManager final {
public:
    static constexpr const char *interface = "org_kde_kwin_blur_manager";
    static constexpr const wl_interface *const wlInterface =
        &org_kde_kwin_blur_manager_interface;
    static constexpr const uint32_t version = 1;
    typedef org_kde_kwin_blur_manager wlType;
    operator org_kde_kwin_blur_manager *() { return data_.get(); }
    OrgKdeKwinBlurManager(wlType *data);
    OrgKdeKwinBlurManager(OrgKdeKwinBlurManager &&other) noexcept = delete;
    OrgKdeKwinBlurManager &
    operator=(OrgKdeKwinBlurManager &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    OrgKdeKwinBlur *create(WlSurface *surface);
    void unset(WlSurface *surface);

private:
    static void destructor(org_kde_kwin_blur_manager *);
    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<org_kde_kwin_blur_manager, &destructor> data_;
};
static inline org_kde_kwin_blur_manager *rawPointer(OrgKdeKwinBlurManager *p) {
    return p ? static_cast<org_kde_kwin_blur_manager *>(*p) : nullptr;
}
} // namespace fcitx::wayland
#endif
