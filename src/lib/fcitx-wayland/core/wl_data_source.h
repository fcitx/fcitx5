#ifndef WL_DATA_SOURCE
#define WL_DATA_SOURCE
#include "fcitx-utils/signals.h"
#include <memory>
#include <wayland-client.h>
namespace fcitx {
namespace wayland {
class WlDataSource {
public:
    static constexpr const char *interface = "wl_data_source";
    static constexpr const wl_interface *const wlInterface =
        &wl_data_source_interface;
    static constexpr const uint32_t version = 3;
    typedef wl_data_source wlType;
    operator wl_data_source *() { return data_.get(); }
    WlDataSource(wlType *data);
    WlDataSource(WlDataSource &&other) : data_(std::move(other.data_)) {}
    WlDataSource &operator=(WlDataSource &&other) {
        data_ = std::move(other.data_);
        return *this;
    }
    auto actualVersion() const { return version_; }
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
    std::unique_ptr<wl_data_source, decltype(&destructor)> data_;
};
}
}
#endif
