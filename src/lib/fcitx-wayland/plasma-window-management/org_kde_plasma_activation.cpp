#include "org_kde_plasma_activation.h"
#include <cassert>
#include "wayland-plasma-window-management-client-protocol.h"

namespace fcitx::wayland {
const struct org_kde_plasma_activation_listener
    OrgKdePlasmaActivation::listener = {
        .app_id =
            [](void *data, org_kde_plasma_activation *wldata,
               const char *appId) {
                auto *obj = static_cast<OrgKdePlasmaActivation *>(data);
                assert(*obj == wldata);
                {
                    obj->appId()(appId);
                }
            },
        .finished =
            [](void *data, org_kde_plasma_activation *wldata) {
                auto *obj = static_cast<OrgKdePlasmaActivation *>(data);
                assert(*obj == wldata);
                {
                    obj->finished()();
                }
            },
};

OrgKdePlasmaActivation::OrgKdePlasmaActivation(org_kde_plasma_activation *data)
    : version_(org_kde_plasma_activation_get_version(data)), data_(data) {
    org_kde_plasma_activation_set_user_data(*this, this);
    org_kde_plasma_activation_add_listener(
        *this, &OrgKdePlasmaActivation::listener, this);
}

void OrgKdePlasmaActivation::destructor(org_kde_plasma_activation *data) {
    org_kde_plasma_activation_destroy(data);
}

} // namespace fcitx::wayland
