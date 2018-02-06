#include "wl_shell_surface.h"
#include "wl_output.h"
#include "wl_seat.h"
#include "wl_surface.h"
#include <cassert>
namespace fcitx {
namespace wayland {
constexpr const char *WlShellSurface::interface;
constexpr const wl_interface *const WlShellSurface::wlInterface;
const uint32_t WlShellSurface::version;
const struct wl_shell_surface_listener WlShellSurface::listener = {
    [](void *data, wl_shell_surface *wldata, uint32_t serial) {
        auto obj = static_cast<WlShellSurface *>(data);
        assert(*obj == wldata);
        { return obj->ping()(serial); }
    },
    [](void *data, wl_shell_surface *wldata, uint32_t edges, int32_t width,
       int32_t height) {
        auto obj = static_cast<WlShellSurface *>(data);
        assert(*obj == wldata);
        { return obj->configure()(edges, width, height); }
    },
    [](void *data, wl_shell_surface *wldata) {
        auto obj = static_cast<WlShellSurface *>(data);
        assert(*obj == wldata);
        { return obj->popupDone()(); }
    },
};
WlShellSurface::WlShellSurface(wl_shell_surface *data)
    : version_(wl_shell_surface_get_version(data)),
      data_(data, &WlShellSurface::destructor) {
    wl_shell_surface_set_user_data(*this, this);
    wl_shell_surface_add_listener(*this, &WlShellSurface::listener, this);
}
void WlShellSurface::destructor(wl_shell_surface *data) {
    { return wl_shell_surface_destroy(data); }
}
void WlShellSurface::pong(uint32_t serial) {
    return wl_shell_surface_pong(*this, serial);
}
void WlShellSurface::move(WlSeat *seat, uint32_t serial) {
    return wl_shell_surface_move(*this, rawPointer(seat), serial);
}
void WlShellSurface::resize(WlSeat *seat, uint32_t serial, uint32_t edges) {
    return wl_shell_surface_resize(*this, rawPointer(seat), serial, edges);
}
void WlShellSurface::setToplevel() {
    return wl_shell_surface_set_toplevel(*this);
}
void WlShellSurface::setTransient(WlSurface *parent, int32_t x, int32_t y,
                                  uint32_t flags) {
    return wl_shell_surface_set_transient(*this, rawPointer(parent), x, y,
                                          flags);
}
void WlShellSurface::setFullscreen(uint32_t method, uint32_t framerate,
                                   WlOutput *output) {
    return wl_shell_surface_set_fullscreen(*this, method, framerate,
                                           rawPointer(output));
}
void WlShellSurface::setPopup(WlSeat *seat, uint32_t serial, WlSurface *parent,
                              int32_t x, int32_t y, uint32_t flags) {
    return wl_shell_surface_set_popup(*this, rawPointer(seat), serial,
                                      rawPointer(parent), x, y, flags);
}
void WlShellSurface::setMaximized(WlOutput *output) {
    return wl_shell_surface_set_maximized(*this, rawPointer(output));
}
void WlShellSurface::setTitle(const char *title) {
    return wl_shell_surface_set_title(*this, title);
}
void WlShellSurface::setClass(const char *class_) {
    return wl_shell_surface_set_class(*this, class_);
}
} // namespace wayland
} // namespace fcitx
