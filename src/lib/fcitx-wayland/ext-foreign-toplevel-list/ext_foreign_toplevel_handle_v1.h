#ifndef EXT_FOREIGN_TOPLEVEL_HANDLE_V1_H_
#define EXT_FOREIGN_TOPLEVEL_HANDLE_V1_H_
#include <cstdint>
#include <wayland-client.h>
#include <wayland-util.h>
#include "fcitx-utils/misc.h"
#include "fcitx-utils/signals.h"
#include "wayland-ext-foreign-toplevel-list-v1-client-protocol.h" // IWYU pragma: export
namespace fcitx::wayland {

class ExtForeignToplevelHandleV1 final {
public:
    static constexpr const char *interface = "ext_foreign_toplevel_handle_v1";
    static constexpr const wl_interface *const wlInterface =
        &ext_foreign_toplevel_handle_v1_interface;
    static constexpr const uint32_t version = 1;
    using wlType = ext_foreign_toplevel_handle_v1;
    operator ext_foreign_toplevel_handle_v1 *() { return data_.get(); }
    ExtForeignToplevelHandleV1(wlType *data);
    ExtForeignToplevelHandleV1(ExtForeignToplevelHandleV1 &&other) noexcept =
        delete;
    ExtForeignToplevelHandleV1 &
    operator=(ExtForeignToplevelHandleV1 &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }

    auto &closed() { return closedSignal_; }
    auto &done() { return doneSignal_; }
    auto &title() { return titleSignal_; }
    auto &appId() { return appIdSignal_; }
    auto &identifier() { return identifierSignal_; }

private:
    static void destructor(ext_foreign_toplevel_handle_v1 *);
    static const struct ext_foreign_toplevel_handle_v1_listener listener;
    fcitx::Signal<void()> closedSignal_;
    fcitx::Signal<void()> doneSignal_;
    fcitx::Signal<void(const char *)> titleSignal_;
    fcitx::Signal<void(const char *)> appIdSignal_;
    fcitx::Signal<void(const char *)> identifierSignal_;

    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<ext_foreign_toplevel_handle_v1, &destructor> data_;
};
static inline ext_foreign_toplevel_handle_v1 *
rawPointer(ExtForeignToplevelHandleV1 *p) {
    return p ? static_cast<ext_foreign_toplevel_handle_v1 *>(*p) : nullptr;
}

} // namespace fcitx::wayland

#endif // EXT_FOREIGN_TOPLEVEL_HANDLE_V1_H_
