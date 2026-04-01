#ifndef EXT_DATA_CONTROL_SOURCE_V1
#define EXT_DATA_CONTROL_SOURCE_V1
#include <memory>
#include <wayland-client.h>
#include "fcitx-utils/signals.h"
#include "wayland-ext-data-control-unstable-v1-client-protocol.h"
namespace fcitx::wayland {
class ExtDataControlSourceV1 final {
public:
    static constexpr const char *interface = "ext_data_control_source_v1";
    static constexpr const wl_interface *const wlInterface =
        &ext_data_control_source_v1_interface;
    static constexpr const uint32_t version = 1;
    typedef ext_data_control_source_v1 wlType;
    operator ext_data_control_source_v1 *() { return data_.get(); }
    ExtDataControlSourceV1(wlType *data);
    ExtDataControlSourceV1(ExtDataControlSourceV1 &&other) noexcept = delete;
    ExtDataControlSourceV1 &
    operator=(ExtDataControlSourceV1 &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    void offer(const char *mimeType);
    auto &send() { return sendSignal_; }
    auto &cancelled() { return cancelledSignal_; }

private:
    static void destructor(ext_data_control_source_v1 *);
    static const struct ext_data_control_source_v1_listener listener;
    fcitx::Signal<void(const char *, int32_t)> sendSignal_;
    fcitx::Signal<void()> cancelledSignal_;
    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<ext_data_control_source_v1, &destructor> data_;
};
static inline ext_data_control_source_v1 *
rawPointer(ExtDataControlSourceV1 *p) {
    return p ? static_cast<ext_data_control_source_v1 *>(*p) : nullptr;
}
} // namespace fcitx::wayland
#endif
