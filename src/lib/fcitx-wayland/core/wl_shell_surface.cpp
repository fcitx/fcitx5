#include "wl_shell_surface.h"
#include <cassert>
#include "wl_output.h"
#include "wl_seat.h"
#include "wl_surface.h"

namespace fcitx::wayland {
const struct wl_shell_surface_listener WlShellSurface::listener = {
#if defined(WL_SHELL_SURFACE_PING_SINCE_VERSION)
    .ping =
        [](void *data, wl_shell_surface *wldata, uint32_t serial) {
            auto *obj = static_cast<WlShellSurface *>(data);
            assert(*obj == wldata);
            {
                obj->ping()(serial);
            }
        },
#endif
#if defined(WL_SHELL_SURFACE_CONFIGURE_SINCE_VERSION)
    .configure =
        [](void *data, wl_shell_surface *wldata, uint32_t edges, int32_t width,
           int32_t height) {
            auto *obj = static_cast<WlShellSurface *>(data);
            assert(*obj == wldata);
            {
                obj->configure()(edges, width, height);
            }
        },
#endif
#if defined(WL_SHELL_SURFACE_POPUP_DONE_SINCE_VERSION)
    .popup_done =
        [](void *data, wl_shell_surface *wldata) {
            auto *obj = static_cast<WlShellSurface *>(data);
            assert(*obj == wldata);
            {
                obj->popupDone()();
            }
        },
#endif
};

WlShellSurface::WlShellSurface(wl_shell_surface *data)
    : version_(wl_shell_surface_get_version(data)), data_(data) {
    wl_shell_surface_set_user_data(*this, this);
    wl_shell_surface_add_listener(*this, &WlShellSurface::listener, this);
}

void WlShellSurface::destructor(wl_shell_surface *data) {
    wl_shell_surface_destroy(data);
}
#if defined(WL_SHELL_SURFACE_PONG_SINCE_VERSION)
void WlShellSurface::pong(uint32_t serial) {
    wl_shell_surface_pong(*this, serial);
}
#endif
#if defined(WL_SHELL_SURFACE_MOVE_SINCE_VERSION)
void WlShellSurface::move(WlSeat *seat, uint32_t serial) {
    wl_shell_surface_move(*this, rawPointer(seat), serial);
}
#endif
#if defined(WL_SHELL_SURFACE_RESIZE_SINCE_VERSION)
void WlShellSurface::resize(WlSeat *seat, uint32_t serial, uint32_t edges) {
    wl_shell_surface_resize(*this, rawPointer(seat), serial, edges);
}
#endif
#if defined(WL_SHELL_SURFACE_SET_TOPLEVEL_SINCE_VERSION)
void WlShellSurface::setToplevel() { wl_shell_surface_set_toplevel(*this); }
#endif
#if defined(WL_SHELL_SURFACE_SET_TRANSIENT_SINCE_VERSION)
void WlShellSurface::setTransient(WlSurface *parent, int32_t x, int32_t y,
                                  uint32_t flags) {
    wl_shell_surface_set_transient(*this, rawPointer(parent), x, y, flags);
}
#endif
#if defined(WL_SHELL_SURFACE_SET_FULLSCREEN_SINCE_VERSION)
void WlShellSurface::setFullscreen(uint32_t method, uint32_t framerate,
                                   WlOutput *output) {
    wl_shell_surface_set_fullscreen(*this, method, framerate,
                                    rawPointer(output));
}
#endif
#if defined(WL_SHELL_SURFACE_SET_POPUP_SINCE_VERSION)
void WlShellSurface::setPopup(WlSeat *seat, uint32_t serial, WlSurface *parent,
                              int32_t x, int32_t y, uint32_t flags) {
    wl_shell_surface_set_popup(*this, rawPointer(seat), serial,
                               rawPointer(parent), x, y, flags);
}
#endif
#if defined(WL_SHELL_SURFACE_SET_MAXIMIZED_SINCE_VERSION)
void WlShellSurface::setMaximized(WlOutput *output) {
    wl_shell_surface_set_maximized(*this, rawPointer(output));
}
#endif
#if defined(WL_SHELL_SURFACE_SET_TITLE_SINCE_VERSION)
void WlShellSurface::setTitle(const char *title) {
    wl_shell_surface_set_title(*this, title);
}
#endif
#if defined(WL_SHELL_SURFACE_SET_CLASS_SINCE_VERSION)
void WlShellSurface::setClass(const char *class_) {
    wl_shell_surface_set_class(*this, class_);
}
#endif

} // namespace fcitx::wayland
