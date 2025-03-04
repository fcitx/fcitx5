#include "org_kde_plasma_window.h"
#include <cassert>
#include "wayland-plasma-window-management-client-protocol.h"
#include "wl_output.h"
#include "wl_surface.h"

namespace fcitx::wayland {
const struct org_kde_plasma_window_listener OrgKdePlasmaWindow::listener = {
    .title_changed =
        [](void *data, org_kde_plasma_window *wldata, const char *title) {
            auto *obj = static_cast<OrgKdePlasmaWindow *>(data);
            assert(*obj == wldata);
            {
                obj->titleChanged()(title);
            }
        },
    .app_id_changed =
        [](void *data, org_kde_plasma_window *wldata, const char *appId) {
            auto *obj = static_cast<OrgKdePlasmaWindow *>(data);
            assert(*obj == wldata);
            {
                obj->appIdChanged()(appId);
            }
        },
    .state_changed =
        [](void *data, org_kde_plasma_window *wldata, uint32_t flags) {
            auto *obj = static_cast<OrgKdePlasmaWindow *>(data);
            assert(*obj == wldata);
            {
                obj->stateChanged()(flags);
            }
        },
    .virtual_desktop_changed =
        [](void *data, org_kde_plasma_window *wldata, int32_t number) {
            auto *obj = static_cast<OrgKdePlasmaWindow *>(data);
            assert(*obj == wldata);
            {
                obj->virtualDesktopChanged()(number);
            }
        },
    .themed_icon_name_changed =
        [](void *data, org_kde_plasma_window *wldata, const char *name) {
            auto *obj = static_cast<OrgKdePlasmaWindow *>(data);
            assert(*obj == wldata);
            {
                obj->themedIconNameChanged()(name);
            }
        },
    .unmapped =
        [](void *data, org_kde_plasma_window *wldata) {
            auto *obj = static_cast<OrgKdePlasmaWindow *>(data);
            assert(*obj == wldata);
            {
                obj->unmapped()();
            }
        },
    .initial_state =
        [](void *data, org_kde_plasma_window *wldata) {
            auto *obj = static_cast<OrgKdePlasmaWindow *>(data);
            assert(*obj == wldata);
            {
                obj->initialState()();
            }
        },
    .parent_window =
        [](void *data, org_kde_plasma_window *wldata,
           org_kde_plasma_window *parent) {
            auto *obj = static_cast<OrgKdePlasmaWindow *>(data);
            assert(*obj == wldata);
            {
                auto *parent_ =
                    parent ? static_cast<OrgKdePlasmaWindow *>(
                                 org_kde_plasma_window_get_user_data(parent))
                           : nullptr;
                obj->parentWindow()(parent_);
            }
        },
    .geometry =
        [](void *data, org_kde_plasma_window *wldata, int32_t x, int32_t y,
           uint32_t width, uint32_t height) {
            auto *obj = static_cast<OrgKdePlasmaWindow *>(data);
            assert(*obj == wldata);
            {
                obj->geometry()(x, y, width, height);
            }
        },
    .icon_changed =
        [](void *data, org_kde_plasma_window *wldata) {
            auto *obj = static_cast<OrgKdePlasmaWindow *>(data);
            assert(*obj == wldata);
            {
                obj->iconChanged()();
            }
        },
    .pid_changed =
        [](void *data, org_kde_plasma_window *wldata, uint32_t pid) {
            auto *obj = static_cast<OrgKdePlasmaWindow *>(data);
            assert(*obj == wldata);
            {
                obj->pidChanged()(pid);
            }
        },
    .virtual_desktop_entered =
        [](void *data, org_kde_plasma_window *wldata, const char *id) {
            auto *obj = static_cast<OrgKdePlasmaWindow *>(data);
            assert(*obj == wldata);
            {
                obj->virtualDesktopEntered()(id);
            }
        },
    .virtual_desktop_left =
        [](void *data, org_kde_plasma_window *wldata, const char *is) {
            auto *obj = static_cast<OrgKdePlasmaWindow *>(data);
            assert(*obj == wldata);
            {
                obj->virtualDesktopLeft()(is);
            }
        },
    .application_menu =
        [](void *data, org_kde_plasma_window *wldata, const char *serviceName,
           const char *objectPath) {
            auto *obj = static_cast<OrgKdePlasmaWindow *>(data);
            assert(*obj == wldata);
            {
                obj->applicationMenu()(serviceName, objectPath);
            }
        },
    .activity_entered =
        [](void *data, org_kde_plasma_window *wldata, const char *id) {
            auto *obj = static_cast<OrgKdePlasmaWindow *>(data);
            assert(*obj == wldata);
            {
                obj->activityEntered()(id);
            }
        },
    .activity_left =
        [](void *data, org_kde_plasma_window *wldata, const char *id) {
            auto *obj = static_cast<OrgKdePlasmaWindow *>(data);
            assert(*obj == wldata);
            {
                obj->activityLeft()(id);
            }
        },
    .resource_name_changed =
        [](void *data, org_kde_plasma_window *wldata,
           const char *resourceName) {
            auto *obj = static_cast<OrgKdePlasmaWindow *>(data);
            assert(*obj == wldata);
            {
                obj->resourceNameChanged()(resourceName);
            }
        },
};

OrgKdePlasmaWindow::OrgKdePlasmaWindow(org_kde_plasma_window *data)
    : version_(org_kde_plasma_window_get_version(data)), data_(data) {
    org_kde_plasma_window_set_user_data(*this, this);
    org_kde_plasma_window_add_listener(*this, &OrgKdePlasmaWindow::listener,
                                       this);
}

void OrgKdePlasmaWindow::destructor(org_kde_plasma_window *data) {
    const auto version = org_kde_plasma_window_get_version(data);
    if (version >= 4) {
        org_kde_plasma_window_destroy(data);
        return;
    }
}
void OrgKdePlasmaWindow::setState(uint32_t flags, uint32_t state) {
    org_kde_plasma_window_set_state(*this, flags, state);
}
void OrgKdePlasmaWindow::setVirtualDesktop(uint32_t number) {
    org_kde_plasma_window_set_virtual_desktop(*this, number);
}
void OrgKdePlasmaWindow::setMinimizedGeometry(WlSurface *panel, uint32_t x,
                                              uint32_t y, uint32_t width,
                                              uint32_t height) {
    org_kde_plasma_window_set_minimized_geometry(*this, rawPointer(panel), x, y,
                                                 width, height);
}
void OrgKdePlasmaWindow::unsetMinimizedGeometry(WlSurface *panel) {
    org_kde_plasma_window_unset_minimized_geometry(*this, rawPointer(panel));
}
void OrgKdePlasmaWindow::close() { org_kde_plasma_window_close(*this); }
void OrgKdePlasmaWindow::requestMove() {
    org_kde_plasma_window_request_move(*this);
}
void OrgKdePlasmaWindow::requestResize() {
    org_kde_plasma_window_request_resize(*this);
}
void OrgKdePlasmaWindow::getIcon(int32_t fd) {
    org_kde_plasma_window_get_icon(*this, fd);
}
void OrgKdePlasmaWindow::requestEnterVirtualDesktop(const char *id) {
    org_kde_plasma_window_request_enter_virtual_desktop(*this, id);
}
void OrgKdePlasmaWindow::requestEnterNewVirtualDesktop() {
    org_kde_plasma_window_request_enter_new_virtual_desktop(*this);
}
void OrgKdePlasmaWindow::requestLeaveVirtualDesktop(const char *id) {
    org_kde_plasma_window_request_leave_virtual_desktop(*this, id);
}
void OrgKdePlasmaWindow::requestEnterActivity(const char *id) {
    org_kde_plasma_window_request_enter_activity(*this, id);
}
void OrgKdePlasmaWindow::requestLeaveActivity(const char *id) {
    org_kde_plasma_window_request_leave_activity(*this, id);
}
void OrgKdePlasmaWindow::sendToOutput(WlOutput *output) {
    org_kde_plasma_window_send_to_output(*this, rawPointer(output));
}
} // namespace fcitx::wayland
