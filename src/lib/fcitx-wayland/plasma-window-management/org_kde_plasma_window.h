#ifndef ORG_KDE_PLASMA_WINDOW_H_
#define ORG_KDE_PLASMA_WINDOW_H_
#include <cstdint>
#include <wayland-client.h>
#include <wayland-util.h>
#include "fcitx-utils/misc.h"
#include "fcitx-utils/signals.h"
#include "wayland-plasma-window-management-client-protocol.h" // IWYU pragma: export
namespace fcitx::wayland {

class WlOutput;
class WlSurface;

class OrgKdePlasmaWindow final {
public:
    static constexpr const char *interface = "org_kde_plasma_window";
    static constexpr const wl_interface *const wlInterface =
        &org_kde_plasma_window_interface;
    static constexpr const uint32_t version = 16;
    using wlType = org_kde_plasma_window;
    operator org_kde_plasma_window *() { return data_.get(); }
    OrgKdePlasmaWindow(wlType *data);
    OrgKdePlasmaWindow(OrgKdePlasmaWindow &&other) noexcept = delete;
    OrgKdePlasmaWindow &operator=(OrgKdePlasmaWindow &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    void setState(uint32_t flags, uint32_t state);
    void setVirtualDesktop(uint32_t number);
    void setMinimizedGeometry(WlSurface *panel, uint32_t x, uint32_t y,
                              uint32_t width, uint32_t height);
    void unsetMinimizedGeometry(WlSurface *panel);
    void close();
    void requestMove();
    void requestResize();
    void getIcon(int32_t fd);
    void requestEnterVirtualDesktop(const char *id);
    void requestEnterNewVirtualDesktop();
    void requestLeaveVirtualDesktop(const char *id);
    void requestEnterActivity(const char *id);
    void requestLeaveActivity(const char *id);
    void sendToOutput(WlOutput *output);

    auto &titleChanged() { return titleChangedSignal_; }
    auto &appIdChanged() { return appIdChangedSignal_; }
    auto &stateChanged() { return stateChangedSignal_; }
    auto &virtualDesktopChanged() { return virtualDesktopChangedSignal_; }
    auto &themedIconNameChanged() { return themedIconNameChangedSignal_; }
    auto &unmapped() { return unmappedSignal_; }
    auto &initialState() { return initialStateSignal_; }
    auto &parentWindow() { return parentWindowSignal_; }
    auto &geometry() { return geometrySignal_; }
    auto &iconChanged() { return iconChangedSignal_; }
    auto &pidChanged() { return pidChangedSignal_; }
    auto &virtualDesktopEntered() { return virtualDesktopEnteredSignal_; }
    auto &virtualDesktopLeft() { return virtualDesktopLeftSignal_; }
    auto &applicationMenu() { return applicationMenuSignal_; }
    auto &activityEntered() { return activityEnteredSignal_; }
    auto &activityLeft() { return activityLeftSignal_; }
    auto &resourceNameChanged() { return resourceNameChangedSignal_; }

private:
    static void destructor(org_kde_plasma_window *);
    static const struct org_kde_plasma_window_listener listener;
    fcitx::Signal<void(const char *)> titleChangedSignal_;
    fcitx::Signal<void(const char *)> appIdChangedSignal_;
    fcitx::Signal<void(uint32_t)> stateChangedSignal_;
    fcitx::Signal<void(int32_t)> virtualDesktopChangedSignal_;
    fcitx::Signal<void(const char *)> themedIconNameChangedSignal_;
    fcitx::Signal<void()> unmappedSignal_;
    fcitx::Signal<void()> initialStateSignal_;
    fcitx::Signal<void(OrgKdePlasmaWindow *)> parentWindowSignal_;
    fcitx::Signal<void(int32_t, int32_t, uint32_t, uint32_t)> geometrySignal_;
    fcitx::Signal<void()> iconChangedSignal_;
    fcitx::Signal<void(uint32_t)> pidChangedSignal_;
    fcitx::Signal<void(const char *)> virtualDesktopEnteredSignal_;
    fcitx::Signal<void(const char *)> virtualDesktopLeftSignal_;
    fcitx::Signal<void(const char *, const char *)> applicationMenuSignal_;
    fcitx::Signal<void(const char *)> activityEnteredSignal_;
    fcitx::Signal<void(const char *)> activityLeftSignal_;
    fcitx::Signal<void(const char *)> resourceNameChangedSignal_;

    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<org_kde_plasma_window, &destructor> data_;
};
static inline org_kde_plasma_window *rawPointer(OrgKdePlasmaWindow *p) {
    return p ? static_cast<org_kde_plasma_window *>(*p) : nullptr;
}

} // namespace fcitx::wayland

#endif // ORG_KDE_PLASMA_WINDOW_H_
