#ifndef ORG_KDE_KWIN_BLUR
#define ORG_KDE_KWIN_BLUR
#include <memory>
#include <wayland-client.h>
#include "fcitx-utils/signals.h"
#include "wayland-blur-client-protocol.h"
namespace fcitx::wayland {
class WlRegion;
class OrgKdeKwinBlur final {
public:
    static constexpr const char *interface = "org_kde_kwin_blur";
    static constexpr const wl_interface *const wlInterface =
        &org_kde_kwin_blur_interface;
    static constexpr const uint32_t version = 1;
    typedef org_kde_kwin_blur wlType;
    operator org_kde_kwin_blur *() { return data_.get(); }
    OrgKdeKwinBlur(wlType *data);
    OrgKdeKwinBlur(OrgKdeKwinBlur &&other) noexcept = delete;
    OrgKdeKwinBlur &operator=(OrgKdeKwinBlur &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    void commit();
    void setRegion(WlRegion *region);

private:
    static void destructor(org_kde_kwin_blur *);
    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<org_kde_kwin_blur, &destructor> data_;
};
static inline org_kde_kwin_blur *rawPointer(OrgKdeKwinBlur *p) {
    return p ? static_cast<org_kde_kwin_blur *>(*p) : nullptr;
}
} // namespace fcitx::wayland
#endif
