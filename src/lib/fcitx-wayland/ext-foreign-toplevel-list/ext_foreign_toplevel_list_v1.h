#ifndef EXT_FOREIGN_TOPLEVEL_LIST_V1_H_
#define EXT_FOREIGN_TOPLEVEL_LIST_V1_H_
#include <cstdint>
#include <wayland-client.h>
#include <wayland-util.h>
#include "fcitx-utils/misc.h"
#include "fcitx-utils/signals.h"
#include "wayland-ext-foreign-toplevel-list-v1-client-protocol.h" // IWYU pragma: export
namespace fcitx::wayland {

class ExtForeignToplevelHandleV1;

class ExtForeignToplevelListV1 final {
public:
    static constexpr const char *interface = "ext_foreign_toplevel_list_v1";
    static constexpr const wl_interface *const wlInterface =
        &ext_foreign_toplevel_list_v1_interface;
    static constexpr const uint32_t version = 1;
    using wlType = ext_foreign_toplevel_list_v1;
    operator ext_foreign_toplevel_list_v1 *() { return data_.get(); }
    ExtForeignToplevelListV1(wlType *data);
    ExtForeignToplevelListV1(ExtForeignToplevelListV1 &&other) noexcept =
        delete;
    ExtForeignToplevelListV1 &
    operator=(ExtForeignToplevelListV1 &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
#if defined(EXT_FOREIGN_TOPLEVEL_LIST_V1_STOP_SINCE_VERSION)
    void stop();
#endif

    auto &toplevel() { return toplevelSignal_; }
    auto &finished() { return finishedSignal_; }

private:
    static void destructor(ext_foreign_toplevel_list_v1 *);
    static const struct ext_foreign_toplevel_list_v1_listener listener;
    fcitx::Signal<void(ExtForeignToplevelHandleV1 *)> toplevelSignal_;
    fcitx::Signal<void()> finishedSignal_;

    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<ext_foreign_toplevel_list_v1, &destructor> data_;
};
static inline ext_foreign_toplevel_list_v1 *
rawPointer(ExtForeignToplevelListV1 *p) {
    return p ? static_cast<ext_foreign_toplevel_list_v1 *>(*p) : nullptr;
}

} // namespace fcitx::wayland

#endif // EXT_FOREIGN_TOPLEVEL_LIST_V1_H_
