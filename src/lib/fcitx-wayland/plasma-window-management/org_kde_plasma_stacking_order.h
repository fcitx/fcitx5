#ifndef ORG_KDE_PLASMA_STACKING_ORDER_H_
#define ORG_KDE_PLASMA_STACKING_ORDER_H_
#include <cstdint>
#include <wayland-client.h>
#include <wayland-util.h>
#include "fcitx-utils/misc.h"
#include "fcitx-utils/signals.h"
#include "wayland-plasma-window-management-client-protocol.h" // IWYU pragma: export
namespace fcitx::wayland {

class OrgKdePlasmaStackingOrder final {
public:
    static constexpr const char *interface = "org_kde_plasma_stacking_order";
    static constexpr const wl_interface *const wlInterface =
        &org_kde_plasma_stacking_order_interface;
    static constexpr const uint32_t version = 17;
    using wlType = org_kde_plasma_stacking_order;
    operator org_kde_plasma_stacking_order *() { return data_.get(); }
    OrgKdePlasmaStackingOrder(wlType *data);
    OrgKdePlasmaStackingOrder(OrgKdePlasmaStackingOrder &&other) noexcept =
        delete;
    OrgKdePlasmaStackingOrder &
    operator=(OrgKdePlasmaStackingOrder &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }

    auto &window() { return windowSignal_; }
    auto &done() { return doneSignal_; }

private:
    static void destructor(org_kde_plasma_stacking_order *);
    static const struct org_kde_plasma_stacking_order_listener listener;
    fcitx::Signal<void(const char *)> windowSignal_;
    fcitx::Signal<void()> doneSignal_;

    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<org_kde_plasma_stacking_order, &destructor> data_;
};
static inline org_kde_plasma_stacking_order *
rawPointer(OrgKdePlasmaStackingOrder *p) {
    return p ? static_cast<org_kde_plasma_stacking_order *>(*p) : nullptr;
}

} // namespace fcitx::wayland

#endif // ORG_KDE_PLASMA_STACKING_ORDER_H_
