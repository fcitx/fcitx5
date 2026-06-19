#ifndef ORG_KDE_PLASMA_ACTIVATION_FEEDBACK_H_
#define ORG_KDE_PLASMA_ACTIVATION_FEEDBACK_H_
#include <cstdint>
#include <wayland-client.h>
#include <wayland-util.h>
#include "fcitx-utils/misc.h"
#include "fcitx-utils/signals.h"
#include "wayland-plasma-window-management-client-protocol.h" // IWYU pragma: export
namespace fcitx::wayland {

class OrgKdePlasmaActivation;

class OrgKdePlasmaActivationFeedback final {
public:
    static constexpr const char *interface =
        "org_kde_plasma_activation_feedback";
    static constexpr const wl_interface *const wlInterface =
        &org_kde_plasma_activation_feedback_interface;
    static constexpr const uint32_t version = 1;
    using wlType = org_kde_plasma_activation_feedback;
    operator org_kde_plasma_activation_feedback *() { return data_.get(); }
    OrgKdePlasmaActivationFeedback(wlType *data);
    OrgKdePlasmaActivationFeedback(
        OrgKdePlasmaActivationFeedback &&other) noexcept = delete;
    OrgKdePlasmaActivationFeedback &
    operator=(OrgKdePlasmaActivationFeedback &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }

    auto &activation() { return activationSignal_; }

private:
    static void destructor(org_kde_plasma_activation_feedback *);
    static const struct org_kde_plasma_activation_feedback_listener listener;
    fcitx::Signal<void(OrgKdePlasmaActivation *)> activationSignal_;

    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<org_kde_plasma_activation_feedback, &destructor> data_;
};
static inline org_kde_plasma_activation_feedback *
rawPointer(OrgKdePlasmaActivationFeedback *p) {
    return p ? static_cast<org_kde_plasma_activation_feedback *>(*p) : nullptr;
}

} // namespace fcitx::wayland

#endif // ORG_KDE_PLASMA_ACTIVATION_FEEDBACK_H_
