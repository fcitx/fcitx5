#ifndef ZWLR_FOREIGN_TOPLEVEL_MANAGER_V1
#define ZWLR_FOREIGN_TOPLEVEL_MANAGER_V1
#include <memory>
#include <wayland-client.h>
#include "fcitx-utils/signals.h"
#include "wayland-wlr-foreign-toplevel-management-client-protocol.h"
namespace fcitx::wayland {
class ZwlrForeignToplevelHandleV1;
class ZwlrForeignToplevelManagerV1 final {
public:
    static constexpr const char *interface = "zwlr_foreign_toplevel_manager_v1";
    static constexpr const wl_interface *const wlInterface =
        &zwlr_foreign_toplevel_manager_v1_interface;
    static constexpr const uint32_t version = 3;
    typedef zwlr_foreign_toplevel_manager_v1 wlType;
    operator zwlr_foreign_toplevel_manager_v1 *() { return data_.get(); }
    ZwlrForeignToplevelManagerV1(wlType *data);
    ZwlrForeignToplevelManagerV1(
        ZwlrForeignToplevelManagerV1 &&other) noexcept = delete;
    ZwlrForeignToplevelManagerV1 &
    operator=(ZwlrForeignToplevelManagerV1 &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    void stop();
    auto &toplevel() { return toplevelSignal_; }
    auto &finished() { return finishedSignal_; }

private:
    static void destructor(zwlr_foreign_toplevel_manager_v1 *);
    static const struct zwlr_foreign_toplevel_manager_v1_listener listener;
    fcitx::Signal<void(ZwlrForeignToplevelHandleV1 *)> toplevelSignal_;
    fcitx::Signal<void()> finishedSignal_;
    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<zwlr_foreign_toplevel_manager_v1, &destructor> data_;
};
static inline zwlr_foreign_toplevel_manager_v1 *
rawPointer(ZwlrForeignToplevelManagerV1 *p) {
    return p ? static_cast<zwlr_foreign_toplevel_manager_v1 *>(*p) : nullptr;
}
} // namespace fcitx::wayland
#endif
