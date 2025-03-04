#ifndef WL_SHELL_H_
#define WL_SHELL_H_
#include <cstdint>
#include <wayland-client.h>
#include <wayland-util.h>
#include "fcitx-utils/misc.h"
namespace fcitx::wayland {

class WlShellSurface;
class WlSurface;

class WlShell final {
public:
    static constexpr const char *interface = "wl_shell";
    static constexpr const wl_interface *const wlInterface =
        &wl_shell_interface;
    static constexpr const uint32_t version = 1;
    using wlType = wl_shell;
    operator wl_shell *() { return data_.get(); }
    WlShell(wlType *data);
    WlShell(WlShell &&other) noexcept = delete;
    WlShell &operator=(WlShell &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    WlShellSurface *getShellSurface(WlSurface *surface);

private:
    static void destructor(wl_shell *);

    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<wl_shell, &destructor> data_;
};
static inline wl_shell *rawPointer(WlShell *p) {
    return p ? static_cast<wl_shell *>(*p) : nullptr;
}

} // namespace fcitx::wayland

#endif // WL_SHELL_H_
