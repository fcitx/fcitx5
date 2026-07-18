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
#if defined(WL_SHELL_SURFACE_PONG_SINCE_VERSION)
    void pong(uint32_t serial);
#endif
#if defined(WL_SHELL_SURFACE_MOVE_SINCE_VERSION)
    void move(WlSeat *seat, uint32_t serial);
#endif
#if defined(WL_SHELL_SURFACE_RESIZE_SINCE_VERSION)
    void resize(WlSeat *seat, uint32_t serial, uint32_t edges);
#endif
#if defined(WL_SHELL_SURFACE_SET_TOPLEVEL_SINCE_VERSION)
    void setToplevel();
#endif
#if defined(WL_SHELL_SURFACE_SET_TRANSIENT_SINCE_VERSION)
    void setTransient(WlSurface *parent, int32_t x, int32_t y, uint32_t flags);
#endif
#if defined(WL_SHELL_SURFACE_SET_FULLSCREEN_SINCE_VERSION)
    void setFullscreen(uint32_t method, uint32_t framerate, WlOutput *output);
#endif
#if defined(WL_SHELL_SURFACE_SET_POPUP_SINCE_VERSION)
    void setPopup(WlSeat *seat, uint32_t serial, WlSurface *parent, int32_t x,
                  int32_t y, uint32_t flags);
#endif
#if defined(WL_SHELL_SURFACE_SET_MAXIMIZED_SINCE_VERSION)
    void setMaximized(WlOutput *output);
#endif
#if defined(WL_SHELL_SURFACE_SET_TITLE_SINCE_VERSION)
    void setTitle(const char *title);
#endif
#if defined(WL_SHELL_SURFACE_SET_CLASS_SINCE_VERSION)
    void setClass(const char *class_);
#endif

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
