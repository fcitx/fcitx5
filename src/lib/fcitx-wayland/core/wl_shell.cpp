#include "wl_shell.h"
#include <cassert>
#include "wl_shell_surface.h"
#include "wl_surface.h"
namespace fcitx::wayland {
WlShell::WlShell(wl_shell *data)
    : version_(wl_shell_get_version(data)), data_(data) {
    wl_shell_set_user_data(*this, this);
}
void WlShell::destructor(wl_shell *data) {
    { return wl_shell_destroy(data); }
}
WlShellSurface *WlShell::getShellSurface(WlSurface *surface) {
    return new WlShellSurface(
        wl_shell_get_shell_surface(*this, rawPointer(surface)));
}
} // namespace fcitx::wayland
