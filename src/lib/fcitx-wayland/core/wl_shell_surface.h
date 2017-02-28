#ifndef WL_SHELL_SURFACE
#define WL_SHELL_SURFACE
#include "fcitx-utils/signals.h"
#include <memory>
#include <wayland-client.h>
namespace fcitx {
namespace wayland {
class WlOutput;
class WlSeat;
class WlSurface;
class WlShellSurface {
public:
    static constexpr const char *interface = "wl_shell_surface";
    static constexpr const wl_interface *const wlInterface = &wl_shell_surface_interface;
    static constexpr const uint32_t version = 1;
    typedef wl_shell_surface wlType;
    operator wl_shell_surface *() { return data_.get(); }
    WlShellSurface(wlType *data);
    WlShellSurface(WlShellSurface &&other) : data_(std::move(other.data_)) {}
    WlShellSurface &operator=(WlShellSurface &&other) {
        data_ = std::move(other.data_);
        return *this;
    }
    auto actualVersion() const { return version_; }
    void pong(uint32_t serial);
    void move(WlSeat *seat, uint32_t serial);
    void resize(WlSeat *seat, uint32_t serial, uint32_t edges);
    void setToplevel();
    void setTransient(WlSurface *parent, int32_t x, int32_t y, uint32_t flags);
    void setFullscreen(uint32_t method, uint32_t framerate, WlOutput *output);
    void setPopup(WlSeat *seat, uint32_t serial, WlSurface *parent, int32_t x, int32_t y, uint32_t flags);
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
    std::unique_ptr<wl_shell_surface, decltype(&destructor)> data_;
};
}
}
#endif
