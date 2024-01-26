#ifndef ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1
#define ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1
#include <wayland-client.h>
#include "fcitx-utils/signals.h"
#include "wayland-wlr-foreign-toplevel-management-client-protocol.h"
namespace fcitx::wayland {
class WlOutput;
class WlSeat;
class WlSurface;
class ZwlrForeignToplevelHandleV1 final {
public:
    static constexpr const char *interface = "zwlr_foreign_toplevel_handle_v1";
    static constexpr const wl_interface *const wlInterface =
        &zwlr_foreign_toplevel_handle_v1_interface;
    static constexpr const uint32_t version = 3;
    typedef zwlr_foreign_toplevel_handle_v1 wlType;
    operator zwlr_foreign_toplevel_handle_v1 *() { return data_.get(); }
    ZwlrForeignToplevelHandleV1(wlType *data);
    ZwlrForeignToplevelHandleV1(ZwlrForeignToplevelHandleV1 &&other) noexcept =
        delete;
    ZwlrForeignToplevelHandleV1 &
    operator=(ZwlrForeignToplevelHandleV1 &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    void setMaximized();
    void unsetMaximized();
    void setMinimized();
    void unsetMinimized();
    void activate(WlSeat *seat);
    void close();
    void setRectangle(WlSurface *surface, int32_t x, int32_t y, int32_t width,
                      int32_t height);
    void setFullscreen(WlOutput *output);
    void unsetFullscreen();
    auto &title() { return titleSignal_; }
    auto &appId() { return appIdSignal_; }
    auto &outputEnter() { return outputEnterSignal_; }
    auto &outputLeave() { return outputLeaveSignal_; }
    auto &state() { return stateSignal_; }
    auto &done() { return doneSignal_; }
    auto &closed() { return closedSignal_; }
    auto &parent() { return parentSignal_; }

private:
    static void destructor(zwlr_foreign_toplevel_handle_v1 *);
    static const struct zwlr_foreign_toplevel_handle_v1_listener listener;
    fcitx::Signal<void(const char *)> titleSignal_;
    fcitx::Signal<void(const char *)> appIdSignal_;
    fcitx::Signal<void(WlOutput *)> outputEnterSignal_;
    fcitx::Signal<void(WlOutput *)> outputLeaveSignal_;
    fcitx::Signal<void(wl_array *)> stateSignal_;
    fcitx::Signal<void()> doneSignal_;
    fcitx::Signal<void()> closedSignal_;
    fcitx::Signal<void(ZwlrForeignToplevelHandleV1 *)> parentSignal_;
    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<zwlr_foreign_toplevel_handle_v1, &destructor> data_;
};
static inline zwlr_foreign_toplevel_handle_v1 *
rawPointer(ZwlrForeignToplevelHandleV1 *p) {
    return p ? static_cast<zwlr_foreign_toplevel_handle_v1 *>(*p) : nullptr;
}
} // namespace fcitx::wayland
#endif
