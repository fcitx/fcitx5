#ifndef ZWP_TABLET_V2
#define ZWP_TABLET_V2
#include <wayland-client.h>
#include "fcitx-utils/signals.h"
#include "wayland-tablet-client-protocol.h"
namespace fcitx::wayland {
class ZwpTabletV2 final {
public:
    static constexpr const char *interface = "zwp_tablet_v2";
    static constexpr const wl_interface *const wlInterface =
        &zwp_tablet_v2_interface;
    static constexpr const uint32_t version = 1;
    typedef zwp_tablet_v2 wlType;
    operator zwp_tablet_v2 *() { return data_.get(); }
    ZwpTabletV2(wlType *data);
    ZwpTabletV2(ZwpTabletV2 &&other) noexcept = delete;
    ZwpTabletV2 &operator=(ZwpTabletV2 &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    auto &name() { return nameSignal_; }
    auto &id() { return idSignal_; }
    auto &path() { return pathSignal_; }
    auto &done() { return doneSignal_; }
    auto &removed() { return removedSignal_; }

private:
    static void destructor(zwp_tablet_v2 *);
    static const struct zwp_tablet_v2_listener listener;
    fcitx::Signal<void(const char *)> nameSignal_;
    fcitx::Signal<void(uint32_t, uint32_t)> idSignal_;
    fcitx::Signal<void(const char *)> pathSignal_;
    fcitx::Signal<void()> doneSignal_;
    fcitx::Signal<void()> removedSignal_;
    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<zwp_tablet_v2, &destructor> data_;
};
static inline zwp_tablet_v2 *rawPointer(ZwpTabletV2 *p) {
    return p ? static_cast<zwp_tablet_v2 *>(*p) : nullptr;
}
} // namespace fcitx::wayland
#endif
