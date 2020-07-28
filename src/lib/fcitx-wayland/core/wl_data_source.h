#ifndef WL_DATA_SOURCE
#define WL_DATA_SOURCE
#include <memory>
#include <wayland-client.h>
#include "fcitx-utils/signals.h"
namespace fcitx {
namespace wayland {
class WlDataSource final {
public:
    static constexpr const char *interface = "wl_data_source";
    static constexpr const wl_interface *const wlInterface =
        &wl_data_source_interface;
    static constexpr const uint32_t version = 3;
    typedef wl_data_source wlType;
    operator wl_data_source *() { return data_.get(); }
    WlDataSource(wlType *data);
    WlDataSource(WlDataSource &&other) noexcept = delete;
    WlDataSource &operator=(WlDataSource &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    void offer(const char *mimeType);
    void setActions(uint32_t dndActions);
    auto &target() { return targetSignal_; }
    auto &send() { return sendSignal_; }
    auto &cancelled() { return cancelledSignal_; }
    auto &dndDropPerformed() { return dndDropPerformedSignal_; }
    auto &dndFinished() { return dndFinishedSignal_; }
    auto &action() { return actionSignal_; }

private:
    static void destructor(wl_data_source *);
    static const struct wl_data_source_listener listener;
    fcitx::Signal<void(const char *)> targetSignal_;
    fcitx::Signal<void(const char *, int32_t)> sendSignal_;
    fcitx::Signal<void()> cancelledSignal_;
    fcitx::Signal<void()> dndDropPerformedSignal_;
    fcitx::Signal<void()> dndFinishedSignal_;
    fcitx::Signal<void(uint32_t)> actionSignal_;
    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<wl_data_source, &destructor> data_;
};
static inline wl_data_source *rawPointer(WlDataSource *p) {
    return p ? static_cast<wl_data_source *>(*p) : nullptr;
}
} // namespace wayland
} // namespace fcitx
#endif
