#include "org_kde_plasma_stacking_order.h"
#include <cassert>
#include "wayland-plasma-window-management-client-protocol.h"

namespace fcitx::wayland {
const struct org_kde_plasma_stacking_order_listener
    OrgKdePlasmaStackingOrder::listener = {
        .window =
            [](void *data, org_kde_plasma_stacking_order *wldata,
               const char *uuid) {
                auto *obj = static_cast<OrgKdePlasmaStackingOrder *>(data);
                assert(*obj == wldata);
                {
                    obj->window()(uuid);
                }
            },
        .done =
            [](void *data, org_kde_plasma_stacking_order *wldata) {
                auto *obj = static_cast<OrgKdePlasmaStackingOrder *>(data);
                assert(*obj == wldata);
                {
                    obj->done()();
                }
            },
};

OrgKdePlasmaStackingOrder::OrgKdePlasmaStackingOrder(
    org_kde_plasma_stacking_order *data)
    : version_(org_kde_plasma_stacking_order_get_version(data)), data_(data) {
    org_kde_plasma_stacking_order_set_user_data(*this, this);
    org_kde_plasma_stacking_order_add_listener(
        *this, &OrgKdePlasmaStackingOrder::listener, this);
}

void OrgKdePlasmaStackingOrder::destructor(
    org_kde_plasma_stacking_order *data) {
    org_kde_plasma_stacking_order_destroy(data);
}

} // namespace fcitx::wayland
