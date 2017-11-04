#ifndef WL_SHELL
#define WL_SHELL
#include "fcitx-utils/signals.h"
#include <memory>
#include <wayland-client.h>
namespace fcitx {
namespace wayland {
class WlShellSurface;
class WlSurface;
class WlShell final {
public:
    static constexpr const char *interface = "wl_shell";
    static constexpr const wl_interface *const wlInterface =
        &wl_shell_interface;
    static constexpr const uint32_t version = 1;
    typedef wl_shell wlType;
    operator wl_shell *() { return data_.get(); }
    WlShell(wlType *data);
    WlShell(WlShell &&other) noexcept = delete;
    WlShell &operator=(WlShell &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    WlShellSurface *getShellSurface(WlSurface *surface);

private:
    static void destructor(wl_shell *);
    uint32_t version_;
    std::unique_ptr<wl_shell, decltype(&destructor)> data_;
};
static inline wl_shell *rawPointer(WlShell *p) {
    return p ? static_cast<wl_shell *>(*p) : nullptr;
}
}
}
#endif
