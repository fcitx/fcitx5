#ifndef ZCOSMIC_TOPLEVEL_HANDLE_V1_H_
#define ZCOSMIC_TOPLEVEL_HANDLE_V1_H_
#include <cstdint>
#include <wayland-client.h>
#include <wayland-util.h>
#include "fcitx-utils/misc.h"
#include "fcitx-utils/signals.h"
#include "wayland-cosmic-toplevel-info-unstable-v1-client-protocol.h" // IWYU pragma: export
namespace fcitx::wayland {

class ExtWorkspaceHandleV1;
class WlOutput;
class ZcosmicWorkspaceHandleV1;

class ZcosmicToplevelHandleV1 final {
public:
    static constexpr const char *interface = "zcosmic_toplevel_handle_v1";
    static constexpr const wl_interface *const wlInterface =
        &zcosmic_toplevel_handle_v1_interface;
    static constexpr const uint32_t version = 3;
    using wlType = zcosmic_toplevel_handle_v1;
    operator zcosmic_toplevel_handle_v1 *() { return data_.get(); }
    ZcosmicToplevelHandleV1(wlType *data);
    ZcosmicToplevelHandleV1(ZcosmicToplevelHandleV1 &&other) noexcept = delete;
    ZcosmicToplevelHandleV1 &
    operator=(ZcosmicToplevelHandleV1 &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }

    auto &closed() { return closedSignal_; }
    auto &done() { return doneSignal_; }
    auto &title() { return titleSignal_; }
    auto &appId() { return appIdSignal_; }
    auto &outputEnter() { return outputEnterSignal_; }
    auto &outputLeave() { return outputLeaveSignal_; }
    auto &workspaceEnter() { return workspaceEnterSignal_; }
    auto &workspaceLeave() { return workspaceLeaveSignal_; }
    auto &state() { return stateSignal_; }
    auto &geometry() { return geometrySignal_; }
    auto &extWorkspaceEnter() { return extWorkspaceEnterSignal_; }
    auto &extWorkspaceLeave() { return extWorkspaceLeaveSignal_; }

private:
    static void destructor(zcosmic_toplevel_handle_v1 *);
    static const struct zcosmic_toplevel_handle_v1_listener listener;
    fcitx::Signal<void()> closedSignal_;
    fcitx::Signal<void()> doneSignal_;
    fcitx::Signal<void(const char *)> titleSignal_;
    fcitx::Signal<void(const char *)> appIdSignal_;
    fcitx::Signal<void(WlOutput *)> outputEnterSignal_;
    fcitx::Signal<void(WlOutput *)> outputLeaveSignal_;
    fcitx::Signal<void(ZcosmicWorkspaceHandleV1 *)> workspaceEnterSignal_;
    fcitx::Signal<void(ZcosmicWorkspaceHandleV1 *)> workspaceLeaveSignal_;
    fcitx::Signal<void(wl_array *)> stateSignal_;
    fcitx::Signal<void(WlOutput *, int32_t, int32_t, int32_t, int32_t)>
        geometrySignal_;
    fcitx::Signal<void(ExtWorkspaceHandleV1 *)> extWorkspaceEnterSignal_;
    fcitx::Signal<void(ExtWorkspaceHandleV1 *)> extWorkspaceLeaveSignal_;

    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<zcosmic_toplevel_handle_v1, &destructor> data_;
};
static inline zcosmic_toplevel_handle_v1 *
rawPointer(ZcosmicToplevelHandleV1 *p) {
    return p ? static_cast<zcosmic_toplevel_handle_v1 *>(*p) : nullptr;
}

} // namespace fcitx::wayland

#endif // ZCOSMIC_TOPLEVEL_HANDLE_V1_H_
