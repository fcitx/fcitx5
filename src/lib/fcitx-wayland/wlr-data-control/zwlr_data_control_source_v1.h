#ifndef ZWLR_DATA_CONTROL_SOURCE_V1
#define ZWLR_DATA_CONTROL_SOURCE_V1
#include <wayland-client.h>
#include "fcitx-utils/signals.h"
#include "wayland-wlr-data-control-unstable-v1-client-protocol.h"
namespace fcitx::wayland {
class ZwlrDataControlSourceV1 final {
public:
    static constexpr const char *interface = "zwlr_data_control_source_v1";
    static constexpr const wl_interface *const wlInterface =
        &zwlr_data_control_source_v1_interface;
    static constexpr const uint32_t version = 1;
    typedef zwlr_data_control_source_v1 wlType;
    operator zwlr_data_control_source_v1 *() { return data_.get(); }
    ZwlrDataControlSourceV1(wlType *data);
    ZwlrDataControlSourceV1(ZwlrDataControlSourceV1 &&other) noexcept = delete;
    ZwlrDataControlSourceV1 &
    operator=(ZwlrDataControlSourceV1 &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    void offer(const char *mimeType);
    auto &send() { return sendSignal_; }
    auto &cancelled() { return cancelledSignal_; }

private:
    static void destructor(zwlr_data_control_source_v1 *);
    static const struct zwlr_data_control_source_v1_listener listener;
    fcitx::Signal<void(const char *, int32_t)> sendSignal_;
    fcitx::Signal<void()> cancelledSignal_;
    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<zwlr_data_control_source_v1, &destructor> data_;
};
static inline zwlr_data_control_source_v1 *
rawPointer(ZwlrDataControlSourceV1 *p) {
    return p ? static_cast<zwlr_data_control_source_v1 *>(*p) : nullptr;
}
} // namespace fcitx::wayland
#endif
