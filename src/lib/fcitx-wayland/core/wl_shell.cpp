#include "wl_shell.h"
#include <cassert>
#include "wl_shell_surface.h"
#include "wl_surface.h"
namespace fcitx {
namespace wayland {
constexpr const char *WlShell::interface;
constexpr const wl_interface *const WlShell::wlInterface;
const uint32_t WlShell::version;
WlShell::WlShell(wl_shell *data)
    : version_(wl_shell_get_version(data)), data_(data, &WlShell::destructor) {
    wl_shell_set_user_data(*this, this);
}
void WlShell::destructor(wl_shell *data) {
    { return wl_shell_destroy(data); }
}
WlShellSurface *WlShell::getShellSurface(WlSurface *surface) {
    return new WlShellSurface(
        wl_shell_get_shell_surface(*this, rawPointer(surface)));
}
} // namespace wayland
} // namespace fcitx
