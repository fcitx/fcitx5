#ifndef WL_OUTPUT
#define WL_OUTPUT
#include "fcitx-utils/signals.h"
#include <memory>
#include <wayland-client.h>
namespace fcitx {
namespace wayland {
class WlOutput {
public:
    static constexpr const char *interface = "wl_output";
    static constexpr const wl_interface *const wlInterface =
        &wl_output_interface;
    static constexpr const uint32_t version = 3;
    typedef wl_output wlType;
    operator wl_output *() { return data_.get(); }
    WlOutput(wlType *data);
    WlOutput(WlOutput &&other) : data_(std::move(other.data_)) {}
    WlOutput &operator=(WlOutput &&other) {
        data_ = std::move(other.data_);
        return *this;
    }
    auto actualVersion() const { return version_; }
    auto &geometry() { return geometrySignal_; }
    auto &mode() { return modeSignal_; }
    auto &done() { return doneSignal_; }
    auto &scale() { return scaleSignal_; }

private:
    static void destructor(wl_output *);
    static const struct wl_output_listener listener;
    fcitx::Signal<void(int32_t, int32_t, int32_t, int32_t, int32_t,
                       const char *, const char *, int32_t)>
        geometrySignal_;
    fcitx::Signal<void(uint32_t, int32_t, int32_t, int32_t)> modeSignal_;
    fcitx::Signal<void()> doneSignal_;
    fcitx::Signal<void(int32_t)> scaleSignal_;
    uint32_t version_;
    std::unique_ptr<wl_output, decltype(&destructor)> data_;
};
}
}
#endif
