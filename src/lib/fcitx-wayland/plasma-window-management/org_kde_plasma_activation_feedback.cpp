#include "org_kde_plasma_activation_feedback.h"
#include <cassert>
#include "org_kde_plasma_activation.h"
#include "wayland-plasma-window-management-client-protocol.h"

namespace fcitx::wayland {
const struct org_kde_plasma_activation_feedback_listener
    OrgKdePlasmaActivationFeedback::listener = {
        .activation =
            [](void *data, org_kde_plasma_activation_feedback *wldata,
               org_kde_plasma_activation *id) {
                auto *obj = static_cast<OrgKdePlasmaActivationFeedback *>(data);
                assert(*obj == wldata);
                {
                    auto *id_ = new OrgKdePlasmaActivation(id);
                    obj->activation()(id_);
                }
            },
};

OrgKdePlasmaActivationFeedback::OrgKdePlasmaActivationFeedback(
    org_kde_plasma_activation_feedback *data)
    : version_(org_kde_plasma_activation_feedback_get_version(data)),
      data_(data) {
    org_kde_plasma_activation_feedback_set_user_data(*this, this);
    org_kde_plasma_activation_feedback_add_listener(
        *this, &OrgKdePlasmaActivationFeedback::listener, this);
}

void OrgKdePlasmaActivationFeedback::destructor(
    org_kde_plasma_activation_feedback *data) {
    const auto version = org_kde_plasma_activation_feedback_get_version(data);
#if defined(ORG_KDE_PLASMA_ACTIVATION_FEEDBACK_DESTROY_SINCE_VERSION)
    if (version >= 1) {
        org_kde_plasma_activation_feedback_destroy(data);
        return;
    }
#endif
}

} // namespace fcitx::wayland
