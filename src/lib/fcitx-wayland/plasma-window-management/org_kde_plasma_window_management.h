#ifndef ORG_KDE_PLASMA_WINDOW_MANAGEMENT
#define ORG_KDE_PLASMA_WINDOW_MANAGEMENT
#include <wayland-client.h>
#include "fcitx-utils/signals.h"
#include "wayland-plasma-window-management-client-protocol.h"
namespace fcitx::wayland {
class OrgKdePlasmaWindow;
class OrgKdePlasmaWindowManagement final {
public:
    static constexpr const char *interface = "org_kde_plasma_window_management";
    static constexpr const wl_interface *const wlInterface =
        &org_kde_plasma_window_management_interface;
    static constexpr const uint32_t version = 16;
    typedef org_kde_plasma_window_management wlType;
    operator org_kde_plasma_window_management *() { return data_.get(); }
    OrgKdePlasmaWindowManagement(wlType *data);
    OrgKdePlasmaWindowManagement(
        OrgKdePlasmaWindowManagement &&other) noexcept = delete;
    OrgKdePlasmaWindowManagement &
    operator=(OrgKdePlasmaWindowManagement &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    void showDesktop(uint32_t state);
    OrgKdePlasmaWindow *getWindow(uint32_t internalWindowId);
    OrgKdePlasmaWindow *getWindowByUuid(const char *internalWindowUuid);
    auto &showDesktopChanged() { return showDesktopChangedSignal_; }
    auto &window() { return windowSignal_; }
    auto &stackingOrderChanged() { return stackingOrderChangedSignal_; }
    auto &stackingOrderUuidChanged() { return stackingOrderUuidChangedSignal_; }
    auto &windowWithUuid() { return windowWithUuidSignal_; }

private:
    static void destructor(org_kde_plasma_window_management *);
    static const struct org_kde_plasma_window_management_listener listener;
    fcitx::Signal<void(uint32_t)> showDesktopChangedSignal_;
    fcitx::Signal<void(uint32_t)> windowSignal_;
    fcitx::Signal<void(wl_array *)> stackingOrderChangedSignal_;
    fcitx::Signal<void(const char *)> stackingOrderUuidChangedSignal_;
    fcitx::Signal<void(uint32_t, const char *)> windowWithUuidSignal_;
    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<org_kde_plasma_window_management, &destructor> data_;
};
static inline org_kde_plasma_window_management *
rawPointer(OrgKdePlasmaWindowManagement *p) {
    return p ? static_cast<org_kde_plasma_window_management *>(*p) : nullptr;
}
} // namespace fcitx::wayland
#endif
