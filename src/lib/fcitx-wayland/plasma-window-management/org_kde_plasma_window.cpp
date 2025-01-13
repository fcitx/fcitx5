#include "org_kde_plasma_window.h"
#include <cassert>
#include "wl_output.h"
#include "wl_surface.h"
namespace fcitx::wayland {
const struct org_kde_plasma_window_listener OrgKdePlasmaWindow::listener = {
    [](void *data, org_kde_plasma_window *wldata, const char *title) {
        auto *obj = static_cast<OrgKdePlasmaWindow *>(data);
        assert(*obj == wldata);
        {
            return obj->titleChanged()(title);
        }
    },
    [](void *data, org_kde_plasma_window *wldata, const char *appId) {
        auto *obj = static_cast<OrgKdePlasmaWindow *>(data);
        assert(*obj == wldata);
        {
            return obj->appIdChanged()(appId);
        }
    },
    [](void *data, org_kde_plasma_window *wldata, uint32_t flags) {
        auto *obj = static_cast<OrgKdePlasmaWindow *>(data);
        assert(*obj == wldata);
        {
            return obj->stateChanged()(flags);
        }
    },
    [](void *data, org_kde_plasma_window *wldata, int32_t number) {
        auto *obj = static_cast<OrgKdePlasmaWindow *>(data);
        assert(*obj == wldata);
        {
            return obj->virtualDesktopChanged()(number);
        }
    },
    [](void *data, org_kde_plasma_window *wldata, const char *name) {
        auto *obj = static_cast<OrgKdePlasmaWindow *>(data);
        assert(*obj == wldata);
        {
            return obj->themedIconNameChanged()(name);
        }
    },
    [](void *data, org_kde_plasma_window *wldata) {
        auto *obj = static_cast<OrgKdePlasmaWindow *>(data);
        assert(*obj == wldata);
        {
            return obj->unmapped()();
        }
    },
    [](void *data, org_kde_plasma_window *wldata) {
        auto *obj = static_cast<OrgKdePlasmaWindow *>(data);
        assert(*obj == wldata);
        {
            return obj->initialState()();
        }
    },
    [](void *data, org_kde_plasma_window *wldata,
       org_kde_plasma_window *parent) {
        auto *obj = static_cast<OrgKdePlasmaWindow *>(data);
        assert(*obj == wldata);
        {
            auto *parent_ =
                parent ? static_cast<OrgKdePlasmaWindow *>(
                             org_kde_plasma_window_get_user_data(parent))
                       : nullptr;
            return obj->parentWindow()(parent_);
        }
    },
    [](void *data, org_kde_plasma_window *wldata, int32_t x, int32_t y,
       uint32_t width, uint32_t height) {
        auto *obj = static_cast<OrgKdePlasmaWindow *>(data);
        assert(*obj == wldata);
        {
            return obj->geometry()(x, y, width, height);
        }
    },
    [](void *data, org_kde_plasma_window *wldata) {
        auto *obj = static_cast<OrgKdePlasmaWindow *>(data);
        assert(*obj == wldata);
        {
            return obj->iconChanged()();
        }
    },
    [](void *data, org_kde_plasma_window *wldata, uint32_t pid) {
        auto *obj = static_cast<OrgKdePlasmaWindow *>(data);
        assert(*obj == wldata);
        {
            return obj->pidChanged()(pid);
        }
    },
    [](void *data, org_kde_plasma_window *wldata, const char *id) {
        auto *obj = static_cast<OrgKdePlasmaWindow *>(data);
        assert(*obj == wldata);
        {
            return obj->virtualDesktopEntered()(id);
        }
    },
    [](void *data, org_kde_plasma_window *wldata, const char *is) {
        auto *obj = static_cast<OrgKdePlasmaWindow *>(data);
        assert(*obj == wldata);
        {
            return obj->virtualDesktopLeft()(is);
        }
    },
    [](void *data, org_kde_plasma_window *wldata, const char *serviceName,
       const char *objectPath) {
        auto *obj = static_cast<OrgKdePlasmaWindow *>(data);
        assert(*obj == wldata);
        {
            return obj->applicationMenu()(serviceName, objectPath);
        }
    },
    [](void *data, org_kde_plasma_window *wldata, const char *id) {
        auto *obj = static_cast<OrgKdePlasmaWindow *>(data);
        assert(*obj == wldata);
        {
            return obj->activityEntered()(id);
        }
    },
    [](void *data, org_kde_plasma_window *wldata, const char *id) {
        auto *obj = static_cast<OrgKdePlasmaWindow *>(data);
        assert(*obj == wldata);
        {
            return obj->activityLeft()(id);
        }
    },
    [](void *data, org_kde_plasma_window *wldata, const char *resourceName) {
        auto *obj = static_cast<OrgKdePlasmaWindow *>(data);
        assert(*obj == wldata);
        {
            return obj->resourceNameChanged()(resourceName);
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
    auto version = org_kde_plasma_window_get_version(data);
    if (version >= 4) {
        return org_kde_plasma_window_destroy(data);
    }
}
void OrgKdePlasmaWindow::setState(uint32_t flags, uint32_t state) {
    return org_kde_plasma_window_set_state(*this, flags, state);
}
void OrgKdePlasmaWindow::setVirtualDesktop(uint32_t number) {
    return org_kde_plasma_window_set_virtual_desktop(*this, number);
}
void OrgKdePlasmaWindow::setMinimizedGeometry(WlSurface *panel, uint32_t x,
                                              uint32_t y, uint32_t width,
                                              uint32_t height) {
    return org_kde_plasma_window_set_minimized_geometry(
        *this, rawPointer(panel), x, y, width, height);
}
void OrgKdePlasmaWindow::unsetMinimizedGeometry(WlSurface *panel) {
    return org_kde_plasma_window_unset_minimized_geometry(*this,
                                                          rawPointer(panel));
}
void OrgKdePlasmaWindow::close() { return org_kde_plasma_window_close(*this); }
void OrgKdePlasmaWindow::requestMove() {
    return org_kde_plasma_window_request_move(*this);
}
void OrgKdePlasmaWindow::requestResize() {
    return org_kde_plasma_window_request_resize(*this);
}
void OrgKdePlasmaWindow::getIcon(int32_t fd) {
    return org_kde_plasma_window_get_icon(*this, fd);
}
void OrgKdePlasmaWindow::requestEnterVirtualDesktop(const char *id) {
    return org_kde_plasma_window_request_enter_virtual_desktop(*this, id);
}
void OrgKdePlasmaWindow::requestEnterNewVirtualDesktop() {
    return org_kde_plasma_window_request_enter_new_virtual_desktop(*this);
}
void OrgKdePlasmaWindow::requestLeaveVirtualDesktop(const char *id) {
    return org_kde_plasma_window_request_leave_virtual_desktop(*this, id);
}
void OrgKdePlasmaWindow::requestEnterActivity(const char *id) {
    return org_kde_plasma_window_request_enter_activity(*this, id);
}
void OrgKdePlasmaWindow::requestLeaveActivity(const char *id) {
    return org_kde_plasma_window_request_leave_activity(*this, id);
}
void OrgKdePlasmaWindow::sendToOutput(WlOutput *output) {
    return org_kde_plasma_window_send_to_output(*this, rawPointer(output));
}
} // namespace fcitx::wayland
