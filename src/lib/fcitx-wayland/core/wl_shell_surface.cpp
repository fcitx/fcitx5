#include "wl_shell_surface.h"
#include <cassert>
#include "wl_output.h"
#include "wl_seat.h"
#include "wl_surface.h"

namespace fcitx::wayland {
const struct wl_shell_surface_listener WlShellSurface::listener = {
    .ping =
        [](void *data, wl_shell_surface *wldata, uint32_t serial) {
            auto *obj = static_cast<WlShellSurface *>(data);
            assert(*obj == wldata);
            {
                obj->ping()(serial);
            }
        },
    .configure =
        [](void *data, wl_shell_surface *wldata, uint32_t edges, int32_t width,
           int32_t height) {
            auto *obj = static_cast<WlShellSurface *>(data);
            assert(*obj == wldata);
            {
                obj->configure()(edges, width, height);
            }
        },
    .popup_done =
        [](void *data, wl_shell_surface *wldata) {
            auto *obj = static_cast<WlShellSurface *>(data);
            assert(*obj == wldata);
            {
                obj->popupDone()();
            }
        },
};

WlShellSurface::WlShellSurface(wl_shell_surface *data)
    : version_(wl_shell_surface_get_version(data)), data_(data) {
    wl_shell_surface_set_user_data(*this, this);
    wl_shell_surface_add_listener(*this, &WlShellSurface::listener, this);
}

void WlShellSurface::destructor(wl_shell_surface *data) {
    wl_shell_surface_destroy(data);
}
void WlShellSurface::pong(uint32_t serial) {
    wl_shell_surface_pong(*this, serial);
}
void WlShellSurface::move(WlSeat *seat, uint32_t serial) {
    wl_shell_surface_move(*this, rawPointer(seat), serial);
}
void WlShellSurface::resize(WlSeat *seat, uint32_t serial, uint32_t edges) {
    wl_shell_surface_resize(*this, rawPointer(seat), serial, edges);
}
void WlShellSurface::setToplevel() { wl_shell_surface_set_toplevel(*this); }
void WlShellSurface::setTransient(WlSurface *parent, int32_t x, int32_t y,
                                  uint32_t flags) {
    wl_shell_surface_set_transient(*this, rawPointer(parent), x, y, flags);
}
void WlShellSurface::setFullscreen(uint32_t method, uint32_t framerate,
                                   WlOutput *output) {
    wl_shell_surface_set_fullscreen(*this, method, framerate,
                                    rawPointer(output));
}
void WlShellSurface::setPopup(WlSeat *seat, uint32_t serial, WlSurface *parent,
                              int32_t x, int32_t y, uint32_t flags) {
    wl_shell_surface_set_popup(*this, rawPointer(seat), serial,
                               rawPointer(parent), x, y, flags);
}
void WlShellSurface::setMaximized(WlOutput *output) {
    wl_shell_surface_set_maximized(*this, rawPointer(output));
}
void WlShellSurface::setTitle(const char *title) {
    wl_shell_surface_set_title(*this, title);
}
void WlShellSurface::setClass(const char *class_) {
    wl_shell_surface_set_class(*this, class_);
}

} // namespace fcitx::wayland
