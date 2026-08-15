#ifndef ZCOSMIC_TOPLEVEL_INFO_V1_H_
#define ZCOSMIC_TOPLEVEL_INFO_V1_H_
#include <cstdint>
#include <wayland-client.h>
#include <wayland-util.h>
#include "fcitx-utils/misc.h"
#include "fcitx-utils/signals.h"
#include "wayland-cosmic-toplevel-info-unstable-v1-client-protocol.h" // IWYU pragma: export
namespace fcitx::wayland {

class ExtForeignToplevelHandleV1;
class ZcosmicToplevelHandleV1;

class ZcosmicToplevelInfoV1 final {
public:
    static constexpr const char *interface = "zcosmic_toplevel_info_v1";
    static constexpr const wl_interface *const wlInterface =
        &zcosmic_toplevel_info_v1_interface;
    static constexpr const uint32_t version = 3;
    using wlType = zcosmic_toplevel_info_v1;
    operator zcosmic_toplevel_info_v1 *() { return data_.get(); }
    ZcosmicToplevelInfoV1(wlType *data);
    ZcosmicToplevelInfoV1(ZcosmicToplevelInfoV1 &&other) noexcept = delete;
    ZcosmicToplevelInfoV1 &
    operator=(ZcosmicToplevelInfoV1 &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
#if defined(ZCOSMIC_TOPLEVEL_INFO_V1_STOP_SINCE_VERSION)
    void stop();
#endif
#if defined(ZCOSMIC_TOPLEVEL_INFO_V1_GET_COSMIC_TOPLEVEL_SINCE_VERSION)
    ZcosmicToplevelHandleV1 *
    getCosmicToplevel(ExtForeignToplevelHandleV1 *foreignToplevel);
#endif

    auto &toplevel() { return toplevelSignal_; }
    auto &finished() { return finishedSignal_; }
    auto &done() { return doneSignal_; }

private:
    static void destructor(zcosmic_toplevel_info_v1 *);
    static const struct zcosmic_toplevel_info_v1_listener listener;
    fcitx::Signal<void(ZcosmicToplevelHandleV1 *)> toplevelSignal_;
    fcitx::Signal<void()> finishedSignal_;
    fcitx::Signal<void()> doneSignal_;

    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<zcosmic_toplevel_info_v1, &destructor> data_;
};
static inline zcosmic_toplevel_info_v1 *rawPointer(ZcosmicToplevelInfoV1 *p) {
    return p ? static_cast<zcosmic_toplevel_info_v1 *>(*p) : nullptr;
}

} // namespace fcitx::wayland

#endif // ZCOSMIC_TOPLEVEL_INFO_V1_H_
