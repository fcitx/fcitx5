#ifndef WL_SHELL_SURFACE_H_
#define WL_SHELL_SURFACE_H_
#include <cstdint>
#include <wayland-client.h>
#include <wayland-util.h>
#include "fcitx-utils/misc.h"
#include "fcitx-utils/signals.h"
namespace fcitx::wayland {

class WlOutput;
class WlSeat;
class WlSurface;

class WlShellSurface final {
public:
    static constexpr const char *interface = "wl_shell_surface";
    static constexpr const wl_interface *const wlInterface =
        &wl_shell_surface_interface;
    static constexpr const uint32_t version = 1;
    using wlType = wl_shell_surface;
    operator wl_shell_surface *() { return data_.get(); }
    WlShellSurface(wlType *data);
    WlShellSurface(WlShellSurface &&other) noexcept = delete;
    WlShellSurface &operator=(WlShellSurface &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    void pong(uint32_t serial);
    void move(WlSeat *seat, uint32_t serial);
    void resize(WlSeat *seat, uint32_t serial, uint32_t edges);
    void setToplevel();
    void setTransient(WlSurface *parent, int32_t x, int32_t y, uint32_t flags);
    void setFullscreen(uint32_t method, uint32_t framerate, WlOutput *output);
    void setPopup(WlSeat *seat, uint32_t serial, WlSurface *parent, int32_t x,
                  int32_t y, uint32_t flags);
    void setMaximized(WlOutput *output);
    void setTitle(const char *title);
    void setClass(const char *class_);

    auto &ping() { return pingSignal_; }
    auto &configure() { return configureSignal_; }
    auto &popupDone() { return popupDoneSignal_; }

private:
    static void destructor(wl_shell_surface *);
    static const struct wl_shell_surface_listener listener;
    fcitx::Signal<void(uint32_t)> pingSignal_;
    fcitx::Signal<void(uint32_t, int32_t, int32_t)> configureSignal_;
    fcitx::Signal<void()> popupDoneSignal_;

    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<wl_shell_surface, &destructor> data_;
};
static inline wl_shell_surface *rawPointer(WlShellSurface *p) {
    return p ? static_cast<wl_shell_surface *>(*p) : nullptr;
}

} // namespace fcitx::wayland

#endif // WL_SHELL_SURFACE_H_
