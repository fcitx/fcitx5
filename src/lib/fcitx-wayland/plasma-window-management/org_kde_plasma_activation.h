#ifndef ORG_KDE_PLASMA_ACTIVATION_H_
#define ORG_KDE_PLASMA_ACTIVATION_H_
#include <cstdint>
#include <wayland-client.h>
#include <wayland-util.h>
#include "fcitx-utils/misc.h"
#include "fcitx-utils/signals.h"
#include "wayland-plasma-window-management-client-protocol.h" // IWYU pragma: export
namespace fcitx::wayland {

class OrgKdePlasmaActivation final {
public:
    static constexpr const char *interface = "org_kde_plasma_activation";
    static constexpr const wl_interface *const wlInterface =
        &org_kde_plasma_activation_interface;
    static constexpr const uint32_t version = 1;
    using wlType = org_kde_plasma_activation;
    operator org_kde_plasma_activation *() { return data_.get(); }
    OrgKdePlasmaActivation(wlType *data);
    OrgKdePlasmaActivation(OrgKdePlasmaActivation &&other) noexcept = delete;
    OrgKdePlasmaActivation &
    operator=(OrgKdePlasmaActivation &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }

    auto &appId() { return appIdSignal_; }
    auto &finished() { return finishedSignal_; }

private:
    static void destructor(org_kde_plasma_activation *);
    static const struct org_kde_plasma_activation_listener listener;
    fcitx::Signal<void(const char *)> appIdSignal_;
    fcitx::Signal<void()> finishedSignal_;

    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<org_kde_plasma_activation, &destructor> data_;
};
static inline org_kde_plasma_activation *rawPointer(OrgKdePlasmaActivation *p) {
    return p ? static_cast<org_kde_plasma_activation *>(*p) : nullptr;
}

} // namespace fcitx::wayland

#endif // ORG_KDE_PLASMA_ACTIVATION_H_
